// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <type_traits>

namespace meshkit
{
/**
 * @brief Point or displacement vector in the horizontal (XY) plane.
 *
 * Coordinates are doubles in the caller's XY plane units; no CRS or unit conversion is
 * applied and no finiteness checks are made, so arithmetic follows plain IEEE rules. The
 * type is a standard-layout pair of doubles `{x, y}` (checked by a static_assert at the end
 * of this header), so a contiguous array of Point2 can be read as interleaved x, y values;
 * the Python bindings expose point arrays that way. It is the point type of
 * detail::StandardStorage, standing in for QPointF.
 */
class Point2
{
    std::array<double, 2> xy_{};

public:
    /** @brief Construct the origin (0, 0). */
    constexpr Point2() = default;
    /**
     * @brief Construct a point from its coordinates.
     * @param[in] x X coordinate in XY plane units.
     * @param[in] y Y coordinate in XY plane units.
     */
    constexpr Point2(double x, double y) : xy_{x, y} {}
    /** @brief X coordinate in XY plane units. */
    constexpr double x() const noexcept { return xy_[0]; }
    /** @brief Y coordinate in XY plane units. */
    constexpr double y() const noexcept { return xy_[1]; }
    /**
     * @brief Pointer to the two contiguous coordinates `{x, y}`.
     * @return Pointer into this object; writing through it changes the point. Valid while
     * this Point2 lives.
     */
    double *data() noexcept { return xy_.data(); }
    /**
     * @brief Read-only pointer to the two contiguous coordinates `{x, y}`.
     * @return Pointer into this object, valid while this Point2 lives.
     */
    const double *data() const noexcept { return xy_.data(); }
    /** @brief True when both coordinates compare equal to 0 (-0.0 counts, NaN does not). */
    constexpr bool isNull() const { return x() == 0 && y() == 0; }
    /**
     * @brief Dot product `a.x * b.x + a.y * b.y`.
     * @param[in] a First vector.
     * @param[in] b Second vector.
     * @return Dot product, in squared XY plane units for two displacement vectors.
     */
    static constexpr double dotProduct(Point2 a, Point2 b) { return a.x() * b.x() + a.y() * b.y(); }
    /**
     * @brief Multiply both coordinates by @p s in place.
     * @param[in] s Scale factor.
     * @return Reference to this point.
     */
    Point2 &operator*=(double s)
    {
        xy_[0] *= s;
        xy_[1] *= s;
        return *this;
    }
    /**
     * @brief Divide both coordinates by @p s in place.
     * @param[in] s Divisor; 0 gives IEEE infinities or NaN, not an error.
     * @return Reference to this point.
     */
    Point2 &operator/=(double s)
    {
        xy_[0] /= s;
        xy_[1] /= s;
        return *this;
    }
    /**
     * @brief Add @p p component-wise in place.
     * @param[in] p Displacement to add.
     * @return Reference to this point.
     */
    Point2 &operator+=(Point2 p)
    {
        xy_[0] += p.x();
        xy_[1] += p.y();
        return *this;
    }
    /**
     * @brief Subtract @p p component-wise in place.
     * @param[in] p Displacement to subtract.
     * @return Reference to this point.
     */
    Point2 &operator-=(Point2 p)
    {
        xy_[0] -= p.x();
        xy_[1] -= p.y();
        return *this;
    }
    /**
     * @brief Component-wise sum.
     * @param[in] a First operand.
     * @param[in] b Second operand.
     * @return `(a.x + b.x, a.y + b.y)`.
     */
    friend constexpr Point2 operator+(Point2 a, Point2 b) { return {a.x() + b.x(), a.y() + b.y()}; }
    /**
     * @brief Component-wise difference, the displacement from @p b to @p a.
     * @param[in] a Minuend.
     * @param[in] b Subtrahend.
     * @return `(a.x - b.x, a.y - b.y)`.
     */
    friend constexpr Point2 operator-(Point2 a, Point2 b) { return {a.x() - b.x(), a.y() - b.y()}; }
    /**
     * @brief Scale a point by a factor.
     * @param[in] a Point or vector to scale.
     * @param[in] s Scale factor.
     * @return `(a.x * s, a.y * s)`.
     */
    friend constexpr Point2 operator*(Point2 a, double s) { return {a.x() * s, a.y() * s}; }
    /**
     * @brief Scale a point by a factor, factor first; same result as `a * s`.
     * @param[in] s Scale factor.
     * @param[in] a Point or vector to scale.
     * @return `(a.x * s, a.y * s)`.
     */
    friend constexpr Point2 operator*(double s, Point2 a) { return a * s; }
    /**
     * @brief Divide a point by a divisor.
     * @param[in] a Point or vector to divide.
     * @param[in] s Divisor; 0 gives IEEE infinities or NaN, not an error.
     * @return `(a.x / s, a.y / s)`.
     */
    friend constexpr Point2 operator/(Point2 a, double s) { return {a.x() / s, a.y() / s}; }
    /**
     * @brief Exact component-wise equality, with no tolerance.
     * @param[in] a First point.
     * @param[in] b Second point.
     * @return True when both coordinates compare equal; +0.0 equals -0.0 and a NaN
     * coordinate is never equal.
     */
    friend constexpr bool operator==(Point2 a, Point2 b)
    {
        return a.x() == b.x() && a.y() == b.y();
    }
    /**
     * @brief Negation of operator==.
     * @param[in] a First point.
     * @param[in] b Second point.
     * @return True when either coordinate differs or is NaN.
     */
    friend constexpr bool operator!=(Point2 a, Point2 b) { return !(a == b); }
};
/**
 * @brief Axis-aligned rectangle in the XY plane, stored as an origin corner and a signed size.
 *
 * Stand-in for QRectF in detail::StandardStorage, with QRectF's naming: left() and top() are
 * the origin corner, right() is left() + width() and bottom() is top() + height(). With a
 * non-negative height, top() is the minimum-y edge, which is not the visually upper edge of a
 * y-up map. Coordinates, width and height are in XY plane units; there are no finiteness
 * checks. Width and height may be negative (see normalized()). contains(), intersects(),
 * intersected() and united() assume normalized boxes (width and height >= 0) and do not
 * normalize their inputs.
 */
class Box2
{
    double x0_ = 0, y0_ = 0, w_ = 0, h_ = 0;

public:
    /** @brief Null box at the origin: width and height 0, so isNull() and isEmpty() hold. */
    constexpr Box2() = default;
    /**
     * @brief Box spanning from corner @p a to corner @p b.
     *
     * The size is stored as `b - a` and the corners are not reordered, so a @p b left of or
     * below @p a gives a negative width or height.
     * @param[in] a Origin corner; becomes left(), top().
     * @param[in] b Opposite corner; becomes right(), bottom() (up to rounding of the size).
     */
    constexpr Box2(Point2 a, Point2 b)
        : x0_(a.x()), y0_(a.y()), w_(b.x() - a.x()), h_(b.y() - a.y())
    {
    }
    /** @brief X of the origin corner; the minimum x when width() >= 0. */
    constexpr double left() const { return x0_; }
    /** @brief left() + width(); the maximum x when width() >= 0. */
    constexpr double right() const { return x0_ + w_; }
    /** @brief Y of the origin corner; the minimum y when height() >= 0. */
    constexpr double top() const { return y0_; }
    /** @brief top() + height(); the maximum y when height() >= 0. */
    constexpr double bottom() const { return y0_ + h_; }
    /** @brief Signed extent along x in XY plane units; negative when not normalized. */
    constexpr double width() const { return w_; }
    /** @brief Signed extent along y in XY plane units; negative when not normalized. */
    constexpr double height() const { return h_; }
    /**
     * @brief Box from an origin corner and a size, all stored as given.
     * @param[in] x Left (origin) x in XY plane units.
     * @param[in] y Top (origin) y in XY plane units.
     * @param[in] w Width; may be negative.
     * @param[in] h Height; may be negative.
     */
    Box2(double x, double y, double w, double h) : x0_(x), y0_(y), w_(w), h_(h) {}
    /**
     * @brief Copy with non-negative width and height covering the same two corners.
     * @return Box from `(min(left, right), min(top, bottom))` to
     * `(max(left, right), max(top, bottom))`.
     */
    Box2 normalized() const
    {
        return {{std::min(left(), right()), std::min(top(), bottom())},
                {std::max(left(), right()), std::max(top(), bottom())}};
    }
    /** @brief True when width and height are both > 0. */
    bool isValid() const { return w_ > 0 && h_ > 0; }
    /**
     * @brief True when width <= 0 or height <= 0.
     *
     * A NaN width or height makes both isValid() and isEmpty() false.
     */
    bool isEmpty() const { return w_ <= 0 || h_ <= 0; }
    /** @brief True when width and height are both exactly 0 (one zero side is empty, not null). */
    bool isNull() const { return w_ == 0 && h_ == 0; }
    /** @brief Corner (right(), bottom()). */
    Point2 bottomRight() const { return {right(), bottom()}; }
    /** @brief Corner (left(), top()), the origin corner. */
    Point2 topLeft() const { return {left(), top()}; }
    /**
     * @brief Inclusive point-in-box test on coordinates; same as `contains(Point2(x, y))`.
     * @param[in] x Query x in XY plane units.
     * @param[in] y Query y in XY plane units.
     * @return True when the point lies inside or on the boundary.
     */
    bool contains(double x, double y) const { return contains({x, y}); }
    /**
     * @brief Inclusive point-in-box test.
     * @param[in] p Query point in XY plane units.
     * @return True when `left() <= p.x <= right()` and `top() <= p.y <= bottom()`, so points
     * on an edge or corner are inside. Always false for a box with a negative width or
     * height, or for a NaN coordinate.
     */
    bool contains(Point2 p) const
    {
        return p.x() >= left() && p.x() <= right() && p.y() >= top() && p.y() <= bottom();
    }
    /**
     * @brief Strict overlap test.
     * @param[in] b Other box, assumed normalized.
     * @return True when `left() < b.right()`, `right() > b.left()`, `top() < b.bottom()` and
     * `bottom() > b.top()`. Boxes that only share an edge or a corner do not intersect.
     */
    bool intersects(Box2 b) const
    {
        return left() < b.right() && right() > b.left() && top() < b.bottom() && bottom() > b.top();
    }
    /**
     * @brief Overlap of this box and @p b.
     * @param[in] b Other box, assumed normalized like this one.
     * @return Box from `(max(left), max(top))` to `(min(right), min(bottom))` of the two
     * boxes. Boxes that touch along an edge give a zero-width or zero-height box there;
     * disjoint boxes give the null box Box2() at the origin.
     */
    Box2 intersected(Box2 b) const
    {
        double x = std::max(left(), b.left()), y = std::max(top(), b.top()),
               r = std::min(right(), b.right()), t = std::min(bottom(), b.bottom());
        return r < x || t < y ? Box2{} : Box2{{x, y}, {r, t}};
    }
    /**
     * @brief Copy with each edge moved by an offset, in XY plane units.
     *
     * `adjusted(-d, -d, d, d)` grows a normalized box by `d` on every side.
     * @param[in] l Added to left().
     * @param[in] t Added to top().
     * @param[in] r Added to right().
     * @param[in] b Added to bottom().
     * @return Box from `(left() + l, top() + t)` to `(right() + r, bottom() + b)`.
     */
    Box2 adjusted(double l, double t, double r, double b) const
    {
        return {{left() + l, top() + t}, {right() + r, bottom() + b}};
    }
    /** @brief Midpoint (left() + width() / 2, top() + height() / 2). */
    constexpr Point2 center() const { return {x0_ + w_ * .5, y0_ + h_ * .5}; }
    /**
     * @brief Bounding box of this box and @p b.
     * @param[in] b Other box.
     * @return Box from `(min(left), min(top))` to `(max(right), max(bottom))` of the two boxes.
     * Each box's left() and top() are taken as its minimum corner, so both must be
     * normalized. A null or empty box is not skipped: uniting with Box2() extends the
     * result to include the origin.
     */
    Box2 united(Box2 b) const
    {
        return {{std::min(x0_, b.x0_), std::min(y0_, b.y0_)},
                {std::max(right(), b.right()), std::max(bottom(), b.bottom())}};
    }
};
/**
 * @brief Plain x, y, z coordinate triple; the vertex type of meshkit::Mesh.
 *
 * Aggregate of three doubles with standard layout and no padding (checked by a static_assert
 * below), so a `std::vector<Point3>` is a contiguous N x 3 array of doubles; the Python
 * bindings expose Mesh::vertices that way. Nothing is validated on construction;
 * validateMesh() rejects a mesh vertex with a non-finite component.
 */
struct Point3 {
    /**
     * @var x
     * @brief Horizontal x coordinate in XY plane units; default 0. For a Mesh,
     * Metadata::horizontalMetresPerUnit gives metres per unit.
     * @var y
     * @brief Horizontal y coordinate in XY plane units; default 0. For a Mesh,
     * Metadata::horizontalMetresPerUnit gives metres per unit.
     * @var z
     * @brief Vertical coordinate (elevation) in Z units; default 0. For a Mesh,
     * Metadata::verticalMetresPerUnit gives metres per unit.
     */
    double x = 0, y = 0, z = 0;
};
static_assert(std::is_standard_layout<Point3>::value && sizeof(Point3) == 3 * sizeof(double));
static_assert(std::is_standard_layout<Point2>::value && sizeof(Point2) == 2 * sizeof(double));
}
