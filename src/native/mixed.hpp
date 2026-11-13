// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshkit/job.hpp"
#include "meshkit/planning.hpp"
#include "meshkit/blocks.hpp"
#include "meshkit/sizefield.hpp"
#include "operation.hpp"
#include <unordered_map>
#include <cstring>
namespace meshkit::native_detail
{
/** @brief Exact hash-map key of an XY point: the bit patterns of its two doubles. */
struct Coordinate {
    /// IEEE-754 bit pattern of x (see coordinate()).
    std::uint64_t x = 0, y = 0;
    /** @var y
     *  @brief IEEE-754 bit pattern of y. */
    /**
     * @brief Bitwise equality of both patterns.
     * @param[in] b Key to compare with.
     * @return True when both bit patterns match.
     */
    bool operator==(const Coordinate &b) const { return x == b.x && y == b.y; }
};
/** @brief Hash functor for Coordinate keys in unordered containers. */
struct CoordinateHash {
    /**
     * @brief Mix the two bit patterns (boost-style hash_combine).
     * @param[in] p Key to hash.
     * @return Hash value.
     */
    std::size_t operator()(Coordinate p) const
    {
        return p.x ^ (p.y + 0x9e3779b97f4a7c15ull + (p.x << 6) + (p.x >> 2));
    }
};
/**
 * @brief Build the exact key of a point, mapping -0.0 to +0.0.
 * @param[in] p Point; no tolerance is applied, so distinct doubles give distinct keys.
 * @return Key holding the bit patterns of p.x() and p.y().
 */
inline Coordinate coordinate(Point2 p)
{
    double x = p.x() == 0 ? 0 : p.x(), y = p.y() == 0 ? 0 : p.y();
    Coordinate k;
    std::memcpy(&k.x, &x, 8);
    std::memcpy(&k.y, &y, 8);
    return k;
}
/** @brief Quad patches planned before triangulation, with the extra sources they need. */
struct PreparedPatches {
    /// Fixed patches from the request, in order, then the planner's patches.
    std::vector<plan::PatchMesh> patches;
    /// Per patch: planning-region index, -1 for fixed patches. Indices below the request's
    /// quadRegions.size() are authored regions; larger ones are automatic block tiles.
    std::vector<int> regions;
    /// Planner decision records, one per planning region (authored, then automatic tiles).
    std::vector<plan::PatchDecision> decisions;
    /// Request points, then explicit region-ring vertices, then patch boundary stations.
    std::vector<Point2> points;
    /// One closed PathKind::Region path per explicit (non-background, non-automatic) region,
    /// id -(i + 1) for planning region i, indexing #points.
    std::vector<Path> extraPaths;
    /// Per patch, per patch vertex: index in #points of its boundary station, or -1 for a
    /// vertex on no boundary segment.
    std::vector<std::vector<int>> boundarySources;
    /// Total quads across #patches.
    std::uint64_t cells = 0, sharedSides = 0;
    /** @var sharedSides
     *  @brief Patch sides the joint planner matched to an already registered side. */
};
/**
 * @brief Plan the request's fixed, authored and automatic quad patches before triangulation.
 *
 * Outer and hole paths become planning domains and holes, other paths constraint lines, and
 * points on no path become pins. With automatic blocks, a candidate grid over the outer
 * paths' bounding box is sampled from maximumSize and the placed blocks are split into tiles
 * added as background automatic-candidate regions. Fixed patches are validated and assessed.
 * Callbacks run synchronously; their exceptions propagate.
 * @param[in] r Borrowed job request; paths must index r.points.
 * @param[in] p Borrowed providers: cancelled, and with a positive terrain tolerance also
 * elevation and terrainBound, are passed to the planner.
 * @param[in] maximumSize Borrowed footprint size bound in XY units (0 = unbounded).
 * @param[in] tolerance Borrowed footprint terrain tolerance in Z units; used only with a
 * positive request terrain tolerance.
 * @param[in,out] work Budget charged for planner work and tiles; also gates the result's
 * workspace estimate.
 * @param[in] attempt Retry number; not used by this function.
 * @return Owned patches, sources and decisions.
 * @throws Stop on invalid input (`path_index`, `region_geometry`), an invalid or cancelled
 * block grid (`planning_grid`), fixed quads in triangles-only mode (`quad_conflict`), planning
 * failure (`quad_plan`, with the planner's Cancelled/ResourceLimit/Infeasible status), invalid
 * fixed patches (`fixed_patch`), result counts above the job limits (`planning_limit`), or
 * from work.
 */
inline PreparedPatches preparePatches(const JobRequest &r, const Providers &p,
                                      const std::function<double(const Box2 &)> &maximumSize,
                                      const std::function<double(const Box2 &)> &tolerance,
                                      Work &work, int attempt)
{
    PreparedPatches out;
    out.points = r.points;
    plan::QuadPlanningGeometry geometry;
    for (const auto &path : r.paths) {
        plan::Polygon polygon;
        for (auto index : path.vertices) {
            if (index >= r.points.size())
                fail("path_index", "Invalid source index before planning.");
            polygon.append(r.points[index]);
        }
        if (path.kind == PathKind::Outer)
            geometry.domains.append(polygon);
        else if (path.kind == PathKind::Hole)
            geometry.holes.append(polygon);
        else
            geometry.lines.append(polygon);
    }
    // Source identities that already lie on a required path are protected by its
    // segment graph. Unattached mandatory points additionally constrain placement.
    std::vector<bool> inPath(r.points.size(), false);
    for (const auto &path : r.paths)
        for (auto i : path.vertices)
            inPath[i] = true;
    for (std::size_t i = 0; i < r.points.size(); ++i)
        if (!inPath[i])
            geometry.pins.append(r.points[i]);
    plan::Vector<plan::QuadRegion> regions(r.quadRegions);
    plan::QuadPlanningOptions options;
    options.defaultSize =
        r.options.maximumSize > 0 ? r.options.maximumSize : std::max(1., r.options.minimumTarget);
    options.maximumCells = r.options.limits.maxCells;
    options.maximumPatchCells = r.options.maximumPatchCells;
    options.maximumWorkspaceBytes = r.options.limits.maxWorkspaceBytes;
    options.workBudget = r.options.limits.maxWork;
    options.limits.minimumAngleDegrees = r.options.minimumAngleDegrees;
    options.limits.minimumEdge = r.options.minimumEdge;
    options.limits.maximumSize = maximumSize;
    options.limits.cancelled = p.cancelled;
    if (r.options.terrainTolerance > 0) {
        options.limits.terrainTolerance = r.options.terrainTolerance;
        options.limits.terrainToleranceForFootprint = tolerance;
        options.limits.elevationAt = p.elevation;
        options.limits.terrainBound = p.terrainBound;
    }
    if (r.options.automaticBlocks) {
        Box2 bounds;
        bool first = true;
        plan::Vector<plan::Vector<Point2>> blockers;
        for (const auto &path : r.paths) {
            plan::Vector<Point2> line;
            for (auto i : path.vertices)
                line.append(r.points[i]);
            if (path.closed || path.kind != PathKind::Line)
                line.append(line.first());
            blockers.append(line);
            if (path.kind == PathKind::Outer) {
                auto box = plan::Polygon(line).boundingRect();
                bounds = first ? box : bounds.united(box);
                first = false;
            }
        }
        for (const auto &region : regions) {
            auto line = plan::Vector<Point2>(region.ring);
            if (!line.empty())
                line.append(line.first());
            blockers.append(line);
        }
        const double pitch =
            std::max(4 * std::max(r.options.minimumTarget, options.defaultSize / 4),
                     std::max(bounds.width(), bounds.height()) / 256);
        const auto grid =
            plan::quadBlockGridFromField(bounds, pitch, maximumSize, blockers, p.cancelled);
        if (!grid.isValid())
            fail("planning_grid", "Automatic patch grid is invalid or cancelled.");
        plan::QuadBlockOptions blocks;
        blocks.maxBlocks = r.options.maximumAutomaticBlocks;
        blocks.minSpacing = std::max(.5 * r.options.minimumTarget, 1e-9);
        auto automatic = plan::placeQuadBlocks(grid, blocks);
        int added = 0;
        for (auto &region : automatic) {
            const auto box = region.ring.boundingRect();
            // Leave room for up to three rounds of terrain/metric refinement.
            const double side =
                region.spacing * std::sqrt(std::max(16., double(r.options.maximumPatchCells) / 64));
            const int nx = std::max(1, int(std::ceil(box.width() / side))),
                      ny = std::max(1, int(std::ceil(box.height() / side)));
            for (int y = 0; y < ny && added < blocks.maxBlocks; ++y)
                for (int x = 0; x < nx && added < blocks.maxBlocks; ++x) {
                    work.tick();
                    auto tile = region;
                    const double gap = std::min(region.spacing,
                                                .1 * std::min(box.width() / nx, box.height() / ny));
                    const double x0 = box.left() + x * box.width() / nx + (x ? gap : 0),
                                 x1 = box.left() + (x + 1) * box.width() / nx -
                                      (x + 1 < nx ? gap : 0);
                    const double y0 = box.top() + y * box.height() / ny + (y ? gap : 0),
                                 y1 = box.top() + (y + 1) * box.height() / ny -
                                      (y + 1 < ny ? gap : 0);
                    tile.ring = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
                    tile.automaticCandidate = true;
                    tile.isBackground = true;
                    regions.append(std::move(tile));
                    ++added;
                }
        }
    }
    if (r.options.cells == CellPlan::Triangles) {
        options.allowQuads = false;
        if (!r.fixedPatches.empty())
            fail("quad_conflict", "Fixed quads conflict with triangles-only mode.");
    }
    geometry.fixedPatches = plan::Vector<plan::PatchMesh>(r.fixedPatches);
    auto result = plan::planQuadRegions(regions, geometry, options);
    work.tick(result.workUsed);
    if (!result.ok)
        fail("quad_plan", result.error,
             result.cancelled         ? RunStatus::Cancelled
             : result.resourceLimited ? RunStatus::ResourceLimit
                                      : RunStatus::Infeasible);
    out.sharedSides = result.sharedSides;
    out.decisions.assign(result.decisions.begin(), result.decisions.end());
    for (const auto &fixed : r.fixedPatches) {
        auto error = plan::validate(fixed);
        if (!error.empty())
            fail("fixed_patch", error);
        const auto assessment = plan::assessPlannedPatch(fixed, options.limits);
        if (!assessment.ok)
            fail("fixed_patch", assessment.reason);
        out.patches.push_back(fixed);
        out.regions.push_back(-1);
    }
    for (std::size_t i = 0; i < result.patches.size(); ++i) {
        out.patches.push_back(std::move(result.patches[i]));
        out.regions.push_back(result.patchRegions[i]);
    }
    auto addRing = [&](const plan::Polygon &ring, PathKind kind, std::int64_t id) {
        Path path;
        path.kind = kind;
        path.closed = true;
        path.id = id;
        for (auto point : ring) {
            if (!finite(point))
                fail("region_geometry", "Nonfinite region coordinate.");
            path.vertices.push_back(Index(out.points.size()));
            out.points.push_back(point);
        }
        out.extraPaths.push_back(std::move(path));
    };
    for (std::size_t i = 0; i < regions.size(); ++i)
        if (!regions[i].isBackground && !regions[i].automaticCandidate)
            addRing(regions[i].ring, PathKind::Region, -std::int64_t(i + 1));
    for (const auto &patch : out.patches) {
        out.cells += patch.quads.size();
        std::vector<int> sources(patch.xy.size(), -1);
        for (auto edge : patch.boundarySegments)
            for (int v : {edge.first, edge.second})
                if (sources[v] < 0) {
                    sources[v] = int(out.points.size());
                    out.points.push_back(patch.xy[v]);
                }
        out.boundarySources.push_back(std::move(sources));
    }
    if (out.cells > r.options.limits.maxCells || out.points.size() > r.options.limits.maxVertices)
        fail("planning_limit", "Planned patch/source counts exceed the job limit.",
             RunStatus::ResourceLimit);
    work.memoryFor({{out.cells, 192}, {out.points.size(), 256}});
    return out;
}
}
