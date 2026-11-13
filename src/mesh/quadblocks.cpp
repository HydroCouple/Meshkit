// SPDX-License-Identifier: Apache-2.0
// Qt value adapter; numerical implementation is shared with the native package.
#include "mesh/qtnative.h"
namespace mesh {
namespace a=native_adapter;namespace n=meshkit::plan;
QuadBlockGrid quadBlockGridFromMesh(const MeshResult&m,double pitch,const QVector<QVector<QPointF>>&b){return a::qt(n::quadBlockGridFromMesh(a::native(m),pitch,a::native(b)));}
/**
 * @brief Build a block grid from a size field over a rectangle, before any triangulation
 * (Qt adapter over meshkit::plan::quadBlockGridFromField).
 * @param[in] r Area to cover, mesh XY units; must have positive width and height. The grid
 * starts at its top-left (minimum x, minimum y) corner with ceil(width / pitch) ×
 * ceil(height / pitch) cells.
 * @param[in] pitch Cell side in mesh XY units; must be finite and > 0.
 * @param[in] f Size bound for one cell's box, called synchronously once per cell; a cell's h
 * is 0.9 × the result when that is finite, > 0 and fits a float, else the cell is unusable
 * (NaN). Must be non-empty: this adapter always wraps it, so an empty f throws
 * std::bad_function_call instead of returning an invalid grid.
 * @param[in] b Feature polylines in the same units, sampled every half pitch; each sample
 * marks its cell and the 8 neighbours unusable (repeat the first point to close a ring).
 * @param[in] cancel Optional; polled every 4096 cells or blocker samples.
 * @param[in] cap Upper bound on cols × rows; must be >= 1.
 * @return The grid, or an invalid default grid on an invalid rectangle or pitch, cap < 1,
 * too many cells or samples, a non-finite blocker coordinate or cancellation.
 */
QuadBlockGrid quadBlockGridFromField(const QRectF&r,double pitch,const std::function<double(const QRectF&)>&f,const QVector<QVector<QPointF>>&b,const std::function<bool()>&cancel,qint64 cap){return a::qt(n::quadBlockGridFromField(a::native(r),pitch,[&](const meshkit::Box2&box){return f(a::qt(box));},a::native(b),cancel,cap));}
QVector<QuadRegion> placeQuadBlocks(const QuadBlockGrid&g,const QuadBlockOptions&o){return a::qt(n::placeQuadBlocks(a::native(g),a::native(o)));}
}
