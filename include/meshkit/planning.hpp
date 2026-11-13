#include "meshkit/regions.hpp"
#include "meshkit/patch.hpp"
#include "meshkit/detail/planmodel.hpp"
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <functional>

namespace meshkit::plan
{
/** @brief Outcome recorded for one quad region in a PatchDecision. */
enum class PatchDisposition { Triangles, Accepted, Repaired, RequiredFailure };
/// @var PatchDisposition::Triangles
/// @brief No mapped patch is delivered and the region is left to triangles (the default).
/// @var PatchDisposition::Accepted
/// @brief Mapped patch accepted without any repair round.
/// @var PatchDisposition::Repaired
/// @brief Mapped patch obtained after refinement: in a QuadPlan, accepted after at least one
/// station-doubling repair round; the native job also records it for a required region whose
/// spacing it halves before retrying.
/// @var PatchDisposition::RequiredFailure
/// @brief A required region could not be planned, so the whole plan was refused.

/** @brief Per-region record of what was decided about a mapped patch and why. */
struct PatchDecision {
    /// @var source
    /// @brief Region label. planQuadRegions() uses QuadRegion::tag, or "region:" followed by
    /// the 0-based region index when the tag is empty.
    /// @var reason
    /// @brief Explanation of the latest refusal, repair or acceptance; may be empty when
    /// planning stopped before the region was decided.
    Text source, reason;
    int region = -1;       ///< 0-based index of the source region; -1 when unassigned.
    /// True for a hard requirement (QuadRegion::requireMapped or directionalSpacing), whose
    /// failure refuses the whole plan.
    bool required = false;
    PatchDisposition disposition = PatchDisposition::Triangles; ///< Outcome; Triangles by default.
    /// Cells (n x m) in the region's patch. Set when a planning round accepts the patch, so
    /// read it only for Accepted or Repaired outcomes.
    std::uint64_t cells = 0;
    /// Repair rounds or retries recorded. planQuadRegions() sets it only for a delivered patch,
    /// where nonzero means Repaired.
    int repairRounds = 0;
};
/**
 * @brief Acceptance limits and callbacks for one mapped patch; see assessPlannedPatch().
 *
 * Scalar limits other than #alongAngleDegrees must be finite and >= 0, and 0 disables a check.
 * Lengths are in XY units, angles in degrees and terrain errors in Z units. Empty callbacks
 * disable their checks except where noted. Callbacks run synchronously on the calling thread.
 */
struct PatchLimits {
    /// @var minimumEdge
    /// @brief Shortest permitted cell edge, XY units; 0 disables. Failing it is not repairable.
    /// @var minimumAngleDegrees
    /// @brief Smallest permitted interior angle (quadQuality() minAngleDeg), degrees; must be
    /// < 180, and 0 disables. Failing it is repairable.
    /// @var maximumAspect
    /// @brief Largest permitted quadQuality() aspect; 0 disables. Failing it is repairable.
    /// planQuadRegions() replaces it with QuadRegion::aspectMax when that is >= 0.
    double minimumEdge = 0, minimumAngleDegrees = 0, maximumAspect = 0;
    /// @var alongSize
    /// @brief Largest mean length, XY units, of the opposite cell sides most aligned with
    /// #alongAngleDegrees. Both this and #acrossSize > 0 enables the directional check, both 0
    /// disables it, and only one > 0 is invalid. Failing it is repairable.
    /// @var acrossSize
    /// @brief Largest mean length, XY units, of the other pair of opposite cell sides; see
    /// #alongSize.
    /// @var alongAngleDegrees
    /// @brief Along-axis direction, degrees counterclockwise from +x. Must be finite even when
    /// the directional check is off.
    double alongSize = 0, acrossSize = 0, alongAngleDegrees = 0;
    /// Constant terrain error tolerance, Z units, used when #terrainToleranceForFootprint is
    /// empty. 0 together with an empty footprint callback disables the terrain check.
    double terrainTolerance = 0;
    /// Tolerance, Z units, for a cell's XY bounding box. When set it replaces
    /// #terrainTolerance and enables the terrain check; a result that is not finite and > 0
    /// fails the patch (not repairable).
    std::function<double(const Box2 &)> terrainToleranceForFootprint;
    /// Maximum cell size, XY units, for an XY bounding box, compared with sqrt(cell area). 0
    /// means no cap; negative or non-finite fails (not repairable); exceeding it is repairable.
    /// planQuadRegions() also calls it with a region's bounding box to shrink initial spacing.
    std::function<double(const Box2 &)> maximumSize;
    /// Ground elevation, Z units, at (x, y) in XY units. Required when the terrain check is
    /// enabled; a non-finite value means unknown coverage and fails (not repairable).
    std::function<double(double, double)> elevationAt;
    /// Certified maximum terrain error, Z units, of the planar triangle through three points.
    /// The arguments point to 3 XY positions and their 3 #elevationAt values, valid only during
    /// the call; each quad is tested as triangles (0,1,2) and (0,2,3). Required when the
    /// terrain check is enabled; a negative or non-finite result means unknown and fails (not
    /// repairable).
    std::function<double(const Point2 *, const double *)> terrainBound;
    /// Polled before the first cell and then every 1024 cells; returning true stops with
    /// PatchAssessment::cancelled. planQuadRegions() also polls it to cancel planning.
    std::function<bool()> cancelled;
};
/** @brief Result of assessPlannedPatch(). */
struct PatchAssessment {
    /// @var ok
    /// @brief True when every cell passed every enabled check.
    /// @var repairable
    /// @brief True when more stations could cure the failure: a shape, directional spacing,
    /// footprint size or terrain error limit was exceeded. False on success and for invalid
    /// limits, geometry or callback results, or unknown terrain.
    /// @var cancelled
    /// @brief True when PatchLimits::cancelled stopped the assessment (#ok is then false).
    bool ok = false, repairable = false, cancelled = false;
    Text reason; ///< Failure description; empty when #ok is true.
};
/**
 * @brief Check a mapped patch cell by cell against shape, size and terrain limits.
 *
 * The first argument is the patch, of which only PatchMesh::xy and PatchMesh::quads are read
 * (boundary and topology are not validated; see validate(const PatchMesh &)). The second holds
 * the limits and callbacks. Checking stops at the first failing cell. Each cell must be a
 * quadrilateral with in-range, finite vertices that is convex with positive area in either
 * winding; then, where enabled, the minimum edge, minimum angle, maximum aspect, directional
 * spacing, footprint size and terrain error limits are tested with a small relative tolerance
 * (about 1e-9). Callbacks run synchronously on the calling thread.
 * @return The assessment. PatchAssessment::ok is false for invalid limits (a scalar negative or
 * non-finite, minimumAngleDegrees >= 180, or only one of alongSize and acrossSize positive), an
 * empty patch, a failing cell or cancellation.
 */
PatchAssessment assessPlannedPatch(const PatchMesh &, const PatchLimits &);
/** @brief Settings, budgets and patch limits for planQuadRegions(). */
struct QuadPlanningOptions {
    /// False forces triangles everywhere: optional regions fall back and any required region
    /// fails the plan.
    bool allowQuads = true;
    /// Target spacing, XY units, for regions whose QuadRegion::spacing is not > 0; must be
    /// finite and > 0.
    double defaultSize = 1;
    /// Base patch limits. Per region, QuadRegion::aspectMax (when >= 0) replaces maximumAspect,
    /// a positive spacing caps maximumSize, and directional spacing sets alongSize, acrossSize
    /// and alongAngleDegrees. limits.cancelled also cancels planning, and limits.maximumSize
    /// also scales initial spacing.
    PatchLimits limits;
    /// Rounds (0..3) in which a repairable failure doubles the station counts of the
    /// candidate's sides; a value outside 0..3 makes the settings invalid.
    int maximumRepairRounds = 3;
    /// @var maximumCells
    /// @brief Cell budget for all planned patches together. It also caps one side's station
    /// count, sets the default #workBudget and bounds repair doubling.
    /// @var maximumPatchCells
    /// @brief Cell budget for a single patch.
    std::uint64_t maximumCells = 20000000, maximumPatchCells = 250000;
    /// @var maximumWorkspaceBytes
    /// @brief Estimated planning memory budget in bytes (about 160 per source segment, 32 per
    /// station and 192 per planned cell); 2 GiB by default.
    /// @var workBudget
    /// @brief Maximum work units; 0 means maximumCells * 512 + 100000. Running out stops
    /// planning with QuadPlan::resourceLimited.
    std::uint64_t maximumWorkspaceBytes = std::uint64_t(2) << 30, workBudget = 0;
    /**
     * Optional filter called with each candidate's CCW boundary ring (no repeated closing
     * vertex); returning true refuses that candidate.
     */
    // Previous bounded conformity retry found this candidate unsafe.
    std::function<bool(const Polygon &)> excluded;
};
/**
 * @brief Source geometry that protects areas from mapped patches; XY units, finite coordinates.
 *
 * Any source vertex lying on a region side becomes a station of that side. A patch must lie
 * inside a domain and outside holes, triangles-only regions and fixed patches, and must not
 * cross or enclose a source line or point, or the ring of another region that is not an
 * automatic candidate.
 */
struct QuadPlanningGeometry {
    /// @var domains
    /// @brief Domain rings, closing vertex optional; a patch must lie inside one.
    /// @var holes
    /// @brief Hole rings, closing vertex optional; a patch may not lie inside one.
    Vector<Polygon> domains, holes;
    Vector<Vector<Point2>> lines; ///< Open protected polylines; patches may not cross or enclose.
    /// Protected points. A pin on a region side becomes a station; one inside a patch must
    /// coincide exactly with a patch vertex.
    Vector<Point2> pins;
    /// Existing patches whose validated boundaries block new ones; an invalid fixed patch
    /// contributes no boundary here.
    Vector<PatchMesh> fixedPatches;
};
/** @brief Result of planQuadRegions(); patches are delivered only when #ok is true. */
struct QuadPlan {
    /// @var ok
    /// @brief True when planning completed, even if every region fell back to triangles.
    /// @var cancelled
    /// @brief True when PatchLimits::cancelled stopped planning.
    /// @var resourceLimited
    /// @brief True when a work, workspace, cell or station budget stopped planning, including a
    /// required region refused for a resource reason.
    bool ok = false, cancelled = false, resourceLimited = false;
    Text error; ///< Why #ok is false; empty on success.
    /// Delivered patches with CCW cells, required regions first and then by region index;
    /// empty unless #ok.
    Vector<PatchMesh> patches;
    Vector<int> patchRegions; ///< Region index of each entry of #patches (parallel array).
    /// One decision per input region, indexed by region; kept, possibly incomplete, when #ok is
    /// false.
    Vector<PatchDecision> decisions;
    /// @var sharedSides
    /// @brief Region sides found identical (same coordinates, either direction) to a side of an
    /// earlier region. Counted before acceptance, so refused candidates are included.
    /// @var workUsed
    /// @brief Work units consumed; compare QuadPlanningOptions::workBudget.
    /// @var plannedCells
    /// @brief Total cells in #patches; 0 unless #ok.
    std::uint64_t sharedSides = 0, workUsed = 0, plannedCells = 0;
};
/**
 * @brief Plan mapped quad patches for all regions jointly, before CDT construction.
 *
 * The first argument lists the regions. Every region whose mode is not TrianglesOnly is a
 * four-sided mapped candidate, with corners from QuadRegion::corners or, when that is empty,
 * the ring vertices that turn by more than 30 degrees; exactly four, in strictly increasing
 * ring order, are required. Rings may wind either way and cells come out CCW. The second
 * argument is the protecting source geometry and the third the settings (defaults when
 * omitted). Spacing comes from
 * QuadRegion::spacing or QuadPlanningOptions::defaultSize, or from hAlong/hAcross assigned to
 * the side pairs by alignment with mappedAlongAngleDeg; PatchLimits::maximumSize can shrink it.
 * Sides that coincide exactly share one station sequence and opposite sides share one count.
 * A candidate that fails a repairable limit has its side counts doubled, for up to
 * QuadPlanningOptions::maximumRepairRounds rounds. When two patches meet along partial sides
 * with unmatched stations, the lower-priority one is refused instead of leaving a T-junction.
 *
 * An optional region that cannot be planned keeps PatchDisposition::Triangles. A required
 * region failure, cancellation, an exhausted budget, invalid settings or a non-finite source
 * coordinate makes QuadPlan::ok false and returns no patches. Callbacks run synchronously on
 * the calling thread; cancellation is polled at the start and every 4096 work units.
 * @return The plan; see QuadPlan.
 */
// Plan all region patches together before CDT construction. Full shared sides
// use one exact station sequence; opposite/shared counts are solved jointly.
// Authored vertices and indexed source junctions are retained. Unknown/infeasible
// preferred candidates use triangles; a required failure refuses the whole plan.
QuadPlan planQuadRegions(const Vector<QuadRegion> &, const QuadPlanningGeometry &,
                         const QuadPlanningOptions & = {});
/** @brief One patch interface edge whose two cells differ in size by more than allowed. */
struct PatchInterfaceConflict {
    /// @var first
    /// @brief Index into MeshResult::triangles of the lower-numbered cell on the edge.
    /// @var second
    /// @brief Index into MeshResult::triangles of the other cell on the edge.
    int first = -1, second = -1;
    /// @var firstSize
    /// @brief Nominal size of #first, XY units: sqrt(area) for a quad, the side of the
    /// equilateral triangle of equal area for a triangle.
    /// @var secondSize
    /// @brief Nominal size of #second, same measure.
    double firstSize = 0, secondSize = 0;
};
/** @brief Result of auditPatchInterfaces(). */
struct PatchInterfaceReport {
    /// @var ok
    /// @brief True when the audit completed; size violations do not clear it (see #violations).
    /// @var cancelled
    /// @brief True when the cancellation callback stopped the audit.
    bool ok = false, cancelled = false;
    Text error; ///< Why #ok is false; empty on success.
    /// @var sharedFaces
    /// @brief Listed edges with exactly two incident cells; edges with one cell are skipped.
    /// @var violations
    /// @brief Shared edges whose size ratio exceeds the allowed ratio (relative tolerance 1e-9).
    std::uint64_t sharedFaces = 0, violations = 0;
    double largestRatio = 1; ///< Largest size ratio over shared edges (>= 1); 1 when none.
    /// Up to maximumExamples (clamped to [0, 1000]) violations with the smallest
    /// (first, second) cell indices, sorted ascending.
    Vector<PatchInterfaceConflict> examples;
};
/**
 * @brief Check the size transition across delivered patch interface edges.
 *
 * Every cell is scanned once and must have positive (CCW) area, not only the cells on listed
 * edges; the nominal sizes of the two cells on each listed edge are compared. Storage grows
 * with the number of listed edges. The first two parameters are unnamed in this declaration;
 * the names below are the definition's.
 * @param[in] mesh The delivered mesh (MeshResult::ok is not read).
 * @param[in] boundaries Patch boundary edges as vertex index pairs into the mesh's vertices;
 * orientation does not matter and MeshEdge::marker and tag are ignored.
 * @param[in] maximumRatio Largest allowed ratio between the two nominal sizes; must be finite
 * and >= 1.
 * @param[in] cancelled Optional callback polled before the first cell and every 4096 cells;
 * returning true stops the audit with PatchInterfaceReport::cancelled.
 * @param[in] maximumExamples Number of violation examples to keep, clamped to [0, 1000].
 * @return The report. PatchInterfaceReport::ok is false for an invalid ratio, an out-of-range or
 * degenerate listed edge, invalid cell connectivity, a cell with non-positive or non-finite
 * area, an edge with more than two incident cells, a listed edge with no incident cell, or
 * cancellation.
 */
// Compact independent incidence/ratio check restricted to actual patch edges.
// Ordinary triangles are scanned once; storage grows with patch perimeter.
PatchInterfaceReport auditPatchInterfaces(const MeshResult &, const Vector<MeshEdge> &,
                                          double maximumRatio,
                                          const std::function<bool()> &cancelled = {},
                                          int maximumExamples = 64);
}
