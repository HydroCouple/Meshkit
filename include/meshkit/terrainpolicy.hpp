// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace meshkit
{
/** @brief Order in which terrain coarsening tries to remove free vertices. */
enum class CoarseningOrder { LegacyFlatness, ResolutionAware };
/** @var CoarseningOrder::LegacyFlatness
 *  @brief Flattest vertex star first (smallest facet-normal spread), tile by tile in
 *  spatial (Morton) order. */
/** @var CoarseningOrder::ResolutionAware
 *  @brief Lowest resolutionRemovalImportance() first, then lowest actual/target size ratio,
 *  tile by tile; the refinement options must then supply a target-size callback, a positive
 *  reference size and a positive vertical-to-XY normal scale. */
/** @brief How TerrainTolerancePolicy::tolerance() derives a footprint's terrain tolerance. */
enum class TerrainToleranceMode { Constant, TargetSize };
/** @var TerrainToleranceMode::Constant
 *  @brief Use the reference tolerance everywhere. */
/** @var TerrainToleranceMode::TargetSize
 *  @brief Scale the reference tolerance linearly with the footprint's target size h. */

// Immutable physical policy. Reference size and h share an XY unit; tolerances share a Z unit. h is the smallest prescribed
// target over the entire affected footprint. Zero h means unbounded size.
// Missing/invalid values return NaN, never an implicit relaxed tolerance.
/**
 * @brief Maps a reference terrain tolerance and a footprint target size to the tolerance used
 * there.
 *
 * Sizes are in XY units and tolerances in Z units. In TargetSize mode the tolerance is
 * `referenceTolerance * h / referenceSize`, clamped to [minimumTolerance, cap], where cap is
 * maximumTolerance, or the reference tolerance when maximumTolerance is 0.
 */
struct TerrainTolerancePolicy {
    /// Tolerance mode; Constant (the default) ignores the other fields except for validation.
    TerrainToleranceMode mode = TerrainToleranceMode::Constant;
    /// Target size, in XY units, at which the scaled tolerance equals the reference tolerance;
    /// must be positive and finite in both modes.
    double referenceSize = 10;
    /// Lower clamp, in Z units, for TargetSize mode; finite and >= 0 (0 = no floor).
    double minimumTolerance = 0;
    /// Upper clamp, in Z units, for TargetSize mode; finite and >= 0, and >= minimumTolerance
    /// unless 0.
    double maximumTolerance = 0; // Zero inherits the resolved reference tolerance.

    /**
     * @brief Check the mode and the finite, nonnegative and ordered numeric fields.
     * @return True when mode is a declared enumerator, referenceSize is positive and finite,
     * both tolerances are finite and >= 0, and maximumTolerance is 0 or >= minimumTolerance.
     */
    bool valid() const
    {
        return (mode == TerrainToleranceMode::Constant ||
                mode == TerrainToleranceMode::TargetSize) &&
               std::isfinite(referenceSize) && referenceSize > 0 &&
               std::isfinite(minimumTolerance) && minimumTolerance >= 0 &&
               std::isfinite(maximumTolerance) && maximumTolerance >= 0 &&
               (maximumTolerance == 0 || maximumTolerance >= minimumTolerance);
    }
    /**
     * @brief Resolve the terrain tolerance for one footprint.
     * @param[in] referenceTolerance Positive finite base tolerance in Z units.
     * @param[in] h Smallest prescribed target size over the footprint, in XY units; finite and
     * >= 0, where 0 means unbounded.
     * @return Constant mode: referenceTolerance. TargetSize mode: the cap for h == 0,
     * otherwise the clamped scaled value (computed in long double). NaN when the policy is not
     * valid(), an argument is out of range, or minimumTolerance exceeds the cap.
     */
    double tolerance(double referenceTolerance, double h) const
    {
        const double unknown = std::numeric_limits<double>::quiet_NaN();
        if (!valid() || !(referenceTolerance > 0) || !std::isfinite(referenceTolerance) ||
            !std::isfinite(h) || h < 0)
            return unknown;
        if (mode == TerrainToleranceMode::Constant)
            return referenceTolerance;
        const double cap = maximumTolerance > 0 ? maximumTolerance : referenceTolerance;
        if (minimumTolerance > cap)
            return unknown;
        if (h == 0)
            return cap;
        const long double scaled = static_cast<long double>(referenceTolerance) * h / referenceSize;
        return double(std::clamp(scaled, static_cast<long double>(minimumTolerance),
                                 static_cast<long double>(cap)));
    }
};

// Ordering heuristic only: maximum normal departure weighted by sampling scale
// relative to the immutable target. All removal gates remain separate.
/**
 * @brief Rank a vertex for resolution-aware coarsening; lower values are tried first.
 * @param[in] flatness Star flatness: the smallest cosine between a facet normal and the star's
 * summed normal, clamped to [-1, 1]; 1 is flat.
 * @param[in] actualSize Current local cell size in XY units; positive and finite.
 * @param[in] targetSize Prescribed target size in the same units; positive and finite.
 * @return `sqrt(2 * (1 - flatness)) * actualSize / targetSize`, dimensionless and >= 0;
 * +infinity when flatness is nonfinite or a size is not positive and finite.
 */
inline double resolutionRemovalImportance(double flatness, double actualSize, double targetSize)
{
    if (!std::isfinite(flatness) || !(actualSize > 0) || !std::isfinite(actualSize) ||
        !(targetSize > 0) || !std::isfinite(targetSize))
        return std::numeric_limits<double>::infinity();
    return std::sqrt(std::max(0., 2 * (1 - std::clamp(flatness, -1., 1.)))) *
           (actualSize / targetSize);
}
}
