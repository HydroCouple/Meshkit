// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "constraintsimplifier.h"
#include <QRectF>

namespace mesh
{
/** @brief Display data for one feature of a constraint-simplification preview. */
struct ConstraintPreviewFeature {
    /** @var id
     *  @brief SimplificationFeature::id of the source feature.
     *  @var label
     *  @brief Source label, or id when the label is empty. */
    QString id, label;
    /** @var original
     *  @brief Original XY vertices; when longer than the per-path display budget, evenly
     *  subsampled by index, keeping the first and last vertex.
     *  @var proposed
     *  @brief Candidate XY vertices, subsampled the same way.
     *  @var removed
     *  @brief A sample of the original vertices the candidate removed, for markers; at most 100
     *  per feature within the preview-wide marker budget. */
    QVector<QPointF> original, proposed, removed;
    /** @var verticesBefore
     *  @brief Full original vertex count, including a repeated closing vertex.
     *  @var verticesAfter
     *  @brief Full candidate vertex count.
     *  @var removedCount
     *  @brief SimplificationChange::removed of the feature. */
    quint64 verticesBefore = 0, verticesAfter = 0, removedCount = 0;
    /** @var maxDeviation
     *  @brief SimplificationChange::maxDeviation (XY units).
     *  @var maxZDeviation
     *  @brief SimplificationChange::maxZDeviation (elevation units).
     *  @var areaChange
     *  @brief SimplificationChange::areaChange (squared XY units). */
    double maxDeviation = 0, maxZDeviation = 0, areaChange = 0;
    /** @var closed
     *  @brief The source is a ring: its closed flag is set or its first and last vertices are
     *  exactly equal.
     *  @var displayReduced
     *  @brief original, proposed or removed holds fewer points than the full data.
     *  @var rolledBack
     *  @brief SimplificationChange::rolledBack of the feature.
     *  @var budgetRetained
     *  @brief SimplificationChange::budgetRetained of the feature. */
    bool closed = false, displayReduced = false, rolledBack = false, budgetRetained = false;
};
/**
 * @brief Bounded display summary of a simplification candidate, built by
 * makeConstraintPreview(). The totals and bounds cover every feature; features holds only the
 * displayed selection.
 */
struct ConstraintPreview {
    /// @brief Displayed features: changed ones (vertices removed, rolled back or budget-retained)
    /// first in input order, then unchanged context in input order, up to the feature limit.
    QVector<ConstraintPreviewFeature> features;
    QRectF bounds; ///< XY extent of all original vertices; a default QRectF when there are none.
    /** @var featuresTotal
     *  @brief Number of source features.
     *  @var verticesBefore
     *  @brief Original vertex count over all features.
     *  @var verticesAfter
     *  @brief Candidate vertex count over all features.
     *  @var removedTotal
     *  @brief Sum of SimplificationChange::removed over all features. */
    quint64 featuresTotal = 0, verticesBefore = 0, verticesAfter = 0, removedTotal = 0;
    /** @var displayReduced
     *  @brief Some features, vertices or removed-vertex markers are not displayed.
     *  @var cancelled
     *  @brief Building stopped on cancellation; the other fields are partial. */
    bool displayReduced = false, cancelled = false;
    /** @var horizontalUnit
     *  @brief XY unit label for display; left empty by makeConstraintPreview().
     *  @var verticalUnit
     *  @brief Elevation unit label for display; left empty by makeConstraintPreview(). */
    QString horizontalUnit, verticalUnit;
};
// Bounded DISPLAY data only. Audits and generation always use the full geometry.
// Changed features precede unchanged context; sampling never changes the result.
// The candidate and source mapping must have passed the preparation audit.
/**
 * @brief Build bounded display data comparing source features with a simplification candidate.
 *
 * Reads its inputs only; it does not run or repeat the preparation audit.
 * @param[in] original Source features given to simplifyConstraints(); read only.
 * @param[in] candidate Result of simplifyConstraints() for @p original; read only. When it is
 * not ok or its feature or change count differs from @p original, an empty default preview is
 * returned without any error indication.
 * @param[in] cancelled Optional poll, called synchronously once per feature and about every
 * 4096 visited vertices; returning true returns the partial preview with cancelled set.
 * @param[in] maximumVertices Display vertex budget, clamped to [4, 250000]. Up to
 * min(2000, maximumVertices / 10) go to removed-vertex markers; the rest is split evenly over
 * the original and proposed paths of the displayed features, at least 2 points per path.
 * @param[in] maximumFeatures Displayed feature limit, clamped to at least 1 and at most
 * min(5000, path budget / 4).
 * @return Owned preview. The unit labels are left empty for the caller to fill.
 */
ConstraintPreview makeConstraintPreview(const QVector<SimplificationFeature> &original,
                                       const ConstraintSimplificationResult &candidate,
                                       const std::function<bool()> &cancelled = {},
                                       int maximumVertices = 50000, int maximumFeatures = 500);
}
