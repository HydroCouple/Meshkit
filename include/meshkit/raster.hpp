// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "job.hpp"
#include <memory>
namespace meshkit
{
/**
 * @brief Regular grid of elevation samples at nodes, in the mesh's XY and Z units.
 *
 * Node (i, j) lies at `origin + (i * xSpacing, j * ySpacing)`, so the row index j increases
 * toward +Y. RasterSurface's constructor checks every field.
 */
struct RasterData {
    /// Node (not cell) count along X; must be >= 2.
    std::uint32_t columns = 0, rows = 0;
    /** @var rows
     *  @brief Node count along Y; must be >= 2. */
    /// XY position of node (0, 0), the minimum-x, minimum-y corner; must be finite.
    Point2 origin;
    /// Node spacing along X in XY units; must be positive and finite.
    double xSpacing = 1, ySpacing = 1;
    /** @var ySpacing
     *  @brief Node spacing along Y in XY units; must be positive and finite. */
    /// Z-unit samples; size columns * rows, node (i, j) at index j * columns + i.
    std::vector<double> heights; // Row-major node samples, NaN = unknown.
    /// Most grid cells one query may visit (cells in the triangle's bounding range); nonzero.
    std::uint64_t maximumCellsPerQuery = 4000000;
};
/**
 * @brief Result of RasterSurface::query() for one triangle.
 *
 * bound and witness are meaningful only when valid is true.
 */
struct RasterQuery {
    /// True when bound certifies the whole triangle; false for any refusal (see query()).
    bool valid = false, resourceLimited = false;
    /** @var resourceLimited
     *  @brief True when the cell range exceeded RasterData::maximumCellsPerQuery; valid is
     *  then false. */
    /// Upper bound, in Z units, on |bilinear surface - triangle plane| over the triangle,
    /// including a rounding margin; 0 unless valid.
    double bound = 0;
    /// XY point where the largest residual was sampled; the triangle's first corner when no
    /// sample exceeded zero.
    Point2 witness;
    /// Grid cells whose overlap with the triangle has nonzero area (partial if the query
    /// stopped at a nonfinite node).
    std::uint64_t cells = 0;
};
// Immutable native bilinear surface. Providers share storage and perform no
// Python/GDAL calls. The certificate concerns this surface, not physical DEM
// accuracy. Every intersected grid cell must have four finite node samples.
/**
 * @brief Immutable bilinear elevation surface over a RasterData grid.
 *
 * Copies share one immutable sample buffer, so copying is cheap and const member functions
 * modify no shared state. NaN samples are unknown and are never interpolated across.
 */
class RasterSurface
{
    std::shared_ptr<const RasterData> data_;

public:
    /**
     * @brief Validate a grid and take ownership of its samples.
     * @param[in] data Grid moved into shared immutable storage.
     * @throws std::invalid_argument When columns or rows is below 2, heights.size() differs
     * from columns * rows, a spacing is not positive and finite, origin is not finite,
     * maximumCellsPerQuery is 0, or the far corner node is not finite.
     */
    explicit RasterSurface(RasterData data);
    /**
     * @brief Bilinearly interpolate the elevation at one XY point.
     * @param[in] x X in the grid's XY units.
     * @param[in] y Y in the grid's XY units.
     * @return Elevation in Z units; NaN outside the closed node extent or for nonfinite input.
     * A NaN supporting node gives NaN (an infinite one a nonfinite result).
     */
    double elevation(double x, double y) const;
    /**
     * @brief Bound the deviation between this surface and the plane through a triangle.
     *
     * The triangle is clipped against every grid cell in its bounding range; the residual is
     * evaluated at each clipped polygon's vertices and at the interior extremum of each clipped
     * edge, where the maximum of a bilinear-minus-linear function over a convex polygon lies.
     * @param[in] triangle Three borrowed XY corners, counterclockwise; not retained.
     * @param[in] z Three borrowed corner elevations in Z units, in the same order.
     * @return valid with bound, witness and cells on success. Not valid when an input is
     * nonfinite, the triangle's bounding box leaves the node extent, the triangle is not
     * strictly counterclockwise, the plane slope is nonfinite, an overlapped cell has a
     * nonfinite node, or no cell overlaps with nonzero area. resourceLimited (and not valid)
     * when the cell range exceeds RasterData::maximumCellsPerQuery.
     */
    RasterQuery query(const Point2 *triangle, const double *z) const;
    /**
     * @brief Package this surface as job terrain providers.
     *
     * Sets elevation, terrainWitness and terrainBound; maximumSize and cancelled stay empty.
     * Each callback holds its own copy of this surface, so it remains usable after this object
     * is destroyed. terrainWitness and terrainBound return query()'s bound (writing the witness
     * through a non-null point argument) or NaN when the query is not valid. When it is
     * resource-limited they throw native_detail::Stop (code `raster_work_limit`, status
     * `ResourceLimit`), not a std::exception; the native job reports it as a failure.
     * @param[in] identity Caller-supplied dataset/version identity copied into the result.
     * @return Owned provider set.
     */
    Providers providers(std::string identity = {}) const;
    /** @brief The shared samples; valid while this object exists and is not reassigned. */
    const RasterData &data() const { return *data_; }
};
}
