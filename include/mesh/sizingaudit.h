// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshresult.h"
#include <QRectF>
#include <functional>

namespace mesh
{
// Version 1: nominal triangle size sqrt(4A/sqrt(3)), quad size sqrt(A).
// The minimum target is advisory. A separately requested edge floor is hard
// in strict mode. Transition is the ratio of nominal sizes across shared faces.
/**
 * @brief Limits, budgets and callbacks for auditMeshSizing().
 *
 * Sizes and lengths are XY lengths in the mesh's planar units. Every numeric limit must be
 * finite and nonnegative, and 0 disables it. Comparisons carry a relative tolerance of 1e-9.
 */
struct SizingAuditOptions {
    /** @var minimumTarget
     *  @brief Advisory smallest nominal size; smaller cells are counted and flagged
     *  BelowTarget but do not fail SizingAuditReport::limitsSatisfied().
     *  @var maximumSize
     *  @brief Global largest nominal size.
     *  @var minimumEdge
     *  @brief Hard floor for the shortest edge of every cell.
     *  @var adjacentRatio
     *  @brief Largest ratio of nominal sizes across a shared face; must be >= 1 when nonzero. */
    double minimumTarget = 0, maximumSize = 0, minimumEdge = 0, adjacentRatio = 0;
    // Bound from the immutable requested field over the WHOLE cell bbox.
    // Zero means unbounded; negative/nonfinite values are errors, never passes.
    /// Optional nominal-size bound, called synchronously once per cell with its XY bounding box.
    std::function<double(const QRectF &)> footprintMaximum;
    /// Optional cancellation poll, called synchronously at the start and every 4096 work units.
    std::function<bool()> cancelled;
    /** @var maximumWorkspaceBytes
     *  @brief Estimated limit for the face records, cell sizes and constraint keys (1 GiB by
     *  default); when exceeded, no cell is checked and the report is resource limited.
     *  @var workBudget
     *  @brief Work-unit limit; 0 selects 1024 per cell plus 100,000. */
    quint64 maximumWorkspaceBytes = quint64(1) << 30, workBudget = 0;
    int maximumExamples = 20; ///< Example records kept, clamped to [0, 1000].
};
/** @brief Per-cell bits in SizingAuditReport::cellFlags and SizingAuditExample::flags. */
enum SizingCellFlag : quint8 {
    BelowTarget = 1,         ///< Nominal size below the advisory minimumTarget.
    BelowEdgeFloor = 2,      ///< Shortest edge below the hard minimumEdge floor.
    AboveMaximum = 4,        ///< Nominal size above the global maximumSize.
    AboveLocalMaximum = 8,   ///< Nominal size above the cell's footprintMaximum bound.
    TransitionExceeded = 16, ///< Either cell of a shared face whose ratio exceeds adjacentRatio.
    TouchesConstraint = 32   ///< An edge is in MeshResult::boundaryEdges; not a violation.
};
/** @brief One recorded violation, for diagnostics. */
struct SizingAuditExample {
    /** @var cell
     *  @brief Index into MeshResult::triangles; the larger cell for a transition.
     *  @var neighbour
     *  @brief The smaller cell across the face for TransitionExceeded; -1 otherwise. */
    int cell = -1, neighbour = -1;
    quint8 flags = 0; ///< The single SizingCellFlag this example reports.
    QPointF location; ///< Average of the cell's vertex XY coordinates.
    /** @var measured
     *  @brief Shortest edge, nominal size or size ratio, according to flags.
     *  @var requested
     *  @brief The limit the measured value was compared with. */
    double measured = 0, requested = 0;
    QString message; ///< English description of the violation.
};
/** @brief Result of auditMeshSizing(). Counts and extremes are partial unless valid is true. */
struct SizingAuditReport {
    /** @var complete
     *  @brief Not stopped by cancellation or a work or workspace budget. Also true when invalid
     *  input stopped the audit, so check valid as well.
     *  @var valid
     *  @brief Every check ran on valid limits and mesh geometry.
     *  @var cancelled
     *  @brief Cancellation was observed.
     *  @var resourceLimited
     *  @brief The work budget or the estimated workspace limit was exceeded. */
    bool complete = false, valid = false, cancelled = false, resourceLimited = false;
    QString error; ///< English reason when valid is false; empty otherwise.
    /** @var cellsChecked
     *  @brief Cells measured.
     *  @var facesChecked
     *  @brief Faces shared by exactly two cells.
     *  @var belowTarget
     *  @brief Cells below the advisory minimumTarget.
     *  @var belowEdgeFloor
     *  @brief Cells with an edge below minimumEdge.
     *  @var aboveMaximum
     *  @brief Cells above maximumSize.
     *  @var aboveLocalMaximum
     *  @brief Cells above their footprintMaximum bound.
     *  @var transitionFaces
     *  @brief Shared faces whose size ratio exceeds adjacentRatio.
     *  @var workUsed
     *  @brief Work units consumed. */
    quint64 cellsChecked = 0, facesChecked = 0, belowTarget = 0, belowEdgeFloor = 0,
            aboveMaximum = 0, aboveLocalMaximum = 0, transitionFaces = 0, workUsed = 0;
    /** @var smallestNominal
     *  @brief Smallest nominal cell size (XY length).
     *  @var largestNominal
     *  @brief Largest nominal cell size (XY length).
     *  @var largestAdjacentRatio
     *  @brief Largest nominal-size ratio across a shared face; 1 when there is none. */
    double smallestNominal = 0, largestNominal = 0, largestAdjacentRatio = 1;
    /// SizingCellFlag bits per MeshResult::triangles entry; empty if no cell was measured.
    QVector<quint8> cellFlags;
    /// First violations found, up to maximumExamples; BelowTarget is never recorded.
    QVector<SizingAuditExample> examples;
    /**
     * @brief Whether the mesh met every hard limit.
     * @return True when complete and valid and no cell or face violated minimumEdge,
     * maximumSize, footprintMaximum or adjacentRatio. Advisory belowTarget counts are ignored.
     */
    bool limitsSatisfied() const
    {
        return complete && valid && !belowEdgeFloor && !aboveMaximum && !aboveLocalMaximum &&
               !transitionFaces;
    }
};
// Read-only independent geometry/connectivity audit. Does not call the CDT's
// acceptance or sizing helpers. Sorted compact face records avoid an edge hash
// proportional to millions of cells. Failure/exhaustion is never certification.
/**
 * @brief Measure delivered cell sizes, edge floors and size transitions of a mixed mesh.
 *
 * The first argument is the mesh, read only. It must be ok and nonempty. Every cell, triangle
 * or quad, needs distinct in-range vertex indices with finite XY and a strictly convex
 * counterclockwise shape; boundary edges need distinct in-range vertices; a face may be shared
 * by at most two cells, which must traverse it in opposite directions. Only XY is used. The
 * second argument holds the limits, budgets and callbacks; callbacks run synchronously on the
 * calling thread.
 * @return Owned report. Invalid limits, mesh geometry or footprint bounds set error and leave
 * valid false; cancellation or an exhausted budget also leave complete false.
 */
SizingAuditReport auditMeshSizing(const MeshResult &, const SizingAuditOptions & = {});
}
