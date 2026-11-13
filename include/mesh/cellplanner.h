// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshquadregion.h"
#include "meshpatch.h"
#include <functional>

namespace mesh
{
/** @enum PatchDisposition
 *  @brief Final outcome of one quad region in unified cell planning.
 *  @var PatchDisposition::Triangles
 *  @brief No patch is delivered; the region is meshed with triangles. The default.
 *  @var PatchDisposition::Accepted
 *  @brief The mapped patch passed every check without repair.
 *  @var PatchDisposition::Repaired
 *  @brief The mapped patch passed after its station counts were refined.
 *  @var PatchDisposition::RequiredFailure
 *  @brief A required region could not be planned; the whole plan fails. */
enum class PatchDisposition { Triangles, Accepted, Repaired, RequiredFailure };
/** @brief Per-region planning record explaining the disposition chosen by planQuadRegions(). */
struct PatchDecision {
    /** @var source
     *  @brief Region tag, or "region:<i>" with the zero-based index when the tag is empty.
     *  @var reason
     *  @brief English explanation: the acceptance note, the refusal or priority reason, or the
     *  last repairable failure. */
    QString source, reason;
    int region = -1; ///< Zero-based index into the planned regions; -1 when not assigned.
    bool required = false; ///< QuadRegion::requireMapped or directionalSpacing was set.
    PatchDisposition disposition = PatchDisposition::Triangles; ///< @brief Final outcome.
    /// @brief n x m cell count at the patch's last successful assessment; 0 if none passed. Not
    /// reset when a later shared-side check refuses the patch, so read it with disposition.
    quint64 cells = 0;
    int repairRounds = 0; ///< Station-doubling rounds the delivered patch needed; 0 otherwise.
};
/**
 * @brief Acceptance limits for one mapped patch, checked cell by cell by assessPlannedPatch().
 *
 * Lengths are XY lengths in the patch's planar units; terrain tolerances use the units of
 * elevationAt and terrainBound. Comparisons carry a small relative tolerance (1e-9, or 1e-10
 * for terrain error). Callbacks run synchronously on the calling thread.
 */
struct PatchLimits {
    /** @var minimumEdge
     *  @brief Shortest allowed cell edge; 0 disables. A violation is not repairable.
     *  @var minimumAngleDegrees
     *  @brief Smallest allowed interior angle in degrees, in [0, 180); 0 disables.
     *  @var maximumAspect
     *  @brief Largest allowed quad aspect ratio as computed by quadQuality(); 0 disables. */
    double minimumEdge = 0, minimumAngleDegrees = 0, maximumAspect = 0;
    /** @var alongSize
     *  @brief Limit on the mean length of the opposite edge pair best aligned with
     *  alongAngleDegrees; 0 disables the directional check. Zero exactly when acrossSize is.
     *  @var acrossSize
     *  @brief Limit on the mean length of the other opposite edge pair; 0 disables.
     *  @var alongAngleDegrees
     *  @brief Undirected along axis in degrees counterclockwise from +x; must be finite. */
    double alongSize = 0, acrossSize = 0, alongAngleDegrees = 0;
    /// @brief Constant terrain error tolerance, used when terrainToleranceForFootprint is unset.
    /// Terrain checks run only when it is positive or that callback is set.
    double terrainTolerance = 0;
    /// @brief Optional tolerance for a cell's XY bounding box; replaces terrainTolerance and
    /// must return a finite positive value.
    std::function<double(const QRectF &)> terrainToleranceForFootprint;
    /// @brief Optional size cap for a cell's XY bounding box, compared with sqrt(cell area);
    /// 0 means no cap, a negative or nonfinite value fails the patch.
    std::function<double(const QRectF &)> maximumSize;
    /// @brief Terrain elevation at (x, y), sampled at the quad corners; required by terrain
    /// checks. A nonfinite value means unknown coverage and fails the patch.
    std::function<double(double, double)> elevationAt;
    /// @brief Certified terrain error bound of the planar triangle through three XY points and
    /// heights (arrays valid only during the call); each quad is checked as triangles 0-1-2 and
    /// 0-2-3. Required by terrain checks; must return a finite value >= 0.
    std::function<double(const QPointF *, const double *)> terrainBound;
    /// Optional cancellation poll; planQuadRegions() also uses it as its cancellation source.
    std::function<bool()> cancelled;
};
/** @brief Result of assessPlannedPatch(). */
struct PatchAssessment {
    /** @var ok
     *  @brief Every cell met every limit.
     *  @var repairable
     *  @brief When ok is false: finer stations may fix the failure (angle, aspect, directional
     *  spacing, footprint size or terrain error exceeded). False for invalid limits or cells,
     *  a short edge, unknown terrain coverage or cancellation.
     *  @var cancelled
     *  @brief PatchLimits::cancelled returned true. */
    bool ok = false, repairable = false, cancelled = false;
    QString reason; ///< English reason for the first failure; empty when ok.
};
/**
 * @brief Check a generated mapped patch cell by cell against acceptance limits.
 *
 * The first argument is the patch, read only; it must have cells, and every cell must be a
 * quad with in-range vertex indices, finite coordinates and a convex, positive-area shape. The
 * second argument holds the limits, validated first: finite nonnegative values,
 * minimumAngleDegrees below 180, a finite alongAngleDegrees, and alongSize and acrossSize
 * either both positive or both zero. Checking stops at the first failing cell. Cancellation
 * is polled every 1024 cells; callbacks run synchronously on the calling thread.
 * @return Owned assessment; reason names the first failure.
 */
PatchAssessment assessPlannedPatch(const PatchMesh &, const PatchLimits &);
/** @brief Settings and budgets for planQuadRegions(). */
struct QuadPlanningOptions {
    bool allowQuads = true; ///< @brief False refuses all regions: global triangles-only priority.
    /// Spacing for regions without a positive QuadRegion::spacing; finite and positive.
    double defaultSize = 1;
    /// @brief Limits for every candidate patch; a region's aspectMax, isotropic spacing and
    /// directional spacing override them per region. Its callback is the cancellation poll.
    PatchLimits limits;
    /// Rounds that double station counts after repairable failures; valid range [0, 3].
    int maximumRepairRounds = 3;
    /** @var maximumCells
     *  @brief Cell budget over all planned patches; also bounds side subdivision counts.
     *  @var maximumPatchCells
     *  @brief Cell limit for a single patch. */
    quint64 maximumCells = 20000000, maximumPatchCells = 250000;
    /** @var maximumWorkspaceBytes
     *  @brief Estimated limit for the source index, stations and planned cells (2 GiB by
     *  default); not a bound on total process memory.
     *  @var workBudget
     *  @brief Work-unit limit; 0 selects 512 per maximumCells plus 100,000. */
    quint64 maximumWorkspaceBytes = quint64(2) << 30, workBudget = 0;
    // Previous bounded conformity retry found this candidate unsafe.
    /// Optional veto given a candidate's ordered patch boundary; returning true refuses it.
    std::function<bool(const QPolygonF &)> excluded;
};
/**
 * @brief Source geometry that protects against planned patches, in the regions' XY frame.
 * Coordinates must be finite. Source vertices lying on a region side become required stations.
 */
struct QuadPlanningGeometry {
    /** @var domains
     *  @brief Domain rings; a patch must lie inside one (tested at its first cell's centroid)
     *  and no domain edge may cross or enter it.
     *  @var holes
     *  @brief Hole rings; they take priority over any patch. */
    QVector<QPolygonF> domains, holes;
    /// Protected open polylines; they may run along a patch boundary but not enter or cross it.
    QVector<QVector<QPointF>> lines;
    /// Protected points; one inside or on a patch must coincide with a generated vertex.
    QVector<QPointF> pins;
    /// Existing patches; their boundaries (orderedPatchBoundary()) take priority like holes.
    QVector<PatchMesh> fixedPatches;
};
/** @brief Result of planQuadRegions(). On failure patches is empty; decisions are kept. */
struct QuadPlan {
    /** @var ok
     *  @brief Planning finished and every required region has a patch.
     *  @var cancelled
     *  @brief Cancellation was observed; ok is false.
     *  @var resourceLimited
     *  @brief A work, workspace or cell budget stopped the plan or refused a required region. */
    bool ok = false, cancelled = false, resourceLimited = false;
    QString error; ///< @brief English reason when ok is false; empty on success.
    /// Delivered patches with CCW quads, required regions first, then in region order.
    QVector<PatchMesh> patches;
    QVector<int> patchRegions; ///< Zero-based region index of each entry in patches.
    QVector<PatchDecision> decisions; ///< One decision per input region, in input order.
    /** @var sharedSides
     *  @brief Region sides that matched an already registered identical side path.
     *  @var workUsed
     *  @brief Work units consumed.
     *  @var plannedCells
     *  @brief Total quads over patches. */
    quint64 sharedSides = 0, workUsed = 0, plannedCells = 0;
};
// Plan all region patches together before CDT construction. Full shared sides
// use one exact station sequence; opposite/shared counts are solved jointly.
// Authored vertices and indexed source junctions are retained. Unknown/infeasible
// preferred candidates use triangles; a required failure refuses the whole plan.
/**
 * @brief Plan mapped quad patches for all regions jointly, before CDT construction.
 *
 * The arguments, all read only, are the regions, the protecting source geometry and the
 * options. A region is a candidate unless quads are disabled globally or by
 * QuadRegionMode::TrianglesOnly, and only when its ring has at least three vertices, nonzero
 * area and exactly four corners: QuadRegion::corners (strictly increasing ring indices) or,
 * when empty, the vertices turning more than 30 degrees. Each side follows the ring between
 * corners with every source vertex on it inserted; opposite sides and identical shared sides
 * get one station count, at least ceil(length / spacing) and never fewer than their segments.
 * Spacing is QuadRegion::spacing or defaultSize (directional spacing by side alignment),
 * scaled down to limits.maximumSize over the ring's bounding box. A candidate is refused when
 * excluded, outside the domain, inside a hole, fixed patch or triangles-only region, crossed
 * or entered by source geometry, duplicating another boundary, or failing
 * assessPlannedPatch(); repairable failures double the counts of its sides for up to
 * maximumRepairRounds rounds. Required regions are processed first; finally a patch whose
 * boundary vertex lies inside another patch's boundary edge is refused, the later one.
 * @return Owned plan. A refused optional region keeps PatchDisposition::Triangles with its
 * reason. A refused required region, invalid settings or input, cancellation or an exhausted
 * work, source-index, station or subdivision budget give ok false with error set and no
 * patches. Callbacks run synchronously on the calling thread.
 */
QuadPlan planQuadRegions(const QVector<QuadRegion> &, const QuadPlanningGeometry &,
                         const QuadPlanningOptions & = {});
/** @brief One interface face whose nominal-size ratio exceeds the limit. */
struct PatchInterfaceConflict {
    /** @var first
     *  @brief Lower MeshResult::triangles index of the two incident cells.
     *  @var second
     *  @brief Higher MeshResult::triangles index of the two incident cells. */
    int first = -1, second = -1;
    /** @var firstSize
     *  @brief Nominal size of cell first (XY length).
     *  @var secondSize
     *  @brief Nominal size of cell second (XY length). */
    double firstSize = 0, secondSize = 0;
};
/** @brief Result of auditPatchInterfaces(). */
struct PatchInterfaceReport {
    /** @var ok
     *  @brief The audit completed on valid input; violations may still be present.
     *  @var cancelled
     *  @brief Cancellation was observed; ok is false. */
    bool ok = false, cancelled = false;
    QString error; ///< English reason when ok is false; empty otherwise.
    /** @var sharedFaces
     *  @brief Interface edges with exactly two incident cells.
     *  @var violations
     *  @brief Shared faces whose size ratio exceeds the limit. */
    quint64 sharedFaces = 0, violations = 0;
    double largestRatio = 1; ///< @brief Largest size ratio over shared faces; 1 when none.
    /// Up to maximumExamples violations, sorted by cell indices.
    QVector<PatchInterfaceConflict> examples;
};
// Compact independent incidence/ratio check restricted to actual patch edges.
// Ordinary triangles are scanned once; storage grows with patch perimeter.
/**
 * @brief Measure nominal-size ratios across delivered patch interface edges.
 *
 * Nominal size is sqrt(4A / sqrt(3)) for a triangle and sqrt(A) for a quad, from the signed
 * XY area, so every cell must be counterclockwise. An edge with two incident cells is a shared
 * face; an edge with one is skipped. The parameters are unnamed in this declaration; the names
 * below are the definition's.
 * @param[in] m The delivered mesh; read only.
 * @param[in] edges Patch boundary edges as vertex index pairs into the mesh's vertices
 * (direction and marker ignored); read only.
 * @param[in] maximumRatio Largest allowed ratio of the two nominal sizes; finite and >= 1.
 * @param[in] cancelled Optional poll, called synchronously about every 4096 cells.
 * @param[in] maximumExamples Violations to report, clamped to [0, 1000]; those with the
 * smallest cell indices are kept.
 * @return Owned report. ok is false with error set for an invalid ratio, edge or cell, a
 * nonpositive cell area, a face with more than two cells, a boundary edge without any cell,
 * or cancellation. Violations do not make ok false.
 */
PatchInterfaceReport auditPatchInterfaces(const MeshResult &, const QVector<MeshEdge> &,
                                          double maximumRatio,
                                          const std::function<bool()> &cancelled = {},
                                          int maximumExamples = 64);
}
