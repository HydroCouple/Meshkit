// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshkit/detail/storage.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
namespace meshkit::plan
{
using detail::Vector;
using detail::Map;
using detail::Set;
/** @brief The constant pi as a double. */
inline constexpr double pi = 3.141592653589793238462643383279502884;
/**
 * @brief Owned byte string with the QString-style helpers used for tags and diagnostics.
 *
 * A std::string (no encoding is imposed) that adds isEmpty() and `%1`..`%9` placeholder
 * substitution through arg(). Functions such as validate() return an empty Text for success.
 */
class Text : public std::string
{
public:
    using std::string::string;
    Text() = default;
    /**
     * @brief Copy a std::string; also serves as an implicit conversion.
     * @param[in] v String to copy.
     */
    Text(const std::string &v) : std::string(v) {};
    /** @brief Return true when the string holds no characters. */
    bool isEmpty() const { return empty(); }
    /**
     * @brief Replace the lowest-numbered `%1`..`%9` placeholder with a formatted value.
     * @param[in] value Value written with `operator<<` to a default-formatted std::ostringstream
     * (a double therefore prints with six significant digits).
     * @return A copy in which every occurrence of the lowest-numbered placeholder present is
     * replaced, or an unchanged copy when there is none. Only one digit is read, so `%10` is
     * `%1` followed by `0`. Inserted text is not rescanned by this call, but a later arg() call
     * sees any placeholder it contains.
     */
    template <class T> Text arg(const T &value) const
    {
        std::ostringstream out;
        out << value;
        Text result = *this;
        std::size_t at = std::string::npos;
        int lowest = 100;
        for (std::size_t i = 0; i + 1 < size(); ++i)
            if ((*this)[i] == '%' && (*this)[i + 1] >= '1' && (*this)[i + 1] <= '9') {
                int n = (*this)[i + 1] - '0';
                if (n < lowest) {
                    lowest = n;
                    at = i;
                }
            }
        if (at != std::string::npos) {
            const auto key = "%" + std::to_string(lowest);
            std::size_t p = 0;
            while ((p = result.find(key, p)) != std::string::npos) {
                result.replace(p, key.size(), out.str());
                p += out.str().size();
            }
        }
        return result;
    }
    /**
     * @brief Substitute two values in turn; exactly `arg(a).arg(b)`.
     * @param[in] a Value for the lowest-numbered placeholder.
     * @param[in] b Value for the lowest-numbered placeholder left after substituting @p a.
     * @return The substituted copy.
     */
    template <class T, class U> Text arg(const T &a, const U &b) const { return arg(a).arg(b); }
};
/** @brief Raw byte buffer held in a std::string; the planner uses it for exact binary keys. */
using Bytes = std::string;
/**
 * @brief Point sequence in XY units used for polygon rings and polylines.
 *
 * The type imposes no closure, orientation, simplicity or finiteness rule; each function that
 * takes a Polygon states its own requirements.
 */
class Polygon : public Vector<Point2>
{
public:
    using Vector<Point2>::Vector;
    Polygon() = default;
    /**
     * @brief Copy a point vector.
     * @param[in] v Points to copy, in order.
     */
    Polygon(const Vector<Point2> &v) : Vector<Point2>(v) {};
    /**
     * @brief Copy a std::vector of points.
     * @param[in] v Points to copy, in order.
     */
    Polygon(const std::vector<Point2> &v) : Vector<Point2>(v) {}
    /**
     * @brief Axis-aligned bounding box of all points.
     * @return Box from the minimum to the maximum x and y, in XY units; a default (all-zero)
     * Box2 when the polygon is empty. Coordinates are not checked for finiteness.
     */
    Box2 boundingRect() const
    {
        if (empty())
            return {};
        double x0 = front().x(), x1 = x0, y0 = front().y(), y1 = y0;
        for (auto p : *this) {
            x0 = std::min(x0, p.x());
            x1 = std::max(x1, p.x());
            y0 = std::min(y0, p.y());
            y1 = std::max(y1, p.y());
        }
        return {{x0, y0}, {x1, y1}};
    }
};
/** @brief Straight segment between two XY points. */
struct Line2 {
    /// @var a
    /// @brief Start point, XY units.
    /// @var b
    /// @brief End point, XY units.
    Point2 a, b;
    /**
     * @brief Construct the segment from @p x to @p y.
     * @param[in] x Start point, stored in #a.
     * @param[in] y End point, stored in #b.
     */
    Line2(Point2 x, Point2 y) : a(x), b(y) {}
    /** @brief Euclidean length |b - a| via std::hypot, in XY units; finiteness is not checked. */
    double length() const { return std::hypot(b.x() - a.x(), b.y() - a.y()); }
};
/** @brief Mesh vertex: planar position plus elevation. */
struct MeshVertex {
    Point2 xy;    ///< Planar position, XY units.
    double z = 0; ///< Elevation, Z units; defaults to 0 (there is no separate "unset" flag).
};
/**
 * @brief Mesh cell: a triangle, or a quadrilateral when #v3 is nonnegative.
 *
 * Corner indices are in cyclic order and refer to the owning vertex array (for example
 * MeshResult::vertices, or PatchMesh::xy for a patch). Nothing here checks index ranges,
 * distinctness, orientation or geometry.
 */
struct MeshTriangle {
    /// @var v0
    /// @brief First corner index.
    /// @var v1
    /// @brief Second corner index.
    /// @var v2
    /// @brief Third corner index.
    /// @var v3
    /// @brief Fourth corner index of a quadrilateral; a negative value (default -1) marks a
    /// triangle.
    int v0 = 0, v1 = 0, v2 = 0, v3 = -1;
    Text tag; ///< Cell label (for example the patch or region tag); empty when untagged.
    /** @brief Return true when #v3 is nonnegative (a quadrilateral); indices are not checked. */
    bool isQuad() const { return v3 >= 0; }
    /** @brief Return 4 for a quadrilateral and 3 for a triangle. */
    int vertexCount() const { return isQuad() ? 4 : 3; }
    /**
     * @brief Corner index stored in slot @p k.
     * @param[in] k Slot 0..3.
     * @return #v0, #v1, #v2 or #v3 for slots 0..3 (slot 3 of a triangle returns its negative
     * sentinel); -1 for any other @p k.
     */
    int vertex(int k) const { return k == 0 ? v0 : k == 1 ? v1 : k == 2 ? v2 : k == 3 ? v3 : -1; }
    /**
     * @brief Store a corner index in one slot, leaving the other fields unchanged.
     * @param[in] k Slot 0..3; any other value is ignored.
     * @param[in] v Index to store, not validated. A nonnegative value in slot 3 makes the cell a
     * quadrilateral and a negative one a triangle.
     */
    void setVertex(int k, int v)
    {
        if (k == 0)
            v0 = v;
        else if (k == 1)
            v1 = v;
        else if (k == 2)
            v2 = v;
        else if (k == 3)
            v3 = v;
    }
};
/** @brief Mesh edge between two vertex indices, carrying a source marker. */
struct MeshEdge {
    /// @var v0
    /// @brief First endpoint index into the owning vertex array.
    /// @var v1
    /// @brief Second endpoint index into the owning vertex array.
    /// @var marker
    /// @brief Marker of the source constraint the edge came from; 0 (default) means none.
    int v0 = 0, v1 = 0, marker = 0;
    Text tag; ///< Optional label; the native planning and audit code does not read it.
};
/**
 * @brief Read-only array that either owns its elements or reads them from borrowed storage.
 *
 * Until borrow() is given a non-null owner, size(), operator[] and iteration use the owned
 * Vector<T> elements. After borrow() they report the borrowed count instead and fetch each
 * element by value through the supplied read function, so a container of another element type
 * can be viewed without copying it. Members inherited from Vector<T> (first(), data(),
 * append(), ...) always act on the owned elements, as does any access through a Vector<T>
 * reference, because size() and operator[] hide rather than override the base members.
 *
 * @tparam T Element type, returned by value.
 */
template <class T> class ReadArray : public Vector<T>
{
    const void *owner_ = nullptr;
    T (*read_)(const void *, std::size_t) = nullptr;
    std::ptrdiff_t count_ = 0;

public:
    using Vector<T>::Vector;
    /**
     * @brief View external storage instead of the owned elements, which are left unchanged.
     * @param[in] owner Opaque pointer passed back to @p read; not owned. It must stay valid and
     * unchanged for as long as this array is read. nullptr returns to the owned elements.
     * @param[in] count Number of borrowed elements reported by size(); must be >= 0.
     * @param[in] read Function returning element `i` of @p owner by value. It is called on every
     * element access without a bounds check and must be non-null when @p owner is non-null.
     */
    void borrow(const void *owner, std::ptrdiff_t count, T (*read)(const void *, std::size_t))
    {
        owner_ = owner;
        count_ = count;
        read_ = read;
    }
    /** @brief Borrowed count while borrowing, otherwise the owned element count. */
    std::ptrdiff_t size() const { return owner_ ? count_ : Vector<T>::size(); }
    /** @brief Return true when size() is 0. */
    bool isEmpty() const { return size() == 0; }
    /**
     * @brief Element @p i, from the borrowed storage or the owned elements.
     * @param[in] i Index in [0, size()); not bounds-checked.
     * @return A copy of the element.
     */
    T operator[](std::size_t i) const
    {
        return owner_ ? read_(owner_, i) : Vector<T>::operator[](i);
    }
    /**
     * @brief Minimal iterator for range-based for loops: supports only `*`, prefix `++` and
     * `!=`, and yields elements by value through operator[].
     */
    struct Iterator {
        const ReadArray *array; ///< Array being iterated; not owned.
        std::size_t i;          ///< Current element index.
        /** @brief Current element, by value. */
        T operator*() const { return (*array)[i]; }
        /**
         * @brief Advance to the next index.
         * @return This iterator.
         */
        Iterator &operator++()
        {
            ++i;
            return *this;
        }
        /**
         * @brief Compare positions only.
         * @param[in] b Iterator to compare with.
         * @return True when the indices differ; the arrays are not compared.
         */
        bool operator!=(const Iterator &b) const { return i != b.i; }
    };
    /** @brief Iterator at index 0. */
    Iterator begin() const { return {this, 0}; }
    /** @brief Iterator at index size(). */
    Iterator end() const { return {this, std::size_t(size())}; }
};
/**
 * @brief Mesh as read by the native audits: vertices, cells, marked edges and a status.
 *
 * Each array may own its elements or borrow a caller's storage (see ReadArray); borrowed
 * storage must outlive every read of this object.
 */
struct MeshResult {
    ReadArray<MeshVertex> vertices;    ///< Positions (XY units) and elevations (Z units).
    ReadArray<MeshTriangle> triangles; ///< Every cell, triangles and quads, indexing #vertices.
    /// Constraint and boundary edges with their source markers, indexing #vertices.
    ReadArray<MeshEdge> boundaryEdges;
    /// True for a successful mesh (default false); the constraint audit refuses a mesh with
    /// ok false.
    bool ok = false;
    Text errorMsg; ///< Failure description that accompanies #ok == false.
};
/**
 * @brief Constraint polyline with marker, tag and optional strip width.
 *
 * Native mirror of the GUI generator's constraint record. The native size field reads only
 * #path; the other fields are carried for the GUI adapter.
 */
struct ConstraintSegment {
    Vector<Point2> path; ///< Polyline vertices, XY units; each consecutive pair is one segment.
    int marker = 0;      ///< Source marker kept on the generated edges; 0 means none.
    Text tag;            ///< Caller-resolved label for the marker; not interpreted here.
    /// Width (XY units) of a swept quad strip requested along an open path; a value that is
    /// not > 0 means a plain constraint. Read by the GUI MeshGenerator only.
    double stripWidth = 0;
};
/** @brief Mandatory mesh point with marker, tag and optional pre-sampled elevation. */
struct SteinerPoint {
    Point2 xy;      ///< Position, XY units.
    int marker = 0; ///< Point marker; 0 means none. The native size field seeds only nonzero ones.
    Text tag;       ///< Caller-resolved label; not interpreted by the native code.
    double z = 0;   ///< Elevation in Z units; meaningful only when #hasZ is true.
    /// True when #z holds a pre-sampled elevation. Metadata only: MeshKit stores it but does
    /// not apply it as an elevation override.
    bool hasZ = false;
};
/**
 * @brief Test whether a cell is a strictly convex quadrilateral; triangles always pass.
 * @param[in] v Vertex array that @p c indexes; indices are not range-checked.
 * @param[in] c Cell to test.
 * @return True for a triangle without inspecting it. For a quadrilateral, true when all four
 * corner turns are finite, nonzero and of one sign (either winding), computed in plain double
 * arithmetic; collinear or reflex corners and non-finite coordinates give false.
 */
inline bool cellIsConvex(const Vector<MeshVertex> &v, const MeshTriangle &c)
{
    if (!c.isQuad())
        return true;
    double before = 0;
    for (int i = 0; i < 4; ++i) {
        auto a = v[c.vertex(i)].xy, b = v[c.vertex((i + 1) % 4)].xy,
             d = v[c.vertex((i + 2) % 4)].xy;
        auto ab = b - a, bd = d - b;
        double turn = ab.x() * bd.y() - ab.y() * bd.x();
        if (!std::isfinite(turn) || turn == 0 || (before && (turn > 0) != (before > 0)))
            return false;
        before = turn;
    }
    return true;
}
}
