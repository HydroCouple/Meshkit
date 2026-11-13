// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QHash>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QVector>
namespace meshkit::detail {
/**
 * @brief Storage policy that instantiates meshkit::BasicConstrainedDelaunay with Qt types
 * (mesh::ConstrainedDelaunay). Stateless: type aliases plus two static message builders.
 */
struct QtStorage {
    /** @typedef Point
     *  2D point type, XY plane units. */
    /** @typedef Box
     *  Axis-aligned XY rectangle passed to footprint callbacks. */
    /** @typedef String
     *  Error text type returned by errorMsg(). */
    /** @typedef PointInput
     *  Point container accepted by build(). */
    using Point=QPointF;using Box=QRectF;using String=QString;using PointInput=QVector<QPointF>;
    /// Dynamic array type for owned arrays and read-out buffers.
    template<class T> using Vector=QVector<T>;
    /// Hash map type for internal segment bookkeeping.
    template<class K,class V> using Hash=QHash<K,V>;
    /// Hash multimap type for overlapping-segment bookkeeping.
    template<class K,class V> using MultiHash=QMultiHash<K,V>;
    /// Hash set type (protected vertices, fixed subsegments).
    template<class K> using Set=QSet<K>;
    /**
     * @brief Convert a kernel diagnostic to the storage's text type.
     * @param[in] s Null-terminated Latin-1 literal from the kernel (e.g. "cdt: cancelled").
     * @return The text as an owned QString.
     */
    static String message(const char *s){return QString::fromLatin1(s);}
    /**
     * @brief Build the error text for a non-finite input point.
     * @param[in] i Zero-based index of the offending point in the build() input.
     * @return "cdt: non-finite input point <i>".
     */
    static String invalidPointMessage(int i){return QStringLiteral("cdt: non-finite input point %1").arg(i);}
};
}
