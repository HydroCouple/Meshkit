// SPDX-License-Identifier: Apache-2.0
#include <emscripten/bind.h>
#include <meshkit/job.hpp>
#include <meshkit/raster.hpp>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using emscripten::val;

template<typename T>
val copyArray(const std::vector<T>& values)
{
    return val(emscripten::typed_memory_view(values.size(), values.data())).call<val>("slice");
}

bool has(const val& object, const char* key)
{
    if (object.isUndefined() || object.isNull()) return false;
    const val v = object[key];
    return !v.isUndefined() && !v.isNull();
}

double integer(val value, double minimum, double maximum, const char* what)
{
    const auto n = value.as<double>();
    if (!std::isfinite(n) || n < minimum || n > maximum || std::floor(n) != n)
        throw std::invalid_argument(std::string("Invalid integer for ") + what);
    return n;
}

unsigned boundedUnsigned(val value, unsigned maximum)
{
    return static_cast<unsigned>(integer(value, 0, maximum, "mesh request"));
}

double finiteOption(const val& options, const char* key, double fallback)
{
    if (!has(options, key)) return fallback;
    const auto n = options[key].as<double>();
    if (!std::isfinite(n)) throw std::invalid_argument(std::string("Option ") + key + " must be finite");
    return n;
}

int intOption(const val& options, const char* key, int fallback, int minimum, int maximum)
{
    if (!has(options, key)) return fallback;
    return static_cast<int>(integer(options[key], minimum, maximum, key));
}

bool boolOption(const val& options, const char* key, bool fallback)
{
    return has(options, key) ? options[key].as<bool>() : fallback;
}

std::string stringOption(const val& options, const char* key)
{
    return has(options, key) ? options[key].as<std::string>() : std::string();
}

/// Meshing policy from `request.options`; every key is optional and the native job checks ranges.
void applyOptions(meshkit::JobOptions& o, const val& options)
{
    if (options.isUndefined() || options.isNull()) return;
    o.minimumTarget = finiteOption(options, "minimumTarget", o.minimumTarget);
    o.minimumEdge = finiteOption(options, "minimumEdge", o.minimumEdge);
    o.minimumAngleDegrees = finiteOption(options, "minimumAngleDegrees", o.minimumAngleDegrees);
    o.adjacentRatio = finiteOption(options, "adjacentRatio", o.adjacentRatio);
    o.flatElevation = finiteOption(options, "flatElevation", o.flatElevation);
    o.coarseningReferenceSize = finiteOption(options, "coarseningReferenceSize", o.coarseningReferenceSize);
    o.terrainTolerance = finiteOption(options, "terrainTolerance", o.terrainTolerance);
    o.smoothingPasses = intOption(options, "smoothingPasses", o.smoothingPasses, 0, 10);
    o.coarsenPasses = intOption(options, "coarsenPasses", o.coarsenPasses, 0, 10);
    o.sizingRepairPasses = intOption(options, "sizingRepairPasses", o.sizingRepairPasses, 0, 3);
    if (has(options, "repairInsertions"))
        o.repairInsertions = static_cast<std::uint32_t>(integer(options["repairInsertions"], 0, 4294967295.0, "repairInsertions"));
    o.strictSizing = boolOption(options, "strictSizing", o.strictSizing);
    o.requireTerrainCertification = boolOption(options, "requireTerrainCertification", o.requireTerrainCertification);
    o.retainInputMapping = boolOption(options, "retainInputMapping", o.retainInputMapping);
    const auto order = stringOption(options, "coarseningOrder");
    if (order == "resolutionAware") o.coarseningOrder = meshkit::CoarseningOrder::ResolutionAware;
    else if (order == "legacyFlatness" || order.empty()) o.coarseningOrder = meshkit::CoarseningOrder::LegacyFlatness;
    else throw std::invalid_argument("Unknown coarseningOrder " + order);
    const auto cells = stringOption(options, "cells");
    if (cells == "automaticMixed") o.cells = meshkit::CellPlan::AutomaticMixed;
    else if (cells == "triangles" || cells.empty()) o.cells = meshkit::CellPlan::Triangles;
    else throw std::invalid_argument("Unknown cells plan " + cells);
}

template<typename T>
std::vector<T> perPoint(const val& input, const char* key, unsigned count, double minimum, double maximum, bool allowNaN)
{
    std::vector<T> out;
    if (!has(input, key)) return out;
    const val array = input[key];
    if (boundedUnsigned(array["length"], count) != count)
        throw std::invalid_argument(std::string(key) + " must have one entry per point");
    out.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        const auto n = array[i].as<double>();
        if (allowNaN && std::isnan(n)) { out.push_back(static_cast<T>(n)); continue; }
        if (!std::isfinite(n) || n < minimum || n > maximum)
            throw std::invalid_argument(std::string("Invalid value in ") + key);
        out.push_back(static_cast<T>(n));
    }
    return out;
}

