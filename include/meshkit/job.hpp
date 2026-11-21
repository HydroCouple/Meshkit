// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "mesh.hpp"
#include "terrainpolicy.hpp"
#include "planning.hpp"
#include "sizefield.hpp"
#include <optional>
#include <functional>

namespace meshkit
{
/** @brief Cell types a job may deliver (JobOptions::cells). */
enum class CellPlan : std::uint8_t { Triangles, AutomaticMixed };
/// @var CellPlan::Triangles
/// @brief Triangles only. Quad regions still add their outlines and spacing caps but get no
///        quads; fixed patches are rejected (code "quad_conflict").
/// @var CellPlan::AutomaticMixed
/// @brief Jointly planned mapped quad patches (plus optional automatic blocks) with triangle
///        infill.

/**
 * @brief Point that assigns a region id to the constrained component containing it.
 *
 * After meshing, runJob() groups cells into components separated by delivered constraint
 * edges and gives every cell of the seed's component #region. Seeds or quad region attributes
 * that give one component different ids fail the job (code "region_conflict").
 */
struct RegionSeed {
    /**
     * @brief Seed position in XY plane units; must be finite, inside the delivered domain and not
     * on a delivered constraint edge.
     */
    Point2 point;
    std::int32_t region = 0; ///< Region id given to every cell of the component.
};
/**
 * @brief Meshing policy of a JobRequest.
 *
 * Sizes and edge lengths are in XY plane units; elevations and terrain tolerances are in Z
 * units (see Metadata). runJob() checks the values first and fails with code "option" (or
 * "size_range") when one is out of range. The nominal size of a triangle is the side of the
 * equilateral triangle of equal area; that of a quadrilateral is the square root of its area.
 */
struct JobOptions {
    CellPlan cells = CellPlan::Triangles; ///< Cell types to deliver; default triangles only.
    /**
     * @var minimumTarget
     * @brief Advisory minimum nominal cell size, finite and >= 0; 0 (default) disables it.
     * Smaller delivered cells are counted in JobReport::belowMinimum (CompleteWithExceptions)
     * but never refused. It must not exceed a positive maximumSize. It also sets the spacing
     * floor of terrain points, and a quarter of it is an edge length below which refinement
     * stops splitting for angle quality; when 0, maximumSize / 4 takes its place there.
     * @var maximumSize
     * @brief Global upper bound on nominal cell size, finite and >= 0; 0 (default) means no
     * global bound. Combined with Providers::maximumSize and quad region spacing by taking the
     * smallest positive value; larger cells are counted in JobReport::sizeConflicts.
     * @var minimumEdge
     * @brief Minimum actual edge length, finite and >= 0; 0 (default) disables it. Shorter
     * cell sides are counted in JobReport::edgeConflicts; refinement also stops splitting for
     * angle quality below it.
     * @var minimumAngleDegrees
     * @brief Smallest triangle angle sought by refinement, in degrees from 0 to 34; default
     * 30. Also bounds the corner angles of planned quad patches.
     */
    double minimumTarget = 0, maximumSize = 0, minimumEdge = 0, minimumAngleDegrees = 30;
    /**
     * @var adjacentRatio
     * @brief Largest allowed ratio between the nominal sizes of two cells sharing a side: 0
     * (default) disables it, otherwise it must be >= 1. Exceeding sides are counted in
     * JobReport::transitionConflicts, and sizing repair refines towards it.
     * @var flatElevation
     * @brief Elevation (Z units, finite; default 0) given to vertices without a finite
     * authored height when Providers::elevation is not set.
     */
    double adjacentRatio = 0, flatElevation = 0;
    /**
     * @var smoothingPasses
     * @brief Passes of non-degrading smoothing of free vertices after size and angle
     * refinement, 0 to 10; default 0 (off).
     * @var coarsenPasses
     * @brief Maximum coarsening (removal) attempts per free vertex, i.e. one inserted by
     * refinement and not on a constraint, 0 to 10; default 0 (off).
     * @var sizingRepairPasses
     * @brief Bounded sizing-repair passes after smoothing, terrain insertion and coarsening,
     * 0 to 3; default 2.
     */
    int smoothingPasses = 0, coarsenPasses = 0, sizingRepairPasses = 2;
    /**
     * @brief Cap on vertices inserted by sizing repair; default 200 000. Reaching it ends the job
     * with RunStatus::ResourceLimit (code "refinement_limit"), so 0 does so whenever
     * sizingRepairPasses is positive.
     */
    std::uint32_t repairInsertions = 200000;
    /// Order in which coarsening tries vertices; default CoarseningOrder::LegacyFlatness.
    CoarseningOrder coarseningOrder = CoarseningOrder::LegacyFlatness;
    /**
     * @var coarseningReferenceSize
     * @brief Positive finite size (XY units; default 10) that resolution-aware coarsening uses
     * as the target where no size bound applies.
     * @var terrainTolerance
     * @brief Reference terrain tolerance in Z units, finite and >= 0; 0 (default) disables
     * terrain checks. When positive, Providers::elevation, Providers::terrainWitness and
     * Providers::terrainBound are all required, and #terrainPolicy derives each footprint's
     * tolerance from it.
     */
    double coarseningReferenceSize = 10, terrainTolerance = 0;
    /**
     * @brief Footprint tolerance policy; must be TerrainTolerancePolicy::valid(), and its
     * TargetSize mode needs a positive terrainTolerance.
     */
    TerrainTolerancePolicy terrainPolicy;
    /**
     * @var strictSizing
     * @brief When true (default), remaining size, edge or transition conflicts fail the job
     * (code "unresolved_size", Infeasible) instead of returning CompleteWithExceptions; with
     * quad patches, transition conflicts first trigger up to three planning retries. Cells
     * below minimumTarget never fail the job.
     * @var requireTerrainCertification
     * @brief When true, any terrain conflict or unknown terrain bound fails the job (code
     * "unverified_terrain", Infeasible); it requires a positive terrainTolerance. Default
     * false.
     * @var retainInputMapping
     * @brief When true (default), JobResult::inputVertexToOutput is filled.
     */
    bool strictSizing = true, requireTerrainCertification = false, retainInputMapping = true;
    /**
     * @brief Most cells one mapped patch may hold; default 250 000. A larger preferred patch is
     * declined; a larger required patch fails the job with RunStatus::ResourceLimit. It also scales
     * the tiles of automatic blocks.
     */
    std::uint64_t maximumPatchCells = 250000;
    /**
     * @brief Whether patch planning also places automatic open-area quad blocks; default false.
     * Takes effect only when planning runs (cells is AutomaticMixed, or quad regions or fixed
     * patches are given).
     */
    bool automaticBlocks = false;
    int maximumAutomaticBlocks = 4096; ///< Cap on automatic block tiles; default 4096.
    Limits limits; ///< Resource caps of the job.
};
/**
 * @brief Complete input of runJob(): source geometry, optional attributes, sizing inputs,
 * metadata and options.
 *
 * runJob() only reads it. Coordinates are XY plane units of one Cartesian frame. Each
 * optional per-point array is either empty or exactly as long as #points.
 */
struct JobRequest {
    /**
     * @brief Mandatory source points: at least three, at most Limits::maxVertices. A point outside
     * the delivered domain or inside a hole fails the job (code "unused_source_point"). Exactly
     * equal XY points share one output vertex (see VertexAlias).
     */
    std::vector<Point2> points;
    /**
     * @brief Source paths indexing #points; at least one PathKind::Outer path is required (no
     * bounding box is inferred). Crossing paths must be noded by the caller (NATIVE_API.md).
     */
    std::vector<Path> paths;
    /// Optional authored Z per point; NaN samples the elevation provider, or uses
    /// JobOptions::flatElevation when there is none.
    std::vector<double> pointElevations;
    /**
     * @brief Positive, unique external ids of #points; empty means 1, 2, 3, ... Generated vertices
     * get ids above the largest source id.
     */
    std::vector<std::int64_t> pointIds;
    /**
     * @brief Markers of #points, delivered as vertex markers; empty means all zero. With
     * #sizeField, points with a nonzero marker also seed the size field.
     */
    std::vector<std::int32_t> pointMarkers;
    /// Region names copied to Mesh::labels; ids must be unique and names UTF-8.
    std::vector<Label> labels;
    std::vector<RegionSeed> regions; ///< Seeds that assign region ids to constrained components.
    /**
     * @brief Quad region policies (see plan::QuadRegion). A positive spacing also caps the size of
     * every cell whose footprint meets the region's bounding box; the attribute becomes the
     * Cell::region of the region's quads.
     */
    std::vector<plan::QuadRegion> quadRegions;
    /**
     * @brief Local quad patches inserted as given after validation; every cell center must lie in
     * the domain. Rejected when options.cells is CellPlan::Triangles.
     */
    std::vector<plan::PatchMesh> fixedPatches;
    /**
     * @brief Optional graded size field built from the source paths and marked points; its bound is
     * combined with Providers::maximumSize (smallest positive value). footprintBounds is forced on.
     */
    std::optional<plan::SizeFieldOptions> sizeField;
    Metadata metadata; ///< Units, CRS text and description; copied to the delivered mesh.
    JobOptions options; ///< Meshing policy and limits.
};
/**
 * @brief Optional caller callbacks used by runJob(), plus the provider identity.
 *
 * runJob() copies this object and calls the callbacks synchronously on the calling thread.
 * Callbacks may be empty, except that a positive JobOptions::terrainTolerance requires
 * #elevation, #terrainWitness and #terrainBound. An exception thrown by a callback ends the
 * job without a mesh (normally with RunStatus::Failed). Pointer arguments are valid only
 * during the call.
 */
struct Providers {
    // Immutable, synchronous native callbacks on the job thread. They must not
    // reenter the job or mutate request arrays. Exceptions end the job without a mesh:
    // std::bad_alloc as ResourceLimit, other exceptions as Failed.
    // Footprint bounds include every prescribed source, including narrow strips.
    /**
     * @brief Upper bound on nominal cell size (XY units) over a whole footprint box. It must return
     * a finite value >= 0, otherwise the job fails (code "size_provider"); 0 means no limit.
     */
    std::function<double(const Box2 &)> maximumSize;
    /**
     * @brief Elevation (Z units) at XY position (x, y). Used for every delivered vertex without a
     * finite authored height (JobOptions::flatElevation when empty); a nonfinite result for a
     * delivered vertex fails the job (code "elevation").
     */
    std::function<double(double, double)> elevation;
    /**
     * @brief Refinement witness: given three XY corners and their heights, returns the triangle's
     * residual against the reference surface (Z units) and writes the point where it occurs to the
     * third argument; NaN means unknown coverage.
     */
    std::function<double(const Point2 *, const double *, Point2 *)> terrainWitness;
    /**
     * @brief Certified maximum residual (Z units) of the linear triangle over three XY corners with
     * the given heights, coverage included. Used by coarsening, patch planning and the final check,
     * which tests a quad as two triangles and counts a nonfinite or negative result as unknown.
     */
    std::function<double(const Point2 *, const double *)> terrainBound;
    /// Polled during the job; returning true stops it with RunStatus::Cancelled.
    std::function<bool()> cancelled;
    std::string identity; ///< Caller supplied dataset/provider version or fingerprint.
};
/** @brief One requirement exception recorded by runJob() as a bounded example. */
struct JobException {
    /// Kind of exception: "size", "edge", "transition", "terrain" or "terrain_unknown".
    std::string code;
    /**
     * @brief Index of the cell in the job's output Mesh::cells (for "transition", the lower of the
     * two cell indices); missingIndex by default.
     */
    Index cell = missingIndex;
    /**
     * @var measured
     * @brief Measured value: nominal size or edge length (XY units), size ratio, or terrain
     * bound (Z units); 0 for "terrain_unknown".
     * @var allowed
     * @brief Limit it was compared with: the size bound, JobOptions::minimumEdge,
     * JobOptions::adjacentRatio, or the footprint terrain tolerance.
     */
    double measured = 0, allowed = 0;
};
/**
 * @brief Report of runJob(): the OperationReport fields plus options, provider identity,
 * planning, refinement and certification statistics.
 *
 * Counters are totals; #examples keeps at most 64 individual exceptions. When a job stops
 * early, the fields keep the values reached so far.
 */
struct JobReport : OperationReport {
    std::string algorithmVersion = "cdt-1/job-2"; ///< Algorithm and report version identifier.
    /**
     * @brief Copy of the options of the returned attempt as supplied (zero defaults such as
     * Limits::maxWork are not expanded); shows automaticBlocks off if a retry disabled it.
     */
    JobOptions resolvedOptions;
    std::string providerIdentity; ///< Copy of Providers::identity.
    /** @brief Whether the independent audit of the delivered source constraints passed. */
    bool constraintAuditPassed = false;
    std::uint64_t constraintSegments = 0; ///< Source segments checked by that audit.
    /**
     * @var plannedPatches
     * @brief Quad patches (fixed and planned) prepared for the returned attempt.
     * @var sharedSides
     * @brief Patch sides shared between planned patches.
     * @var planningRetries
     * @brief Mixed-interface retries before the returned attempt, 0 to 3.
     */
    std::uint64_t plannedPatches = 0, sharedSides = 0, planningRetries = 0;
    /**
     * @brief Patch planning decisions of the returned attempt followed by those of earlier retries;
     * at most 256.
     */
    std::vector<plan::PatchDecision> planning;
    /**
     * @var inserted
     * @brief Vertices inserted by refinement, segment splits included.
     * @var removed
     * @brief Vertices removed by coarsening.
     * @var removalAttempts
     * @brief Coarsening removal attempts.
     * @var queuePeak
     * @brief Peak size of the coarsening priority queue.
     */
    std::uint64_t inserted = 0, removed = 0, removalAttempts = 0, queuePeak = 0;
    /**
     * @var belowMinimum
     * @brief Cells whose nominal size is below JobOptions::minimumTarget (advisory).
     * @var edgeConflicts
     * @brief Cell sides shorter than JobOptions::minimumEdge, counted per cell (a shared side
     * counts twice).
     * @var sizeConflicts
     * @brief Cells whose nominal size exceeds the size bound over their bounding box.
     * @var transitionConflicts
     * @brief Shared sides whose cell size ratio exceeds JobOptions::adjacentRatio.
     */
    std::uint64_t belowMinimum = 0, edgeConflicts = 0, sizeConflicts = 0, transitionConflicts = 0;
    /**
     * @var terrainConflicts
     * @brief Cells whose certified terrain bound exceeds their footprint tolerance.
     * @var terrainUnknown
     * @brief Cells whose terrain bound is unknown (nonfinite or negative).
     */
    std::uint64_t terrainConflicts = 0, terrainUnknown = 0;
    /**
     * @var largestAdjacentRatio
     * @brief Largest nominal-size ratio over all shared sides; 1 when none was measured.
     * @var largestTerrainRatio
     * @brief Largest terrain bound / tolerance over cells with a known bound; 0 when terrain
     * is not checked.
     */
    double largestAdjacentRatio = 1, largestTerrainRatio = 0;
    /// Up to 64 individual exceptions; the counters above hold the totals.
    std::vector<JobException> examples; // At most 64, never one object per cell.
};
/** @brief Owned output of runJob(). */
struct JobResult {
    Mesh mesh; ///< Delivered mesh; empty unless report.complete() is true.
    /**
     * @brief Output vertex index of each JobRequest::points entry when
     * JobOptions::retainInputMapping is set; empty otherwise or when the job did not complete.
     */
    std::vector<Index> inputVertexToOutput;
    JobReport report; ///< Status, counters and diagnostics of the job.
};
/**
 * @brief Feature flags of the native backend, as returned by nativeCapabilities().
 *
 * Every flag is true in this release; MeshKit itself does not read them.
 */
struct Capabilities {
    /**
     * @var qtFree
     * @brief The native core and I/O need no Qt.
     * @var triangles
     * @brief Triangle-only jobs (CellPlan::Triangles) are supported.
     * @var automaticMixed
     * @brief Mixed triangle/quad jobs (CellPlan::AutomaticMixed) are supported.
     */
    bool qtFree = true, triangles = true, automaticMixed = true;
    /**
     * @var terrainProviders
     * @brief Terrain provider callbacks (see Providers) are supported.
     * @var coarsening
     * @brief Coarsening (JobOptions::coarsenPasses) is supported.
     * @var streamingIO
     * @brief Streaming mesh file I/O is supported.
     */
    bool terrainProviders = true, coarsening = true, streamingIO = true;
};
/**
 * @brief Feature flags of this native build.
 * @return A default-constructed Capabilities, in which every flag is true.
 */
constexpr Capabilities nativeCapabilities()
{
    return {};
}
// Borrowed immutable inputs for this synchronous call; owned result buffers.
// No mesh is returned on infeasible/cancelled/resource/failed jobs. The native
// backend supports triangles and jointly planned mapped/automatic mixed meshes.
// CompleteWithExceptions is distinct from a certified result. No CRS conversion,
// DEM access, Python callbacks, or file publication occurs inside runJob.
/**
 * @brief Run one meshing job synchronously on the calling thread.
 *
 * Checks the request, plans quad patches when requested, builds and refines the constrained
 * Delaunay triangulation, assembles the mesh, validates it with validateMesh(), and audits
 * constraints, sizes, transitions and terrain. When a planned patch interface does not
 * conform, or (with strict sizing and quad patches present) a transition exceeds
 * JobOptions::adjacentRatio, it retries with the region's spacing halved (required region),
 * the region meshed as triangles (preferred region) or automatic blocks disabled, at most
 * three times. Independent jobs may run on independent threads.
 *
 * @param[in] original Source geometry and options; borrowed and only read during the call.
 * @param[in] providers Callbacks and identity; copied, and called only on this thread
 *            during the call.
 * @param[in,out] events Optional channel (may be nullptr) that receives Stage and Progress
 *                events, a Diagnostic event per retry and one final Completed event. A channel
 *                ignores events after Completed, so use a new one per job.
 * @return Owned result. Complete and CompleteWithExceptions deliver a mesh; Infeasible,
 *         Cancelled, ResourceLimit and Failed return an empty mesh and mapping with the
 *         report. Exceptions raised during an attempt, including from callbacks, are caught
 *         and reported.
 */
JobResult runJob(const JobRequest &, const Providers & = {}, EventChannel *events = nullptr);
}
