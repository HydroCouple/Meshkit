// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "mesh/cellplanner.h"
#include "mesh/constraintsimplifier.h"
#include "mesh/constraintaudit.h"
#include "mesh/sizefield.h"
#include "mesh/quadblocks.h"
#include "meshkit/planning.hpp"
#include "meshkit/constraints.hpp"
#include "meshkit/constraintaudit.hpp"
#include "meshkit/blocks.hpp"
#include "meshkit/sizefield.hpp"
namespace mesh::native_adapter
{
// Short alias for meshkit::plan, the namespace of the standard-container types.
namespace n = meshkit::plan;
/** @brief Copy the x and y of a QPointF into a meshkit::Point2. */
inline meshkit::Point2 native(QPointF p)
{
    return {p.x(), p.y()};
}
/** @brief Copy the x and y of a meshkit::Point2 into a QPointF. */
inline QPointF qt(meshkit::Point2 p)
{
    return {p.x(), p.y()};
}
/**
 * @brief Convert a QRectF to a meshkit::Box2 spanning the same two corners.
 *
 * (left(), top()) becomes the box origin and (right(), bottom()) its opposite corner, with no
 * y-axis flip and no normalization, so a negative width or height is kept. The size is recomputed
 * from the corners and can differ from the source by rounding.
 * @param[in] b Rectangle to convert.
 * @return The box.
 */
inline meshkit::Box2 native(QRectF b)
{
    return {{b.left(), b.top()}, {b.right(), b.bottom()}};
}
/**
 * @brief Convert a meshkit::Box2 to a QRectF spanning the same two corners.
 *
 * Inverse of native(QRectF): (left(), top()) and (right(), bottom()) become the rectangle's corners
 * without normalization, and the size is recomputed from them.
 * @param[in] b Box to convert.
 * @return The rectangle.
 */
inline QRectF qt(meshkit::Box2 b)
{
    return QRectF(QPointF(b.left(), b.top()), QPointF(b.right(), b.bottom()));
}
/**
 * @brief Convert a QString to meshkit::plan::Text holding its UTF-8 encoding.
 * @param[in] s String to convert.
 * @return The UTF-8 bytes of @p s (QString::toStdString()).
 */
inline n::Text native(const QString &s)
{
    return s.toStdString();
}
/**
 * @brief Convert meshkit::plan::Text to a QString by decoding its bytes as UTF-8.
 *
 * Uses QString::fromStdString(). Byte sequences that are not valid UTF-8 are replaced by the
 * decoder, so Text survives a qt() and native() round trip only when it is valid UTF-8.
 * @param[in] s Text to convert.
 * @return The decoded string.
 */
inline QString qt(const n::Text &s)
{
    return QString::fromStdString(s);
}
/**
 * @brief Fallback that returns a value unchanged, for members whose type needs no conversion.
 *
 * Overload resolution prefers the specific overloads and the QVector template, so this handles
 * arithmetic values, bool, std::optional, std::function and std::pair members. Same-width integer
 * types with different names (quint64 and std::uint64_t, qsizetype and std::ptrdiff_t) convert
 * implicitly where the result is assigned.
 * @tparam T Type of the value.
 * @param[in] v Value to return.
 * @return A copy of @p v.
 */
template <class T> T native(T v)
{
    return v;
}
/**
 * @brief Fallback that returns a value unchanged, for members whose type needs no conversion.
 *
 * Mirror of the native() fallback: overload resolution prefers the specific overloads and the
 * meshkit::plan::Vector template, so this handles arithmetic values, bool, std::optional,
 * std::function and std::pair members.
 * @tparam T Type of the value.
 * @param[in] v Value to return.
 * @return A copy of @p v.
 */
template <class T> T qt(T v)
{
    return v;
}
inline n::QuadRegion native(const QuadRegion &);
inline QuadRegion qt(const n::QuadRegion &);
inline n::StructuredPatch native(const StructuredPatch &);
inline StructuredPatch qt(const n::StructuredPatch &);
inline n::SweptPatch native(const SweptPatch &);
inline SweptPatch qt(const n::SweptPatch &);
inline n::BankPairPatch native(const BankPairPatch &);
inline BankPairPatch qt(const n::BankPairPatch &);
inline n::PatchLine native(const PatchLine &);
inline PatchLine qt(const n::PatchLine &);
inline n::PatchMesh native(const PatchMesh &);
inline PatchMesh qt(const n::PatchMesh &);
inline n::ConstraintSegment native(const ConstraintSegment &);
inline ConstraintSegment qt(const n::ConstraintSegment &);
inline n::SteinerPoint native(const SteinerPoint &);
inline SteinerPoint qt(const n::SteinerPoint &);
inline n::ConstraintSimplificationPolicy native(const ConstraintSimplificationPolicy &);
inline ConstraintSimplificationPolicy qt(const n::ConstraintSimplificationPolicy &);
inline n::ConstraintPolicyOverride native(const ConstraintPolicyOverride &);
inline ConstraintPolicyOverride qt(const n::ConstraintPolicyOverride &);
inline n::SimplificationFeature native(const SimplificationFeature &);
inline SimplificationFeature qt(const n::SimplificationFeature &);
inline n::SimplificationChange native(const SimplificationChange &);
inline SimplificationChange qt(const n::SimplificationChange &);
inline n::ConstraintSimplificationOptions native(const ConstraintSimplificationOptions &);
inline ConstraintSimplificationOptions qt(const n::ConstraintSimplificationOptions &);
inline n::ConstraintSimplificationResult native(const ConstraintSimplificationResult &);
inline ConstraintSimplificationResult qt(const n::ConstraintSimplificationResult &);
inline n::ConstraintAuditOptions native(const ConstraintAuditOptions &);
inline ConstraintAuditOptions qt(const n::ConstraintAuditOptions &);
inline n::ConstraintAuditExample native(const ConstraintAuditExample &);
inline ConstraintAuditExample qt(const n::ConstraintAuditExample &);
inline n::ConstraintAuditReport native(const ConstraintAuditReport &);
inline ConstraintAuditReport qt(const n::ConstraintAuditReport &);
inline n::PatchDecision native(const PatchDecision &);
inline PatchDecision qt(const n::PatchDecision &);
inline n::PatchAssessment native(const PatchAssessment &);
inline PatchAssessment qt(const n::PatchAssessment &);
inline n::PatchInterfaceConflict native(const PatchInterfaceConflict &);
inline PatchInterfaceConflict qt(const n::PatchInterfaceConflict &);
inline n::PatchInterfaceReport native(const PatchInterfaceReport &);
inline PatchInterfaceReport qt(const n::PatchInterfaceReport &);
inline n::QuadPlanningGeometry native(const QuadPlanningGeometry &);
inline QuadPlanningGeometry qt(const n::QuadPlanningGeometry &);
inline n::QuadPlan native(const QuadPlan &);
inline QuadPlan qt(const n::QuadPlan &);
inline n::QuadBlockGrid native(const QuadBlockGrid &);
inline QuadBlockGrid qt(const n::QuadBlockGrid &);
inline n::QuadBlockOptions native(const QuadBlockOptions &);
inline QuadBlockOptions qt(const n::QuadBlockOptions &);
/**
 * @brief Convert a MeshTriangle to the native cell, keeping its corners and tag.
 *
 * The Qt-only cell attributes (mannings, initDepth, initHgl, initU, initV) have no native
 * counterpart and are dropped.
 * @param[in] v Cell to convert.
 * @return Native cell with the same v0..v3 and the tag as UTF-8 Text.
 */
inline n::MeshTriangle native(const MeshTriangle &v)
{
    n::MeshTriangle out;
    out.v0 = v.v0;
    out.v1 = v.v1;
    out.v2 = v.v2;
    out.v3 = v.v3;
    out.tag = native(v.tag);
    return out;
}
/**
 * @brief Convert a native cell to a MeshTriangle, copying its corners and tag.
 *
 * The Qt-only attributes keep their defaults: mannings, initDepth and initHgl are NaN (unset) and
 * initU and initV are 0, so a Qt to native to Qt round trip loses any values set on them.
 * @param[in] v Cell to convert.
 * @return The Qt cell.
 */
inline MeshTriangle qt(const n::MeshTriangle &v)
{
    MeshTriangle out;
    out.v0 = v.v0;
    out.v1 = v.v1;
    out.v2 = v.v2;
    out.v3 = v.v3;
    out.tag = qt(v.tag);
    return out;
}
inline n::Polygon native(const QPolygonF &);
inline QPolygonF qt(const n::Polygon &);
/**
 * @brief Convert a QVector element by element with the native() overload for its element type.
 *
 * Nested QVectors convert recursively and QPolygonF elements become meshkit::plan::Polygon. Only
 * overloads declared before this template are visible to it, which is why the struct conversions
 * are forward-declared above.
 * @tparam T Qt element type. It must be default-constructible, because the native element type is
 * deduced from native(T{}).
 * @param[in] v Vector to convert; each element is copied.
 * @return meshkit::plan::Vector of the converted elements, in the same order.
 */
template <class T> auto native(const QVector<T> &v)
{
    n::Vector<decltype(native(T{}))> out;
    out.reserve(v.size());
    for (int i = 0; i < v.size(); ++i) {
        const T value = v[i];
        out.append(native(value));
    }
    return out;
}
/**
 * @brief Convert a meshkit::plan::Vector element by element with the qt() overload for its element
 * type.
 *
 * Nested vectors convert recursively and meshkit::plan::Polygon elements become QPolygonF. As for
 * the native() template, only overloads declared before this template are visible to it.
 * @tparam T Native element type. It must be default-constructible, because the Qt element type is
 * deduced from qt(T{}).
 * @param[in] v Vector to convert; each element is copied.
 * @return QVector of the converted elements, in the same order.
 */
template <class T> auto qt(const n::Vector<T> &v)
{
    QVector<decltype(qt(T{}))> out;
    out.reserve(v.size());
    for (int i = 0; i < v.size(); ++i) {
        const T value = v[i];
        out.append(qt(value));
    }
    return out;
}
/**
 * @brief Copy the points of a QPolygonF, in order, into a meshkit::plan::Polygon.
 *
 * Points are copied as given: a repeated closing point is kept and nothing is reoriented or
 * deduplicated.
 * @param[in] v Polygon to convert.
 * @return The native polygon.
 */
inline n::Polygon native(const QPolygonF &v)
{
    n::Polygon out;
    out.reserve(v.size());
    for (auto p : v)
        out.append(native(p));
    return out;
}
/** @brief Copy the points of a meshkit::plan::Polygon, in order, into a QPolygonF. */
inline QPolygonF qt(const n::Polygon &v)
{
    return QPolygonF(qt(static_cast<const n::Vector<meshkit::Point2> &>(v)));
}
/** @brief Cast to meshkit::plan::QuadRegionMode, whose enumerators match in name and order. */
inline n::QuadRegionMode native(QuadRegionMode v)
{
    return static_cast<n::QuadRegionMode>(v);
}
/** @brief Cast to QuadRegionMode, whose enumerators match the native enum in name and order. */
inline QuadRegionMode qt(n::QuadRegionMode v)
{
    return static_cast<QuadRegionMode>(v);
}
/** @brief Cast to meshkit::plan::PatchDisposition, whose enumerators match in name and order. */
inline n::PatchDisposition native(PatchDisposition v)
{
    return static_cast<n::PatchDisposition>(v);
}
/** @brief Cast to PatchDisposition, whose enumerators match the native enum in name and order. */
inline PatchDisposition qt(n::PatchDisposition v)
{
    return static_cast<PatchDisposition>(v);
}
/**
 * @brief Cast to meshkit::plan::ConstraintSimplificationPolicy::Mode, whose enumerators match in
 * name and order.
 */
inline n::ConstraintSimplificationPolicy::Mode native(ConstraintSimplificationPolicy::Mode v)
{
    return static_cast<n::ConstraintSimplificationPolicy::Mode>(v);
}
/**
 * @brief Cast to ConstraintSimplificationPolicy::Mode, whose enumerators match the native enum in
 * name and order.
 */
inline ConstraintSimplificationPolicy::Mode qt(n::ConstraintSimplificationPolicy::Mode v)
{
    return static_cast<ConstraintSimplificationPolicy::Mode>(v);
}
/**
 * @brief Cast to meshkit::plan::SimplificationFeature::Role, whose enumerators match in name and
 * order.
 */
inline n::SimplificationFeature::Role native(SimplificationFeature::Role v)
{
    return static_cast<n::SimplificationFeature::Role>(v);
}
/**
 * @brief Cast to SimplificationFeature::Role, whose enumerators match the native enum in name and
 * order.
 */
inline SimplificationFeature::Role qt(n::SimplificationFeature::Role v)
{
    return static_cast<SimplificationFeature::Role>(v);
}
/**
 * @brief Copy every field of a QuadRegion into a meshkit::plan::QuadRegion.
 *
 * meshkit::plan::QuadRegion::attribute, which has no Qt counterpart, is left at 0.
 * @param[in] v Region to convert.
 * @return The native region.
 */
inline n::QuadRegion native(const QuadRegion &v)
{
    n::QuadRegion out;
    out.ring = native(v.ring);
    out.holes = native(v.holes);
    out.isBackground = native(v.isBackground);
    out.mode = native(v.mode);
    out.requireMapped = native(v.requireMapped);
    out.automaticCandidate = native(v.automaticCandidate);
    out.spacing = native(v.spacing);
    out.directionalSpacing = native(v.directionalSpacing);
    out.hAlong = native(v.hAlong);
    out.hAcross = native(v.hAcross);
    out.mappedAlongAngleDeg = native(v.mappedAlongAngleDeg);
    out.aspectMax = native(v.aspectMax);
    out.hasAlignAngle = native(v.hasAlignAngle);
    out.alignAngleDeg = native(v.alignAngleDeg);
    out.alignGuide = native(v.alignGuide);
    out.corners = native(v.corners);
    out.tag = native(v.tag);
    return out;
}
/**
 * @brief Copy a meshkit::plan::QuadRegion into a QuadRegion; the native-only attribute is dropped.
 */
inline QuadRegion qt(const n::QuadRegion &v)
{
    QuadRegion out;
    out.ring = qt(v.ring);
    out.holes = qt(v.holes);
    out.isBackground = qt(v.isBackground);
    out.mode = qt(v.mode);
    out.requireMapped = qt(v.requireMapped);
    out.automaticCandidate = qt(v.automaticCandidate);
    out.spacing = qt(v.spacing);
    out.directionalSpacing = qt(v.directionalSpacing);
    out.hAlong = qt(v.hAlong);
    out.hAcross = qt(v.hAcross);
    out.mappedAlongAngleDeg = qt(v.mappedAlongAngleDeg);
    out.aspectMax = qt(v.aspectMax);
    out.hasAlignAngle = qt(v.hasAlignAngle);
    out.alignAngleDeg = qt(v.alignAngleDeg);
    out.alignGuide = qt(v.alignGuide);
    out.corners = qt(v.corners);
    out.tag = qt(v.tag);
    return out;
}
/** @brief Copy every field of a StructuredPatch into a meshkit::plan::StructuredPatch. */
inline n::StructuredPatch native(const StructuredPatch &v)
{
    n::StructuredPatch out;
    out.corners = native(v.corners);
    out.n = native(v.n);
    out.m = native(v.m);
    out.tag = native(v.tag);
    return out;
}
/** @brief Copy every field of a meshkit::plan::StructuredPatch into a StructuredPatch. */
inline StructuredPatch qt(const n::StructuredPatch &v)
{
    StructuredPatch out;
    out.corners = qt(v.corners);
    out.n = qt(v.n);
    out.m = qt(v.m);
    out.tag = qt(v.tag);
    return out;
}
/** @brief Copy every field of a SweptPatch into a meshkit::plan::SweptPatch. */
inline n::SweptPatch native(const SweptPatch &v)
{
    n::SweptPatch out;
    out.centreline = native(v.centreline);
    out.width = native(v.width);
    out.across = native(v.across);
    out.along = native(v.along);
    out.tag = native(v.tag);
    return out;
}
/** @brief Copy every field of a meshkit::plan::SweptPatch into a SweptPatch. */
inline SweptPatch qt(const n::SweptPatch &v)
{
    SweptPatch out;
    out.centreline = qt(v.centreline);
    out.width = qt(v.width);
    out.across = qt(v.across);
    out.along = qt(v.along);
    out.tag = qt(v.tag);
    return out;
}
/** @brief Copy every field of a BankPairPatch into a meshkit::plan::BankPairPatch. */
inline n::BankPairPatch native(const BankPairPatch &v)
{
    n::BankPairPatch out;
    out.bankA = native(v.bankA);
    out.bankB = native(v.bankB);
    out.across = native(v.across);
    out.along = native(v.along);
    out.tag = native(v.tag);
    return out;
}
/** @brief Copy every field of a meshkit::plan::BankPairPatch into a BankPairPatch. */
inline BankPairPatch qt(const n::BankPairPatch &v)
{
    BankPairPatch out;
    out.bankA = qt(v.bankA);
    out.bankB = qt(v.bankB);
    out.across = qt(v.across);
    out.along = qt(v.along);
    out.tag = qt(v.tag);
    return out;
}
/** @brief Copy every field of a PatchLine into a meshkit::plan::PatchLine. */
inline n::PatchLine native(const PatchLine &v)
{
    n::PatchLine out;
    out.path = native(v.path);
    out.marker = native(v.marker);
    out.tag = native(v.tag);
    return out;
}
/** @brief Copy every field of a meshkit::plan::PatchLine into a PatchLine. */
inline PatchLine qt(const n::PatchLine &v)
{
    PatchLine out;
    out.path = qt(v.path);
    out.marker = qt(v.marker);
    out.tag = qt(v.tag);
    return out;
}
/**
 * @brief Copy every field of a PatchMesh into a meshkit::plan::PatchMesh.
 *
 * Each quad goes through native(const MeshTriangle &), which drops the Qt-only cell attributes.
 * @param[in] v Patch to convert.
 * @return The native patch.
 */
inline n::PatchMesh native(const PatchMesh &v)
{
    n::PatchMesh out;
    out.xy = native(v.xy);
    out.quads = native(v.quads);
    out.boundarySegments = native(v.boundarySegments);
    out.tag = native(v.tag);
    out.lines = native(v.lines);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::PatchMesh into a PatchMesh.
 *
 * Each quad goes through qt(const n::MeshTriangle &), so its Qt-only cell attributes take their
 * defaults.
 * @param[in] v Patch to convert.
 * @return The Qt patch.
 */
inline PatchMesh qt(const n::PatchMesh &v)
{
    PatchMesh out;
    out.xy = qt(v.xy);
    out.quads = qt(v.quads);
    out.boundarySegments = qt(v.boundarySegments);
    out.tag = qt(v.tag);
    out.lines = qt(v.lines);
    return out;
}
/** @brief Copy every field of a ConstraintSegment into a meshkit::plan::ConstraintSegment. */
inline n::ConstraintSegment native(const ConstraintSegment &v)
{
    n::ConstraintSegment out;
    out.path = native(v.path);
    out.marker = native(v.marker);
    out.tag = native(v.tag);
    out.stripWidth = native(v.stripWidth);
    return out;
}
/** @brief Copy every field of a meshkit::plan::ConstraintSegment into a ConstraintSegment. */
inline ConstraintSegment qt(const n::ConstraintSegment &v)
{
    ConstraintSegment out;
    out.path = qt(v.path);
    out.marker = qt(v.marker);
    out.tag = qt(v.tag);
    out.stripWidth = qt(v.stripWidth);
    return out;
}
/** @brief Copy every field of a SteinerPoint into a meshkit::plan::SteinerPoint. */
inline n::SteinerPoint native(const SteinerPoint &v)
{
    n::SteinerPoint out;
    out.xy = native(v.xy);
    out.marker = native(v.marker);
    out.tag = native(v.tag);
    out.z = native(v.z);
    out.hasZ = native(v.hasZ);
    return out;
}
/** @brief Copy every field of a meshkit::plan::SteinerPoint into a SteinerPoint. */
inline SteinerPoint qt(const n::SteinerPoint &v)
{
    SteinerPoint out;
    out.xy = qt(v.xy);
    out.marker = qt(v.marker);
    out.tag = qt(v.tag);
    out.z = qt(v.z);
    out.hasZ = qt(v.hasZ);
    return out;
}
/**
 * @brief Copy every field of a ConstraintSimplificationPolicy into a
 * meshkit::plan::ConstraintSimplificationPolicy.
 */
inline n::ConstraintSimplificationPolicy native(const ConstraintSimplificationPolicy &v)
{
    n::ConstraintSimplificationPolicy out;
    out.mode = native(v.mode);
    out.maxDeviation = native(v.maxDeviation);
    out.maxZDeviation = native(v.maxZDeviation);
    out.maxTurnDegrees = native(v.maxTurnDegrees);
    out.maxAreaChangeFraction = native(v.maxAreaChangeFraction);
    out.maxAreaChange = native(v.maxAreaChange);
    out.minimumClearance = native(v.minimumClearance);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintSimplificationPolicy into a
 * ConstraintSimplificationPolicy.
 */
inline ConstraintSimplificationPolicy qt(const n::ConstraintSimplificationPolicy &v)
{
    ConstraintSimplificationPolicy out;
    out.mode = qt(v.mode);
    out.maxDeviation = qt(v.maxDeviation);
    out.maxZDeviation = qt(v.maxZDeviation);
    out.maxTurnDegrees = qt(v.maxTurnDegrees);
    out.maxAreaChangeFraction = qt(v.maxAreaChangeFraction);
    out.maxAreaChange = qt(v.maxAreaChange);
    out.minimumClearance = qt(v.minimumClearance);
    return out;
}
/**
 * @brief Copy every field of a ConstraintPolicyOverride into a
 * meshkit::plan::ConstraintPolicyOverride.
 */
inline n::ConstraintPolicyOverride native(const ConstraintPolicyOverride &v)
{
    n::ConstraintPolicyOverride out;
    out.mode = native(v.mode);
    out.maxDeviation = native(v.maxDeviation);
    out.maxZDeviation = native(v.maxZDeviation);
    out.maxTurnDegrees = native(v.maxTurnDegrees);
    out.maxAreaChangeFraction = native(v.maxAreaChangeFraction);
    out.minimumClearance = native(v.minimumClearance);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintPolicyOverride into a
 * ConstraintPolicyOverride.
 */
inline ConstraintPolicyOverride qt(const n::ConstraintPolicyOverride &v)
{
    ConstraintPolicyOverride out;
    out.mode = qt(v.mode);
    out.maxDeviation = qt(v.maxDeviation);
    out.maxZDeviation = qt(v.maxZDeviation);
    out.maxTurnDegrees = qt(v.maxTurnDegrees);
    out.maxAreaChangeFraction = qt(v.maxAreaChangeFraction);
    out.minimumClearance = qt(v.minimumClearance);
    return out;
}
/**
 * @brief Copy every field of a SimplificationFeature into a meshkit::plan::SimplificationFeature.
 */
inline n::SimplificationFeature native(const SimplificationFeature &v)
{
    n::SimplificationFeature out;
    out.id = native(v.id);
    out.xy = native(v.xy);
    out.z = native(v.z);
    out.fixed = native(v.fixed);
    out.closed = native(v.closed);
    out.policy = native(v.policy);
    out.role = native(v.role);
    out.marker = native(v.marker);
    out.requiredInMesh = native(v.requiredInMesh);
    out.label = native(v.label);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::SimplificationFeature into a SimplificationFeature.
 */
inline SimplificationFeature qt(const n::SimplificationFeature &v)
{
    SimplificationFeature out;
    out.id = qt(v.id);
    out.xy = qt(v.xy);
    out.z = qt(v.z);
    out.fixed = qt(v.fixed);
    out.closed = qt(v.closed);
    out.policy = qt(v.policy);
    out.role = qt(v.role);
    out.marker = qt(v.marker);
    out.requiredInMesh = qt(v.requiredInMesh);
    out.label = qt(v.label);
    return out;
}
/** @brief Copy every field of a SimplificationChange into a meshkit::plan::SimplificationChange. */
inline n::SimplificationChange native(const SimplificationChange &v)
{
    n::SimplificationChange out;
    out.id = native(v.id);
    out.removed = native(v.removed);
    out.rejectedShortcuts = native(v.rejectedShortcuts);
    out.maxDeviation = native(v.maxDeviation);
    out.maxZDeviation = native(v.maxZDeviation);
    out.areaChange = native(v.areaChange);
    out.budgetRetained = native(v.budgetRetained);
    out.rolledBack = native(v.rolledBack);
    out.sourceVertices = native(v.sourceVertices);
    return out;
}
/** @brief Copy every field of a meshkit::plan::SimplificationChange into a SimplificationChange. */
inline SimplificationChange qt(const n::SimplificationChange &v)
{
    SimplificationChange out;
    out.id = qt(v.id);
    out.removed = qt(v.removed);
    out.rejectedShortcuts = qt(v.rejectedShortcuts);
    out.maxDeviation = qt(v.maxDeviation);
    out.maxZDeviation = qt(v.maxZDeviation);
    out.areaChange = qt(v.areaChange);
    out.budgetRetained = qt(v.budgetRetained);
    out.rolledBack = qt(v.rolledBack);
    out.sourceVertices = qt(v.sourceVertices);
    return out;
}
/**
 * @brief Copy every field of a ConstraintSimplificationOptions into a
 * meshkit::plan::ConstraintSimplificationOptions.
 */
inline n::ConstraintSimplificationOptions native(const ConstraintSimplificationOptions &v)
{
    n::ConstraintSimplificationOptions out;
    out.protectedPoints = native(v.protectedPoints);
    out.cancelled = native(v.cancelled);
    out.workBudget = native(v.workBudget);
    out.maximumWorkspaceBytes = native(v.maximumWorkspaceBytes);
    out.canonicalizeSharedEdges = native(v.canonicalizeSharedEdges);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintSimplificationOptions into a
 * ConstraintSimplificationOptions.
 */
inline ConstraintSimplificationOptions qt(const n::ConstraintSimplificationOptions &v)
{
    ConstraintSimplificationOptions out;
    out.protectedPoints = qt(v.protectedPoints);
    out.cancelled = qt(v.cancelled);
    out.workBudget = qt(v.workBudget);
    out.maximumWorkspaceBytes = qt(v.maximumWorkspaceBytes);
    out.canonicalizeSharedEdges = qt(v.canonicalizeSharedEdges);
    return out;
}
/**
 * @brief Copy every field of a ConstraintSimplificationResult into a
 * meshkit::plan::ConstraintSimplificationResult.
 */
inline n::ConstraintSimplificationResult native(const ConstraintSimplificationResult &v)
{
    n::ConstraintSimplificationResult out;
    out.ok = native(v.ok);
    out.cancelled = native(v.cancelled);
    out.error = native(v.error);
    out.features = native(v.features);
    out.changes = native(v.changes);
    out.sharedSegments = native(v.sharedSegments);
    out.sharedChains = native(v.sharedChains);
    out.workUsed = native(v.workUsed);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintSimplificationResult into a
 * ConstraintSimplificationResult.
 */
inline ConstraintSimplificationResult qt(const n::ConstraintSimplificationResult &v)
{
    ConstraintSimplificationResult out;
    out.ok = qt(v.ok);
    out.cancelled = qt(v.cancelled);
    out.error = qt(v.error);
    out.features = qt(v.features);
    out.changes = qt(v.changes);
    out.sharedSegments = qt(v.sharedSegments);
    out.sharedChains = qt(v.sharedChains);
    out.workUsed = qt(v.workUsed);
    return out;
}
/**
 * @brief Copy every field of a ConstraintAuditOptions into a meshkit::plan::ConstraintAuditOptions.
 */
inline n::ConstraintAuditOptions native(const ConstraintAuditOptions &v)
{
    n::ConstraintAuditOptions out;
    out.protectedPoints = native(v.protectedPoints);
    out.cancelled = native(v.cancelled);
    out.workBudget = native(v.workBudget);
    out.maximumWorkspaceBytes = native(v.maximumWorkspaceBytes);
    out.maximumExamples = native(v.maximumExamples);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintAuditOptions into a ConstraintAuditOptions.
 */
inline ConstraintAuditOptions qt(const n::ConstraintAuditOptions &v)
{
    ConstraintAuditOptions out;
    out.protectedPoints = qt(v.protectedPoints);
    out.cancelled = qt(v.cancelled);
    out.workBudget = qt(v.workBudget);
    out.maximumWorkspaceBytes = qt(v.maximumWorkspaceBytes);
    out.maximumExamples = qt(v.maximumExamples);
    return out;
}
/**
 * @brief Copy every field of a ConstraintAuditExample into a meshkit::plan::ConstraintAuditExample.
 */
inline n::ConstraintAuditExample native(const ConstraintAuditExample &v)
{
    n::ConstraintAuditExample out;
    out.featureId = native(v.featureId);
    out.message = native(v.message);
    out.segment = native(v.segment);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintAuditExample into a ConstraintAuditExample.
 */
inline ConstraintAuditExample qt(const n::ConstraintAuditExample &v)
{
    ConstraintAuditExample out;
    out.featureId = qt(v.featureId);
    out.message = qt(v.message);
    out.segment = qt(v.segment);
    return out;
}
/**
 * @brief Copy every field of a ConstraintAuditReport into a meshkit::plan::ConstraintAuditReport.
 */
inline n::ConstraintAuditReport native(const ConstraintAuditReport &v)
{
    n::ConstraintAuditReport out;
    out.ok = native(v.ok);
    out.complete = native(v.complete);
    out.cancelled = native(v.cancelled);
    out.resourceLimited = native(v.resourceLimited);
    out.error = native(v.error);
    out.featuresChecked = native(v.featuresChecked);
    out.segmentsChecked = native(v.segmentsChecked);
    out.cellsChecked = native(v.cellsChecked);
    out.violations = native(v.violations);
    out.workUsed = native(v.workUsed);
    out.maximumXYDeviation = native(v.maximumXYDeviation);
    out.maximumZDeviation = native(v.maximumZDeviation);
    out.maximumAreaChange = native(v.maximumAreaChange);
    out.coordinateRoundoff = native(v.coordinateRoundoff);
    out.examples = native(v.examples);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::ConstraintAuditReport into a ConstraintAuditReport.
 */
inline ConstraintAuditReport qt(const n::ConstraintAuditReport &v)
{
    ConstraintAuditReport out;
    out.ok = qt(v.ok);
    out.complete = qt(v.complete);
    out.cancelled = qt(v.cancelled);
    out.resourceLimited = qt(v.resourceLimited);
    out.error = qt(v.error);
    out.featuresChecked = qt(v.featuresChecked);
    out.segmentsChecked = qt(v.segmentsChecked);
    out.cellsChecked = qt(v.cellsChecked);
    out.violations = qt(v.violations);
    out.workUsed = qt(v.workUsed);
    out.maximumXYDeviation = qt(v.maximumXYDeviation);
    out.maximumZDeviation = qt(v.maximumZDeviation);
    out.maximumAreaChange = qt(v.maximumAreaChange);
    out.coordinateRoundoff = qt(v.coordinateRoundoff);
    out.examples = qt(v.examples);
    return out;
}
/** @brief Copy every field of a PatchDecision into a meshkit::plan::PatchDecision. */
inline n::PatchDecision native(const PatchDecision &v)
{
    n::PatchDecision out;
    out.source = native(v.source);
    out.reason = native(v.reason);
    out.region = native(v.region);
    out.required = native(v.required);
    out.disposition = native(v.disposition);
    out.cells = native(v.cells);
    out.repairRounds = native(v.repairRounds);
    return out;
}
/** @brief Copy every field of a meshkit::plan::PatchDecision into a PatchDecision. */
inline PatchDecision qt(const n::PatchDecision &v)
{
    PatchDecision out;
    out.source = qt(v.source);
    out.reason = qt(v.reason);
    out.region = qt(v.region);
    out.required = qt(v.required);
    out.disposition = qt(v.disposition);
    out.cells = qt(v.cells);
    out.repairRounds = qt(v.repairRounds);
    return out;
}
/** @brief Copy every field of a PatchAssessment into a meshkit::plan::PatchAssessment. */
inline n::PatchAssessment native(const PatchAssessment &v)
{
    n::PatchAssessment out;
    out.ok = native(v.ok);
    out.repairable = native(v.repairable);
    out.cancelled = native(v.cancelled);
    out.reason = native(v.reason);
    return out;
}
/** @brief Copy every field of a meshkit::plan::PatchAssessment into a PatchAssessment. */
inline PatchAssessment qt(const n::PatchAssessment &v)
{
    PatchAssessment out;
    out.ok = qt(v.ok);
    out.repairable = qt(v.repairable);
    out.cancelled = qt(v.cancelled);
    out.reason = qt(v.reason);
    return out;
}
/**
 * @brief Copy every field of a PatchInterfaceConflict into a meshkit::plan::PatchInterfaceConflict.
 */
inline n::PatchInterfaceConflict native(const PatchInterfaceConflict &v)
{
    n::PatchInterfaceConflict out;
    out.first = native(v.first);
    out.second = native(v.second);
    out.firstSize = native(v.firstSize);
    out.secondSize = native(v.secondSize);
    return out;
}
/**
 * @brief Copy every field of a meshkit::plan::PatchInterfaceConflict into a PatchInterfaceConflict.
 */
inline PatchInterfaceConflict qt(const n::PatchInterfaceConflict &v)
{
    PatchInterfaceConflict out;
    out.first = qt(v.first);
    out.second = qt(v.second);
    out.firstSize = qt(v.firstSize);
    out.secondSize = qt(v.secondSize);
    return out;
}
/** @brief Copy every field of a PatchInterfaceReport into a meshkit::plan::PatchInterfaceReport. */
inline n::PatchInterfaceReport native(const PatchInterfaceReport &v)
{
    n::PatchInterfaceReport out;
    out.ok = native(v.ok);
    out.cancelled = native(v.cancelled);
    out.error = native(v.error);
    out.sharedFaces = native(v.sharedFaces);
    out.violations = native(v.violations);
    out.largestRatio = native(v.largestRatio);
    out.examples = native(v.examples);
    return out;
}
/** @brief Copy every field of a meshkit::plan::PatchInterfaceReport into a PatchInterfaceReport. */
inline PatchInterfaceReport qt(const n::PatchInterfaceReport &v)
{
    PatchInterfaceReport out;
    out.ok = qt(v.ok);
    out.cancelled = qt(v.cancelled);
    out.error = qt(v.error);
    out.sharedFaces = qt(v.sharedFaces);
    out.violations = qt(v.violations);
    out.largestRatio = qt(v.largestRatio);
    out.examples = qt(v.examples);
    return out;
}
/** @brief Copy every field of a QuadPlanningGeometry into a meshkit::plan::QuadPlanningGeometry. */
inline n::QuadPlanningGeometry native(const QuadPlanningGeometry &v)
{
    n::QuadPlanningGeometry out;
    out.domains = native(v.domains);
    out.holes = native(v.holes);
    out.lines = native(v.lines);
    out.pins = native(v.pins);
    out.fixedPatches = native(v.fixedPatches);
    return out;
}
/** @brief Copy every field of a meshkit::plan::QuadPlanningGeometry into a QuadPlanningGeometry. */
inline QuadPlanningGeometry qt(const n::QuadPlanningGeometry &v)
{
    QuadPlanningGeometry out;
    out.domains = qt(v.domains);
    out.holes = qt(v.holes);
    out.lines = qt(v.lines);
    out.pins = qt(v.pins);
    out.fixedPatches = qt(v.fixedPatches);
    return out;
}
/** @brief Copy every field of a QuadPlan into a meshkit::plan::QuadPlan. */
inline n::QuadPlan native(const QuadPlan &v)
{
    n::QuadPlan out;
    out.ok = native(v.ok);
    out.cancelled = native(v.cancelled);
    out.resourceLimited = native(v.resourceLimited);
    out.error = native(v.error);
    out.patches = native(v.patches);
    out.patchRegions = native(v.patchRegions);
    out.decisions = native(v.decisions);
    out.sharedSides = native(v.sharedSides);
    out.workUsed = native(v.workUsed);
    out.plannedCells = native(v.plannedCells);
    return out;
}
/** @brief Copy every field of a meshkit::plan::QuadPlan into a QuadPlan. */
inline QuadPlan qt(const n::QuadPlan &v)
{
    QuadPlan out;
    out.ok = qt(v.ok);
    out.cancelled = qt(v.cancelled);
    out.resourceLimited = qt(v.resourceLimited);
    out.error = qt(v.error);
    out.patches = qt(v.patches);
    out.patchRegions = qt(v.patchRegions);
    out.decisions = qt(v.decisions);
    out.sharedSides = qt(v.sharedSides);
    out.workUsed = qt(v.workUsed);
    out.plannedCells = qt(v.plannedCells);
    return out;
}
/** @brief Copy every field of a QuadBlockGrid into a meshkit::plan::QuadBlockGrid. */
inline n::QuadBlockGrid native(const QuadBlockGrid &v)
{
    n::QuadBlockGrid out;
    out.origin = native(v.origin);
    out.pitch = native(v.pitch);
    out.cols = native(v.cols);
    out.rows = native(v.rows);
    out.h = native(v.h);
    return out;
}
/** @brief Copy every field of a meshkit::plan::QuadBlockGrid into a QuadBlockGrid. */
inline QuadBlockGrid qt(const n::QuadBlockGrid &v)
{
    QuadBlockGrid out;
    out.origin = qt(v.origin);
    out.pitch = qt(v.pitch);
    out.cols = qt(v.cols);
    out.rows = qt(v.rows);
    out.h = qt(v.h);
    return out;
}
/** @brief Copy every field of a QuadBlockOptions into a meshkit::plan::QuadBlockOptions. */
inline n::QuadBlockOptions native(const QuadBlockOptions &v)
{
    n::QuadBlockOptions out;
    out.minSpacing = native(v.minSpacing);
    out.bandRatio = native(v.bandRatio);
    out.minQuadsPerSide = native(v.minQuadsPerSide);
    out.maxBlocks = native(v.maxBlocks);
    return out;
}
/** @brief Copy every field of a meshkit::plan::QuadBlockOptions into a QuadBlockOptions. */
inline QuadBlockOptions qt(const n::QuadBlockOptions &v)
{
    QuadBlockOptions out;
    out.minSpacing = qt(v.minSpacing);
    out.bandRatio = qt(v.bandRatio);
    out.minQuadsPerSide = qt(v.minQuadsPerSide);
    out.maxBlocks = qt(v.maxBlocks);
    return out;
}
/**
 * @brief View a MeshResult as a meshkit::plan::MeshResult without copying its arrays.
 *
 * ok and errorMsg are copied. vertices, triangles and boundaryEdges borrow the QVectors of @p m
 * (meshkit::plan::ReadArray::borrow()) and convert one element on every access, keeping only what
 * the native types hold: vertex xy and z; cell corners v0..v3, without the tag (unlike
 * native(const MeshTriangle &)); edge v0, v1 and marker, without the tag. All other vertex, cell
 * and mesh data (vertex markers, tags and coupling, Manning's n and initial conditions, cell
 * couplings, surface and infiltration data) is not visible. There is no qt() counterpart.
 * @param[in] m Mesh to view. It must outlive the result, and its three arrays must not change while
 * the result is read.
 * @return Native mesh that reads @p m's storage.
 */
inline n::MeshResult native(const MeshResult &m)
{
    n::MeshResult out;
    out.ok = m.ok;
    out.errorMsg = native(m.errorMsg);
    out.vertices.borrow(&m.vertices, m.vertices.size(), [](const void *p, std::size_t i) {
        const auto &v = (*static_cast<const QVector<MeshVertex> *>(p))[i];
        return n::MeshVertex{native(v.xy), v.z};
    });
    out.triangles.borrow(&m.triangles, m.triangles.size(), [](const void *p, std::size_t i) {
        const auto &v = (*static_cast<const QVector<MeshTriangle> *>(p))[i];
        n::MeshTriangle c;
        c.v0 = v.v0;
        c.v1 = v.v1;
        c.v2 = v.v2;
        c.v3 = v.v3;
        return c;
    });
    out.boundaryEdges.borrow(&m.boundaryEdges, m.boundaryEdges.size(),
                             [](const void *p, std::size_t i) {
                                 const auto &v = (*static_cast<const QVector<MeshEdge> *>(p))[i];
                                 n::MeshEdge e;
                                 e.v0 = v.v0;
                                 e.v1 = v.v1;
                                 e.marker = v.marker;
                                 return e;
                             });
    return out;
}
/**
 * @brief Copy SizeFieldOptions into meshkit::plan::SizeFieldOptions; one-way, with no qt()
 * counterpart.
 *
 * Every field is copied: the scalars and the terrainSizeAt callback as they are, each regions and
 * terrainRegions entry as its converted ring and h, and steps element by element.
 * @param[in] o Options to convert.
 * @return Native options holding copies of every field.
 */
inline n::SizeFieldOptions native(const SizeFieldOptions &o)
{
    n::SizeFieldOptions out;
    out.footprintBounds = o.footprintBounds;
    out.nearSize = o.nearSize;
    out.gradation = o.gradation;
    out.areaFloor = o.areaFloor;
    out.maxGridCells = o.maxGridCells;
    out.maxSize = o.maxSize;
    out.terrainSizeAt = o.terrainSizeAt;
    out.steps = native(o.steps);
    for (auto r : o.regions)
        out.regions.append({native(r.ring), r.h});
    for (auto r : o.terrainRegions)
        out.terrainRegions.append({native(r.ring), r.h});
    return out;
}
/**
 * @brief Convert PatchLimits to meshkit::plan::PatchLimits; one-way, with no qt() counterpart.
 *
 * The scalar limits and the elevationAt and cancelled callbacks are copied as they are. The
 * Qt-typed callbacks are wrapped only when set, so an empty one stays empty: maximumSize and
 * terrainToleranceForFootprint receive the native box converted with qt(meshkit::Box2), and
 * terrainBound receives the three native points copied into a QPointF array that lives only for the
 * call, with the elevation pointer passed through.
 * @param[in] o Limits to convert.
 * @return Native limits; each wrapper holds its own copy of the Qt callback.
 */
inline n::PatchLimits native(const PatchLimits &o)
{
    n::PatchLimits out;
    out.minimumEdge = o.minimumEdge;
    out.minimumAngleDegrees = o.minimumAngleDegrees;
    out.maximumAspect = o.maximumAspect;
    out.alongSize = o.alongSize;
    out.acrossSize = o.acrossSize;
    out.alongAngleDegrees = o.alongAngleDegrees;
    out.terrainTolerance = o.terrainTolerance;
    out.cancelled = o.cancelled;
    out.elevationAt = o.elevationAt;
    if (o.maximumSize)
        out.maximumSize = [f = o.maximumSize](const meshkit::Box2 &b) { return f(qt(b)); };
    if (o.terrainToleranceForFootprint)
        out.terrainToleranceForFootprint =
            [f = o.terrainToleranceForFootprint](const meshkit::Box2 &b) { return f(qt(b)); };
    if (o.terrainBound)
        out.terrainBound = [f = o.terrainBound](const meshkit::Point2 *p, const double *z) {
            QPointF points[3] = {qt(p[0]), qt(p[1]), qt(p[2])};
            return f(points, z);
        };
    return out;
}
/**
 * @brief Convert QuadPlanningOptions to meshkit::plan::QuadPlanningOptions; one-way, with no qt()
 * counterpart.
 *
 * The flags, sizes, rounds and budgets are copied, limits goes through native(const PatchLimits &),
 * and a set excluded callback is wrapped so that it receives each native candidate ring converted
 * to a QPolygonF; an empty one stays empty.
 * @param[in] o Options to convert.
 * @return Native options; the wrapper holds its own copy of the Qt callback.
 */
inline n::QuadPlanningOptions native(const QuadPlanningOptions &o)
{
    n::QuadPlanningOptions out;
    out.allowQuads = o.allowQuads;
    out.defaultSize = o.defaultSize;
    out.limits = native(o.limits);
    out.maximumRepairRounds = o.maximumRepairRounds;
    out.maximumCells = o.maximumCells;
    out.maximumPatchCells = o.maximumPatchCells;
    out.maximumWorkspaceBytes = o.maximumWorkspaceBytes;
    out.workBudget = o.workBudget;
    if (o.excluded)
        out.excluded = [f = o.excluded](const n::Polygon &p) { return f(qt(p)); };
    return out;
}
}