val reportObject(const meshkit::JobReport& r)
{
    val out = val::object();
    out.set("algorithmVersion", r.algorithmVersion);
    out.set("constraintAuditPassed", r.constraintAuditPassed);
    out.set("constraintSegments", static_cast<double>(r.constraintSegments));
    out.set("plannedPatches", static_cast<double>(r.plannedPatches));
    out.set("planningRetries", static_cast<double>(r.planningRetries));
    out.set("inserted", static_cast<double>(r.inserted));
    out.set("removed", static_cast<double>(r.removed));
    out.set("belowMinimum", static_cast<double>(r.belowMinimum));
    out.set("edgeConflicts", static_cast<double>(r.edgeConflicts));
    out.set("sizeConflicts", static_cast<double>(r.sizeConflicts));
    out.set("transitionConflicts", static_cast<double>(r.transitionConflicts));
    out.set("terrainConflicts", static_cast<double>(r.terrainConflicts));
    out.set("terrainUnknown", static_cast<double>(r.terrainUnknown));
    out.set("largestAdjacentRatio", r.largestAdjacentRatio);
    out.set("largestTerrainRatio", r.largestTerrainRatio);
    out.set("exampleCount", static_cast<double>(r.examples.size()));
    val resolved = val::object();
    const auto& o = r.resolvedOptions;
    resolved.set("maximumSize", o.maximumSize);
    resolved.set("minimumTarget", o.minimumTarget);
    resolved.set("minimumEdge", o.minimumEdge);
    resolved.set("minimumAngleDegrees", o.minimumAngleDegrees);
    resolved.set("adjacentRatio", o.adjacentRatio);
    resolved.set("smoothingPasses", o.smoothingPasses);
    resolved.set("coarsenPasses", o.coarsenPasses);
    resolved.set("sizingRepairPasses", o.sizingRepairPasses);
    resolved.set("strictSizing", o.strictSizing);
    resolved.set("terrainTolerance", o.terrainTolerance);
    out.set("resolvedOptions", resolved);
    return out;
}

