// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QPointF>
#include <QString>
#include <QVector>
#include <functional>
#include <optional>

namespace mesh
{
/**
 * @brief Per-feature limits for constraint simplification.
 *
 * Simplification only removes vertices; retained coordinates never move. In Mode::Simplify a
 * vertex can be removed when it is not the first vertex, not the last vertex of an open
 * feature, not flagged in SimplificationFeature::fixed, does not turn more than
 * maxTurnDegrees, and the shortcut replacing it passes every limit below plus the topology
 * checks of simplifyConstraints(). XY limits use the features' common planar frame and Z limits
 * the units of SimplificationFeature::z; no unit conversion is applied.
 */
struct ConstraintSimplificationPolicy {
    /** @enum Mode
     *  @brief Whether a feature may lose vertices.
     *  @var Mode::Preserve
     *  @brief Keep every vertex; the feature is returned unchanged.
     *  @var Mode::Simplify
     *  @brief Remove vertices wherever this policy and the topology checks allow it. */
    enum class Mode { Preserve, Simplify };
    Mode mode = Mode::Preserve; ///< @brief Simplification mode; Preserve by default.
    /// @brief Largest XY distance a removed vertex may lie from the shortcut replacing it; 0
    /// allows removing only exactly collinear vertices. Must be finite and nonnegative.
    double maxDeviation = 0;  // XY, in the common planar frame
    /// @brief Largest difference between a removed vertex's z and the height interpolated
    /// linearly along the shortcut at its projected position. Ignored for features without z.
    /// Must be finite and nonnegative.
    double maxZDeviation = 0; // Z, in the supplied elevation units; 0 preserves exact heights
    /// @brief Corner limit in degrees (0 = straight on): a vertex whose direction change
    /// exceeds it, or that has a zero-length neighbouring edge, is kept. Valid range [0, 180].
    double maxTurnDegrees = 5;
    /// @brief Rings only: largest |area change| as a fraction of |original area|, in [0, 1] or
    /// exactly -1. A ring exceeding it is rolled back to its original geometry.
    double maxAreaChangeFraction = .001; // Relative to original ring area; -1 disables
    /// @brief Rings only: largest |area change|; must be finite and nonnegative or exactly -1.
    /// A ring exceeding it is rolled back to its original geometry.
    double maxAreaChange = -1;           // Absolute area in squared XY units; -1 disables
    /// @brief XY distance a shortcut must keep from other original segments and protected
    /// points that do not touch its endpoints; 0 disables the check. Finite and nonnegative.
    double minimumClearance = 0;
    /**
     * @brief Check every field against its valid range.
     * @return Empty string when the policy is valid, otherwise an English message for the first
     * failed group of checks: unknown mode; nonfinite or negative maxDeviation, maxZDeviation
     * or minimumClearance; maxTurnDegrees outside [0, 180]; an area limit that is nonfinite,
     * above 1 (fraction only) or negative other than exactly -1.
     */
    QString validate() const;
};
// Unset fields inherit from the parent policy. The caller supplies consistent
// planar/elevation units before resolving; no implicit conversion or clamping.
/**
 * @brief Partial ConstraintSimplificationPolicy layered over a parent policy, for per-source
 * overrides. ConstraintSimplificationPolicy::maxAreaChange has no override and is always
 * inherited.
 */
struct ConstraintPolicyOverride {
    /// Mode override; validate() rejects values outside [-1, 1].
    int mode = -1; // -1 inherits, 0 preserves, 1 simplifies
    /** @var maxDeviation
     *  @brief Replaces ConstraintSimplificationPolicy::maxDeviation when set (XY units).
     *  @var maxZDeviation
     *  @brief Replaces ConstraintSimplificationPolicy::maxZDeviation when set (Z units).
     *  @var maxTurnDegrees
     *  @brief Replaces ConstraintSimplificationPolicy::maxTurnDegrees when set (degrees).
     *  @var maxAreaChangeFraction
     *  @brief Replaces ConstraintSimplificationPolicy::maxAreaChangeFraction when set; must be
     *  nonnegative here, so an override cannot disable the limit with -1.
     *  @var minimumClearance
     *  @brief Replaces ConstraintSimplificationPolicy::minimumClearance when set (XY units). */
    std::optional<double> maxDeviation, maxZDeviation, maxTurnDegrees,
        maxAreaChangeFraction, minimumClearance;
    /**
     * @brief Check the override on its own.
     * @return Empty string when valid, otherwise an English message. Rejects a mode outside
     * [-1, 1] and a negative maxAreaChangeFraction, then applies the set fields to a
     * default-constructed policy and returns ConstraintSimplificationPolicy::validate() of it.
     */
    QString validate() const;
    /**
     * @brief Apply the set fields to a parent policy.
     * @param[in] parent Policy taken by value; supplies every field that is not overridden.
     * @return @p parent with mode replaced when mode >= 0 (0 Preserve, positive Simplify) and
     * every engaged optional copied over. No validation, unit conversion or clamping.
     */
    ConstraintSimplificationPolicy resolve(ConstraintSimplificationPolicy parent) const;
};
/**
 * @brief One source constraint, an open polyline or a ring, given to simplifyConstraints().
 *
 * A feature is a ring when closed is true or when its first and last XY vertices are exactly
 * equal; a repeated closing vertex is optional and is reproduced in the output. role, marker,
 * requiredInMesh and label are not used by the simplifier; they are carried through unchanged
 * for the constraint audits and the caller.
 */
struct SimplificationFeature {
    QString id; ///< Caller identifier, copied to SimplificationChange::id and error prefixes.
    QVector<QPointF> xy; ///< @brief Vertices in the common planar frame; all must be finite.
    /// Elevations parallel to xy; a repeated closing vertex must repeat its elevation.
    QVector<double> z;   // Empty or exactly one finite value per XY vertex
    /// Per-vertex flags parallel to xy; a flagged vertex is never removed.
    QVector<bool> fixed; // Semantic vertices; absent flags mean false
    bool closed = false; ///< Treat the feature as a ring even without a repeated closing vertex.
    ConstraintSimplificationPolicy policy; ///< Limits for this feature; validated before use.
    /** @enum Role
     *  @brief Meaning of the feature for the delivered-mesh audit; ignored by the simplifier.
     *  @var Role::Line
     *  @brief Ordinary constraint path.
     *  @var Role::Domain
     *  @brief Domain outer ring; the mesh audit requires its cells on the inner side.
     *  @var Role::Hole
     *  @brief Hole ring; the mesh audit requires its cells on the outer side.
     *  @var Role::Protected
     *  @brief Protected outline, such as a region or fixed-patch boundary; the mesh audit
     *  treats it like Line, without a side check.
     *  @var role
     *  @brief Feature role; Role::Line by default. */
    enum class Role { Line, Domain, Hole, Protected } role = Role::Line;
    int marker = 0; ///< Boundary marker; when nonzero, the mesh audit matches only this marker.
    bool requiredInMesh = true; ///< @brief False makes the delivered-mesh audit skip it.
    /// Display label; makeConstraintPreview() falls back to id when it is empty.
    QString label; // Optional human-readable source label; id remains stable.
};
/**
 * @brief Outcome for one input feature; ConstraintSimplificationResult::changes is parallel to
 * the input. The deviation and area statistics describe accepted removals and are zero when
 * the feature is unchanged, rolled back or retained.
 */
struct SimplificationChange {
    QString id; ///< Copy of SimplificationFeature::id.
    /** @var removed
     *  @brief Vertices removed (input minus output xy count); 0 when the feature is unchanged.
     *  @var rejectedShortcuts
     *  @brief Candidate shortcuts refused by a deviation or topology check; each refusal keeps
     *  one more vertex and retries both halves. A shared chain's count is added to every owner.
     */
    qsizetype removed = 0, rejectedShortcuts = 0;
    /** @var maxDeviation
     *  @brief Largest XY distance of a removed vertex from its accepted shortcut.
     *  @var maxZDeviation
     *  @brief Largest elevation error of a removed vertex against its accepted shortcut.
     *  @var areaChange
     *  @brief Rings only: |output area - input area| in squared XY units. */
    double maxDeviation = 0, maxZDeviation = 0, areaChange = 0;
    /** @var budgetRetained
     *  @brief The work budget, the workspace limit or cancellation stopped the pass, so the
     *  original geometry was kept.
     *  @var rolledBack
     *  @brief A simplification was reverted: the ring area limits failed or the ring area
     *  changed sign or vanished, or a new shortcut intersects another output segment (both
     *  owners revert). A failed ring reverts every simplified owner in its shared component. */
    bool budgetRetained = false, rolledBack = false;
    // Empty means identity. Otherwise maps each output vertex to its original index.
    /// Output-to-input vertex indices, increasing; a repeated closing vertex maps to the last.
    QVector<int> sourceVertices;
};
/** @brief Collection-wide inputs and resource limits for simplifyConstraints(). */
struct ConstraintSimplificationOptions {
    /// @brief Finite points in the planar frame that no shortcut may pass through or enclose
    /// between itself and the original path, kept at the policy's minimumClearance.
    QVector<QPointF> protectedPoints;
    /// @brief Optional cancellation poll, called synchronously at the start and about every
    /// 4096 work units; returning true ends the pass with the original features.
    std::function<bool()> cancelled;
    // Zero selects 256 operations per input vertex plus 100,000. Exhaustion
    // retains original features, never relaxes geometry checks.
    /// Work-unit limit for the pass; 0 selects 256 per input vertex plus 100,000.
    quint64 workBudget = 0;
    /// @brief Simplify segments shared exactly by several features once, as canonical chains
    /// under the strictest owner policies (needs more than one feature). False simplifies
    /// every feature on its own against the common index.
    bool canonicalizeSharedEdges = true;
    // A bounded shared-edge workspace; exceeding it retains the original graph.
    /// Estimated byte limit for the shared-edge canonicalization records (1 GiB by default).
    quint64 maximumWorkspaceBytes = quint64(1) << 30;
};
/** @brief Output of simplifyConstraints(); features and changes are parallel to the input. */
struct ConstraintSimplificationResult {
    /** @var ok
     *  @brief True when the pass finished, also when exhausted limits kept the originals (see
     *  SimplificationChange::budgetRetained). False on invalid input or cancellation.
     *  @var cancelled
     *  @brief Cancellation was observed; ok is false and the features are the originals. */
    bool ok = false, cancelled = false;
    QString error; ///< English reason when ok is false without cancellation; otherwise empty.
    QVector<SimplificationFeature> features; ///< Output features; the originals on failure.
    QVector<SimplificationChange> changes; ///< One outcome per input feature.
    /** @var sharedSegments
     *  @brief Distinct segments owned by more than one feature; counted only by the
     *  shared-edge path, 0 otherwise.
     *  @var sharedChains
     *  @brief Canonical chains built from shared segments; counted only by the shared-edge
     *  path, 0 otherwise.
     *  @var workUsed
     *  @brief Work units consumed, including shared-edge indexing. */
    quint64 sharedSegments = 0, sharedChains = 0, workUsed = 0;
};
// All features share one immutable segment index. Input must describe the
// intended original graph in one planar frame. Coordinates never move. Every
// accepted shortcut is checked against its entire original span (XY and Z),
// original obstacles/pins and swept pockets; output shortcuts are checked
// against each other. Exact shared-edge chains are simplified once under the
// strictest owner policies and stitched back with source mappings. Differently
// sampled overlaps remain conservative; coordinates are never proximity-welded.
/**
 * @brief Remove redundant vertices from a collection of constraint features without moving
 * any coordinate.
 *
 * Every feature is validated first: its policy, its z count, equal elevations on a repeated
 * closing vertex, finite coordinates and a total vertex count below INT_MAX - 3; then the
 * protected points. When no feature is in Mode::Simplify with more than two vertices the input
 * is returned unchanged with ok true. Otherwise the kept anchors are split recursively until
 * every shortcut passes the XY/Z deviation, monotone-projection, crossing, junction, clearance
 * and pocket checks against the original graph and protected points. Rings are then checked
 * against their area limits and a final index checks the output shortcuts against each other.
 * @param[in] features Source features in one planar frame; read only, copied into the result.
 * @param[in] options Protected points, cancellation and resource limits; the callback runs
 * synchronously on the calling thread.
 * @return Owned result. ok is false with error set for invalid input or a shared graph that
 * retraces a segment or has inconsistent shared chains or heights; cancellation sets
 * cancelled. Exhausted work or workspace limits keep ok true and return the originals with
 * every change marked budgetRetained.
 */
ConstraintSimplificationResult
simplifyConstraints(const QVector<SimplificationFeature> &features,
                    const ConstraintSimplificationOptions &options = {});

namespace detail
{
// Internal shared-chain adapter; the public entry point validates input first.
/**
 * @brief Internal: group exactly shared segments into chains, simplify each chain once under
 * the strictest owner policies and map the result back to every owner.
 *
 * Performs no policy or coordinate validation; use simplifyConstraints(). Without any shared
 * segment it delegates to simplifyConstraints() with canonicalization disabled.
 * @param[in] features Source features, read only; must already have passed validation.
 * @param[in] options As for simplifyConstraints(); maximumWorkspaceBytes bounds the estimated
 * shared-edge records.
 * @return Owned result with the conventions of simplifyConstraints().
 */
ConstraintSimplificationResult
simplifySharedConstraints(const QVector<SimplificationFeature> &features,
                          const ConstraintSimplificationOptions &options);
}
}
