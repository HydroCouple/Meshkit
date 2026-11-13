// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "geometry.hpp"
#include "events.hpp"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace meshkit
{
/// Zero-based index into a native vertex, cell or edge array.
using Index = std::uint32_t;
/**
 * @brief Sentinel index (all bits set, 4294967295) meaning "no index".
 *
 * It is, for example, the default JobException::cell. A mesh must hold fewer vertices than
 * this value; validateMesh() reports a larger vertex array as a resource limit.
 */
constexpr Index missingIndex = ~Index(0);
/**
 * @brief Role of a Path in a JobRequest or a delivered Mesh.
 *
 * runJob() closes Outer, Hole and Region rings implicitly; a Line stays open unless
 * Path::closed is set. runJob() and validateMesh() reject values above Region.
 */
enum class PathKind : std::uint8_t { Outer, Hole, Line, Region };
/// @var PathKind::Outer
/// @brief Outer domain ring; the domain is the union of all outer rings minus the holes.
/// @var PathKind::Hole
/// @brief Ring subtracted from the domain; cells inside it are removed.
/// @var PathKind::Line
/// @brief Constrained polyline (breakline) whose segments become chains of cell edges.
/// @var PathKind::Region
/// @brief Closed constrained ring inside the domain; it separates region-label components
///        but does not add or remove domain area.

/**
 * @brief Indexed polyline or ring: a source constraint of a JobRequest, or a delivered path of
 * a Mesh.
 *
 * In a JobRequest the indices refer to JobRequest::points; in a Mesh they refer to
 * Mesh::vertices. runJob() collapses consecutive duplicate vertices (after merging exactly
 * coincident points), requires a closed ring to keep at least three distinct vertices without
 * revisiting one, and delivers each path as the chain of output vertices along its cell edges,
 * without repeating the first vertex of a closed path.
 */
struct Path {
    std::vector<Index> vertices; ///< Zero-based vertex indices; at least two are required.
    PathKind kind = PathKind::Line; ///< Role of the path; defaults to an open Line.
    /**
     * @brief Numeric marker; runJob() copies it to every delivered Edge along this path. Default 0.
     */
    std::int32_t marker = 0;
    /**
     * @brief Caller identifier. In a delivered mesh runJob() replaces 0 with the path's one-based
     * position in JobRequest::paths.
     */
    std::int64_t id = 0;
    /**
     * @brief Explicit closure of a Line path. Other kinds are closed implicitly, and runJob() sets
     * this to true on every delivered path that is closed.
     */
    bool closed = false;
};
/**
 * @brief Triangle or quadrilateral cell of a Mesh.
 *
 * validateMesh() requires every used corner index to be inside Mesh::vertices and the corners
 * to form a strictly convex, counter-clockwise polygon in the XY plane.
 */
struct Cell {
    /**
     * @brief Corner vertex indices in counter-clockwise order; only the first #count entries are
     * used (runJob() writes 0 to the unused fourth entry of a triangle).
     */
    std::array<Index, 4> vertices{};
    /**
     * @brief Region identifier, named by the Label with the same id when one exists; 0 when no
     * region was assigned.
     */
    std::int32_t region = 0;
    std::uint8_t count = 3; ///< Number of corners: 3 (triangle) or 4 (quadrilateral).
};
/** @brief Marked edge between two vertices of a Mesh. */
struct Edge {
    /**
     * @var a
     * @brief First endpoint, a zero-based index into Mesh::vertices.
     * @var b
     * @brief Second endpoint, a zero-based index into Mesh::vertices; validateMesh() requires
     * it to differ from #a.
     */
    Index a = 0, b = 0;
    /**
     * @brief Numeric marker; runJob() writes the Path::marker of the source path (or the marker of
     * a patch line). Default 0.
     */
    std::int32_t marker = 0;
};
/** @brief Name of a region identifier used by Cell::region. */
struct Label {
    std::int32_t id = 0; ///< Region id; must be unique among the labels of a mesh or request.
    std::string name;    ///< Region name as UTF-8 text.
};
/**
 * @brief Extra source identity merged into a shared output vertex.
 *
 * runJob() merges source points with exactly equal XY into one vertex. The lowest-index such
 * point supplies the vertex's own id and marker; every other one is recorded as an alias.
 */
struct VertexAlias {
    Index vertex = 0;          ///< Zero-based index of the shared vertex in Mesh::vertices.
    std::int64_t sourceId = 0; ///< External id of the merged source point.
    std::int32_t marker = 0;   ///< Marker of the merged source point.
};
/**
 * @brief Physical units, coordinate reference text and description of a mesh or job request.
 *
 * MeshKit never converts coordinates between reference systems; the strings are carried
 * through unchanged. runJob() and validateMesh() require both unit scales to be positive and
 * finite and both strings to be valid UTF-8, and count the string bytes against
 * Limits::maxMetadataBytes.
 */
struct Metadata {
    /**
     * @var horizontalMetresPerUnit
     * @brief Metres per XY plane unit, the unit of point and vertex x/y; default 1.
     * @var verticalMetresPerUnit
     * @brief Metres per Z (elevation) unit; default 1. runJob() scales elevation differences by
     * verticalMetresPerUnit / horizontalMetresPerUnit when it ranks facet normals for
     * resolution-aware coarsening.
     */
    double horizontalMetresPerUnit = 1, verticalMetresPerUnit = 1;
    /**
     * @brief Whether the unit scales are declared; default true. runJob() refuses a request with
     * false (code "units_required"); the SMS 2DM reader sets false. validateMesh() ignores it.
     */
    bool unitsKnown = true;
    /**
     * @var crs
     * @brief Coordinate reference system text (UTF-8), carried through unchanged; may be empty.
     * @var description
     * @brief Free-form description (UTF-8); may be empty.
     */
    std::string crs, description;
};
// Geometry and sparse numeric attributes only; no engine/coupling ownership.
// Empty id/marker arrays mean sequential one-based ids / zero markers. Arrays
// are owned and contiguous. Connectivity is zero-based uint32 throughout.
/**
 * @brief Neutral owned mesh: XYZ vertices, triangle/quad cells, marked edges, paths, optional
 * external ids and markers, region labels, source aliases and metadata.
 *
 * X and Y are in XY plane units and Z in elevation units (see Metadata). Each optional
 * per-item array is either empty or exactly as long as the array it annotates; validateMesh()
 * checks this.
 */
struct Mesh {
    std::vector<Point3> vertices; ///< Vertex positions; every coordinate must be finite.
    std::vector<Cell> cells;      ///< Triangle and quadrilateral cells.
    /**
     * @brief Marked constraint edges, not every cell edge. runJob() fills it with the cell-edge
     * chains of delivered paths and the interior lines of quad patches.
     */
    std::vector<Edge> edges;
    /**
     * @brief Delivered paths. runJob() also delivers the rings of planned quad regions as Region
     * paths with negative ids.
     */
    std::vector<Path> paths;
    /**
     * @var vertexIds
     * @brief External vertex ids: empty means 1, 2, 3, ...; otherwise one unique id per vertex.
     * @var cellIds
     * @brief External cell ids: empty means one-based sequential ids; otherwise one unique id
     * per cell.
     * @var edgeIds
     * @brief External edge ids: empty means one-based sequential ids; otherwise one unique id
     * per edge.
     */
    std::vector<std::int64_t> vertexIds, cellIds, edgeIds;
    /// Numeric vertex markers: empty means all zero; otherwise one marker per vertex.
    std::vector<std::int32_t> vertexMarkers;
    std::vector<Label> labels; ///< Names of the region ids used by Cell::region; ids are unique.
    /** @brief Further source identities merged into shared vertices (exact XY duplicates). */
    std::vector<VertexAlias> aliases;
    Metadata metadata; ///< Units, CRS text and description.
};
/**
 * @brief Resource caps for jobs, validation and file I/O.
 *
 * The caps bound counts, estimated workspace and counted work units, not total process memory
 * or time spent inside caller callbacks. Reaching one generally ends the operation with
 * RunStatus::ResourceLimit and no partial result; patch planning may instead decline a
 * preferred quad patch that would exceed the cell or workspace cap.
 */
struct Limits {
    /**
     * @var maxVertices
     * @brief Maximum number of vertices (source points, working vertices during refinement,
     * mesh vertices, and also vertex aliases); default 20 000 000.
     * @var maxCells
     * @brief Maximum number of cells; default 20 000 000. runJob() refuses 0.
     * @var maxEdges
     * @brief Maximum number of edges, of paths, and of vertex indices summed over all paths;
     * default 40 000 000.
     */
    std::uint64_t maxVertices = 20000000, maxCells = 20000000, maxEdges = 40000000;
    /// Cap on estimated workspace in bytes (computed from counts, not measured); default 4 GiB.
    std::uint64_t maxWorkspaceBytes = std::uint64_t(4) << 30;
    /**
     * @brief Work-unit allowance of one operation's own counter. runJob() also passes it as the
     * separate budget of patch planning and of the final constraint audit. Zero selects
     * 20 000 000 000 units for the operation's counter; planning and the audit then derive their
     * budgets from their input sizes.
     */
    std::uint64_t maxWork = 0; // Zero: 20 billion for the operation's own counter (above).
    /**
     * @var maxMetadataBytes
     * @brief Cap on metadata text bytes (CRS, description, label names and, for a job, the
     * provider identity); the label count is also capped at a quarter of it. Default 8 MiB.
     * @var maxRecordBytes
     * @brief Cap on the length in bytes of one SMS 2DM text line; default 1 MiB.
     */
    std::uint64_t maxMetadataBytes = 8 * 1024 * 1024, maxRecordBytes = 1024 * 1024;
};
/**
 * @brief Versioned outcome of a native operation: mesh validation, file I/O, or a job (as the
 * base of JobReport).
 */
struct OperationReport {
    std::uint32_t schemaVersion = 1; ///< Report schema version; 1 in this release.
    RunStatus status = RunStatus::Failed; ///< Final status; Failed until the operation sets it.
    /**
     * @var code
     * @brief Short machine-readable outcome code, e.g. "valid", "complete", "units" or
     * "cancelled".
     * @var message
     * @brief Human-readable description of the outcome.
     */
    std::string code, message;
    Counts counts; ///< Vertex and cell counters of the operation; -1 where unavailable.
    /**
     * @var work
     * @brief Work units used by the operation's own counter (see Limits::maxWork).
     * @var record
     * @brief Record reached by a file reader (the SMS 2DM line number, or the number of MeshKit
     * binary records read), which locates a read error; 0 for operations that read no records.
     */
    std::uint64_t work = 0, record = 0;
    double seconds = 0; ///< Elapsed time of the operation in seconds (steady clock).
    /**
     * @brief Whether the operation delivered a result.
     * @return true for RunStatus::Complete and RunStatus::CompleteWithExceptions.
     */
    bool complete() const noexcept
    {
        return status == RunStatus::Complete || status == RunStatus::CompleteWithExceptions;
    }
};
// Local connectivity/finite/positive convex-cell validation. It does not prove
// arbitrary nonincident nonoverlap or CRS correctness. Caller owns metadata.
/**
 * @brief Check a mesh's local validity without modifying it.
 *
 * Checks, in order: counts against the limits, positive finite unit scales, UTF-8 metadata,
 * unique label ids, id and marker array lengths, unique ids, finite coordinates, cells with 3
 * or 4 in-range corners forming a strictly convex counter-clockwise polygon (exact
 * orientation tests), edges with distinct in-range endpoints, paths with a known kind and at
 * least two in-range vertices, and in-range alias vertices. Metadata::unitsKnown, path
 * closure, duplicate edges and overlap between non-adjacent cells are not checked. The first
 * failure ends validation. Synchronous; nothing is retained after return.
 *
 * @param[in] mesh Mesh to check; borrowed and only read.
 * @param[in] limits Caps on counts, metadata bytes and work units (default Limits{}).
 * @param[in] cancelled Optional callback, polled on entry and then every 4096 work units;
 *            returning true ends validation with RunStatus::Cancelled.
 * @return Report with status Complete, code "valid", the vertex/triangle/quad counts and the
 *         work used on success. Otherwise Infeasible for invalid data, ResourceLimit for an
 *         exceeded cap or failed allocation, Cancelled, or Failed (code "exception") for any
 *         other std::exception; never CompleteWithExceptions.
 */
OperationReport validateMesh(const Mesh &, const Limits & = {},
                             const std::function<bool()> &cancelled = {});
}