val generate(val input)
{
    meshkit::JobRequest request;
    const val points = input["points"], paths = input["paths"];
    const auto length = boundedUnsigned(points["length"], 400000);
    if (length % 2) throw std::invalid_argument("Points must be XY pairs");
    const unsigned count = length / 2;
    for (unsigned i = 0; i < length; i += 2)
        request.points.push_back({points[i].as<double>(), points[i+1].as<double>()});
    const auto pathCount = boundedUnsigned(paths["length"], 200000);
    unsigned remaining = 400000;
    for (unsigned i = 0; i < pathCount; ++i) {
        const val source = paths[i], indices = source["vertices"];
        meshkit::Path path;
        path.kind = static_cast<meshkit::PathKind>(boundedUnsigned(source["kind"], 3));
        path.marker = has(source, "marker") ? source["marker"].as<int>() : 0;
        if (has(source, "closed")) path.closed = source["closed"].as<bool>();
        const auto n = boundedUnsigned(indices["length"], remaining);
        remaining -= n;
        for (unsigned j = 0; j < n; ++j)
            path.vertices.push_back(boundedUnsigned(indices[j], count));
        request.paths.push_back(std::move(path));
    }
    request.pointElevations = perPoint<double>(input, "pointElevations", count, -1e12, 1e12, true);
    request.pointIds = perPoint<std::int64_t>(input, "pointIds", count, 1, 9007199254740991.0, false);
    request.pointMarkers = perPoint<std::int32_t>(input, "pointMarkers", count, -2147483648.0, 2147483647.0, false);
    if (has(input, "labels")) {
        const val labels = input["labels"];
        const auto n = boundedUnsigned(labels["length"], 100000);
        for (unsigned i = 0; i < n; ++i) {
            meshkit::Label label;
            label.id = static_cast<std::int32_t>(integer(labels[i]["id"], -2147483648.0, 2147483647.0, "label id"));
            label.name = labels[i]["name"].as<std::string>();
            request.labels.push_back(std::move(label));
        }
    }
    if (has(input, "regions")) {
        const val regions = input["regions"];
        const auto n = boundedUnsigned(regions["length"], 100000);
        for (unsigned i = 0; i < n; ++i) {
            meshkit::RegionSeed seed;
            seed.point = {regions[i]["x"].as<double>(), regions[i]["y"].as<double>()};
            seed.region = static_cast<std::int32_t>(integer(regions[i]["region"], -2147483648.0, 2147483647.0, "region id"));
            request.regions.push_back(seed);
        }
    }
    if (has(input, "metadata")) {
        const val m = input["metadata"];
        request.metadata.horizontalMetresPerUnit = finiteOption(m, "horizontalMetresPerUnit", 1);
        request.metadata.verticalMetresPerUnit = finiteOption(m, "verticalMetresPerUnit", 1);
        request.metadata.crs = stringOption(m, "crs");
        request.metadata.description = stringOption(m, "description");
    }
    request.options.maximumSize = input["maximumSize"].as<double>();
    applyOptions(request.options, input["options"]);
    const val limits = input["limits"];
    request.options.limits.maxCells = has(limits, "maxCells") ? boundedUnsigned(limits["maxCells"], 4000000) : 200000;
    request.options.limits.maxVertices = has(limits, "maxVertices") ? boundedUnsigned(limits["maxVertices"], 4000000) : 200000;
    request.options.limits.maxWorkspaceBytes =
        std::uint64_t(has(limits, "maxWorkspaceMB") ? boundedUnsigned(limits["maxWorkspaceMB"], 2048) : 256u) << 20;

    meshkit::Providers providers;
    if (has(input, "raster")) {
        const val raster = input["raster"];
        meshkit::RasterData grid;
        grid.columns = boundedUnsigned(raster["columns"], 16384);
        grid.rows = boundedUnsigned(raster["rows"], 16384);
        const auto cells = grid.rows * grid.columns;
        if (cells > 16000000 || boundedUnsigned(raster["heights"]["length"], 16000000) != cells)
            throw std::invalid_argument("Raster dimensions and height count must agree");
        grid.origin = {raster["x"].as<double>(), raster["y"].as<double>()};
        grid.xSpacing = raster["dx"].as<double>();
        grid.ySpacing = raster["dy"].as<double>();
        grid.heights = emscripten::convertJSArrayToNumberVector<double>(raster["heights"]);
        providers = meshkit::RasterSurface(std::move(grid)).providers();
    }
    const auto job = meshkit::runJob(request, providers);
    val out = val::object();
    out.set("ok", job.report.complete());
    out.set("status", std::string(meshkit::statusName(job.report.status)));
    out.set("code", job.report.code);
    out.set("message", job.report.message);
    out.set("constraintAuditPassed", job.report.constraintAuditPassed);
    out.set("schemaVersion", job.report.schemaVersion);
    out.set("report", reportObject(job.report));
    if (!job.report.complete()) return out;
    std::vector<double> xyz, vertexIds;
    std::vector<meshkit::Index> triangles, quads, edges;
    std::vector<std::int32_t> markers, triangleRegion, quadRegion;
    for (const auto& p : job.mesh.vertices) xyz.insert(xyz.end(), {p.x, p.y, p.z});
    for (const auto& c : job.mesh.cells) {
        if (c.count == 4) {
            quads.insert(quads.end(), c.vertices.begin(), c.vertices.begin() + 4);
            quadRegion.push_back(c.region);
        } else {
            triangles.insert(triangles.end(), c.vertices.begin(), c.vertices.begin() + 3);
            triangleRegion.push_back(c.region);
        }
    }
    for (const auto& edge : job.mesh.edges) {
        edges.insert(edges.end(), {edge.a, edge.b});
        markers.push_back(edge.marker);
    }
    for (const auto id : job.mesh.vertexIds) vertexIds.push_back(static_cast<double>(id));
    out.set("xyz", copyArray(xyz));
    out.set("triangles", copyArray(triangles));
    out.set("triangleRegion", copyArray(triangleRegion));
    out.set("quads", copyArray(quads));
    out.set("quadRegion", copyArray(quadRegion));
    out.set("edgeVertices", copyArray(edges));
    out.set("edgeMarkers", copyArray(markers));
    out.set("vertexMarkers", copyArray(job.mesh.vertexMarkers));
    out.set("vertexIds", copyArray(vertexIds));
    out.set("sourceVertex", copyArray(job.inputVertexToOutput));
    val labels = val::array();
    for (const auto& label : job.mesh.labels) {
        val entry = val::object();
        entry.set("id", label.id);
        entry.set("name", label.name);
        labels.call<void>("push", entry);
    }
    out.set("labels", labels);
    val metadata = val::object();
    metadata.set("horizontalMetresPerUnit", job.mesh.metadata.horizontalMetresPerUnit);
    metadata.set("verticalMetresPerUnit", job.mesh.metadata.verticalMetresPerUnit);
    metadata.set("crs", job.mesh.metadata.crs);
    metadata.set("description", job.mesh.metadata.description);
    out.set("metadata", metadata);
    return out;
}

val run(val input)
{
    try { return generate(input); }
    catch (const std::exception& e) {
        val failure = val::object();
        failure.set("ok", false);
        failure.set("status", std::string("failed"));
        failure.set("message", std::string(e.what()));
        return failure;
    }
}
}
EMSCRIPTEN_BINDINGS(meshkit_browser) { emscripten::function("runJob", &run); }
