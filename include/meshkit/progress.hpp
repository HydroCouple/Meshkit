// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>

namespace meshkit
{
/**
 * @brief Counter snapshot passed to ConstrainedDelaunay::QualityOptions::onProgress during
 * refineQuality().
 *
 * Published at the start, when each phase begins, periodically within phases and once at the
 * end; the values are advisory.
 */
struct GenerationProgress {
    /** @brief Refinement phase being run. */
    enum class Stage { Refining, Smoothing, Terrain, Coarsening, Verifying };
    /// @var Stage::Refining
    /// @brief Size and angle refinement, and later the sizing-repair passes.
    /// @var Stage::Smoothing
    /// @brief Non-degrading smoothing of free vertices.
    /// @var Stage::Terrain
    /// @brief Worst-first insertion of terrain points.
    /// @var Stage::Coarsening
    /// @brief Removal of free vertices.
    /// @var Stage::Verifying
    /// @brief Final terrain check of every live triangle.
    Stage stage = Stage::Refining; ///< Phase being run.
    // Allocated non-super vertex slots, including unused inputs/removed slots.
    /// Working vertex slots, excluding the three super-triangle vertices.
    std::int64_t workingVertices = 0;
    /// Live triangle count.
    std::int64_t triangles = 0; // live triangles, excluding removed regions
    /// Quadrilateral count; refineQuality() leaves it 0 for the caller to fill.
    std::int64_t quads = 0;     // fixed structured cells, supplied by generator
    /**
     * @var inserted
     * @brief Vertices inserted so far, segment splits included.
     * @var removed
     * @brief Vertices removed by coarsening so far.
     */
    std::int64_t inserted = 0, removed = 0;
};
}
