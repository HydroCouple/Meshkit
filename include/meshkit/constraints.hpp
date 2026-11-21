#include "meshkit/detail/planmodel.hpp"
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <functional>
#include <optional>

namespace meshkit::plan
{
/**
 * @brief Per-feature limits for constraint simplification. XY values use the features'
 * common planar frame, Z values the authored elevation units; nothing is converted or
 * clamped. Check with validate().
 */
struct ConstraintSimplificationPolicy {
    /**
     * @brief Whether vertices of a feature may be removed.
     *
     * - Preserve: keep every vertex (default).
     * - Simplify: remove vertices whose replacing shortcut passes every limit.
     */
    enum class Mode {
        Preserve, ///< Keep every vertex (default).
        Simplify  ///< Remove vertices whose replacing shortcut passes every limit.
    };
    Mode mode = Mode::Preserve; ///< Preserve (default) or Simplify.
    /// Largest XY distance from a removed vertex to the shortcut segment replacing it; 0
    /// removes only vertices exactly on the shortcut. Finite and >= 0.
    double maxDeviation = 0;  // XY, in the common planar frame
    /// Largest |z - linear interpolation along the shortcut| of a removed vertex, in Z units;
    /// used only for features with z. 0 removes only exactly linear heights. Finite, >= 0.
    double maxZDeviation = 0; // Z, in the supplied elevation units; 0 preserves exact heights
    /// Vertices turning by more than this many degrees (0 = straight on, 180 = reversal) are
    /// kept as corners, as are vertices next to a zero-length edge. Default 5; [0, 180].
    double maxTurnDegrees = 5;
    /// Rings only: largest |area change| / |original area|; a failing ring is restored.
    /// Default 0.001; valid [0, 1], or exactly -1 to disable.
    double maxAreaChangeFraction = .001; // Relative to original ring area; -1 disables
    /// Rings only: largest absolute area change in squared XY units; default -1 (disabled),
    /// otherwise >= 0. A ring whose area changes sign or becomes zero is always restored.
    double maxAreaChange = -1;           // Absolute area in squared XY units; -1 disables
    /// XY distance a shortcut must keep from original segments outside its span and from
    /// protected points, unless one of its endpoints lies on them; 0 (default) disables.
    /// Finite and >= 0.
    double minimumClearance = 0;
    /**
     * @brief Check the mode and the ranges documented on each field.
     * @return Empty text when valid, otherwise an English message for the first failure.
     */
    Text validate() const;
};
// Unset fields inherit from the parent policy. The caller supplies consistent
// planar/elevation units before resolving; no implicit conversion or clamping.
/**
 * @brief Optional per-source changes to a parent ConstraintSimplificationPolicy, applied by
 * resolve(). Unset fields inherit; maxAreaChange cannot be overridden and always inherits.
 * Values use the parent's units; nothing is converted or clamped.
 */
struct ConstraintPolicyOverride {
    /// Mode override: -1 inherits (default), 0 Preserve, 1 Simplify; other values fail
    /// validate().
    int mode = -1; // -1 inherits, 0 preserves, 1 simplifies
    /**
     * @var maxDeviation
     * @brief Replaces ConstraintSimplificationPolicy::maxDeviation when set.
     * @var maxZDeviation
     * @brief Replaces ConstraintSimplificationPolicy::maxZDeviation when set.
     * @var maxTurnDegrees
     * @brief Replaces ConstraintSimplificationPolicy::maxTurnDegrees when set.
     * @var maxAreaChangeFraction
     * @brief Replaces ConstraintSimplificationPolicy::maxAreaChangeFraction when set; must
     * be >= 0, so the -1 "disabled" value cannot be set here.
     * @var minimumClearance
     * @brief Replaces ConstraintSimplificationPolicy::minimumClearance when set.
     */
    std::optional<double> maxDeviation, maxZDeviation, maxTurnDegrees, maxAreaChangeFraction,
        minimumClearance;
    /**
     * @brief Check mode, a set maxAreaChangeFraction, and this override resolved against a
     * default-constructed policy; it is not checked against any particular parent.
     * @return Empty text when valid, otherwise an English message for the first failure.
     */
    Text validate() const;
    /**
     * @brief Merge this override into a parent policy without validation.
     * @param[in] parent Policy supplying every unset field; taken by value.
     * @return parent with each set field replaced; mode 0 gives Preserve, any positive mode
     * Simplify, a negative mode inherits.
     */
    ConstraintSimplificationPolicy resolve(ConstraintSimplificationPolicy parent) const;
};
/**
 * @brief One constraint polyline or ring for simplification and the constraint audits. It is
 * a ring when closed is true or its last XY vertex equals its first; such a closing
 * duplicate is kept in the output and must carry the same elevation as the first vertex.
 */
struct SimplificationFeature {
    /// Stable identifier: copied to SimplificationChange::id and prefixed to error messages.
    /// Uniqueness is not checked.
    Text id;
    Vector<Point2> xy; ///< Finite vertex coordinates in the common planar frame (XY units).
    /// Optional authored elevations in Z units; compared with maxZDeviation, never changed.
    Vector<double> z;   // Empty or exactly one finite value per XY vertex
    /// Per-vertex flags; a true vertex is never removed. Missing trailing flags are false.
    Vector<bool> fixed; // Semantic vertices; absent flags mean false
    bool closed = false; ///< Treat as a ring even without a repeated closing vertex.
    ConstraintSimplificationPolicy policy; ///< Simplification limits for this feature.
    /**
     * @var role
     * @brief Meaning for auditMeshConstraints() (default Line); copied unchanged by
     * simplification.
     */
    /**
     * @brief Meaning of a feature for auditMeshConstraints().
     *
     * - Line: constraint path; only coverage by delivered edges is checked.
     * - Domain: outer boundary ring; each covered edge needs one incident cell on its inside
     *   (two, one per side, where adjacent domains share the edge).
     * - Hole: hole ring; each covered edge needs one incident cell outside the hole.
     * - Protected: caller label (Python role "protected"); treated like Line here.
     */
    enum class Role {
        Line,     ///< Constraint path; only coverage by delivered edges is checked.
        Domain,   ///< Outer boundary ring; covered edges need an incident cell inside.
        Hole,     ///< Hole ring; covered edges need an incident cell outside the hole.
        Protected ///< Caller label (Python role "protected"); treated like Line.
    } role = Role::Line;
    /// Boundary marker, copied unchanged by simplification. When nonzero,
    /// auditMeshConstraints() accepts only delivered edges with the same marker.
    int marker = 0;
    /// True (default): auditMeshConstraints() requires delivered cell edges to cover this
    /// feature; false: the mesh audit skips it.
    bool requiredInMesh = true;
    /// Optional human-readable label; copied unchanged, not read by simplification or the
    /// audits.
    Text label; // Optional human-readable source label; id remains stable.
};
/**
 * @brief Per-feature outcome of simplifyConstraints(); changes are in input order.
 */
struct SimplificationChange {
    Text id; ///< Copy of the feature's id.
    /**
     * @var removed
     * @brief Vertices removed (input XY count minus output XY count); 0 when unchanged.
     * @var rejectedShortcuts
     * @brief Candidate shortcuts rejected; each is split at its worst (or middle) vertex
     * and both halves are retried.
     */
    std::ptrdiff_t removed = 0, rejectedShortcuts = 0;
    /**
     * @var maxDeviation
     * @brief Largest XY distance of a removed vertex from its accepted shortcut; 0 when
     * nothing was removed or the edit was reverted.
     * @var maxZDeviation
     * @brief Largest Z deviation of a removed vertex from linear interpolation along its
     * shortcut, in Z units; 0 when nothing was removed or the edit was reverted.
     * @var areaChange
     * @brief Rings: absolute area change in squared XY units; 0 for open lines and
     * reverted edits.
     */
    double maxDeviation = 0, maxZDeviation = 0, areaChange = 0;
    /**
     * @var budgetRetained
     * @brief The original geometry was kept because the work budget or workspace limit ran
     * out, or the run was canceled.
     * @var rolledBack
     * @brief An accepted edit was reverted: ring area/orientation limit failed, output
     * shortcuts of different features intersected, or a feature in the same shared-segment
     * component was reverted.
     */
    bool budgetRetained = false, rolledBack = false;
    // Empty means identity. Otherwise maps each output vertex to its original index.
    Vector<int> sourceVertices; ///< Input index of each output vertex; empty means identity.
};
/**
 * @brief Pins, cancellation and resource limits for simplifyConstraints().
 */
struct ConstraintSimplificationOptions {
    /// Finite XY points a shortcut must not pass through (except at an endpoint), sweep
    /// over or (with minimumClearance) approach. A non-finite point fails the call.
    Vector<Point2> protectedPoints;
    /// Optional; polled before work starts and every 4096 work units. Returning true ends
    /// the call with cancelled set, ok false and the original features.
    std::function<bool()> cancelled;
    // Zero selects 256 operations per input vertex plus 100,000. Exhaustion
    // retains original features, never relaxes geometry checks.
    /// Work units allowed; 0 selects 256 per input vertex plus 100,000. Exhaustion keeps
    /// every original feature.
    std::uint64_t workBudget = 0;
    /// True (default): with several features, exact shared segments are simplified once
    /// as chains (detail::simplifySharedConstraints()). False: each feature is simplified
    /// separately, still checked against all original geometry.
    bool canonicalizeSharedEdges = true;
    // A bounded shared-edge workspace; exceeding it retains the original graph.
    /// Estimated shared-edge workspace limit in bytes (default 1 GiB); exceeding it keeps
    /// every original feature. Not a limit on total process memory.
    std::uint64_t maximumWorkspaceBytes = std::uint64_t(1) << 30;
};
/**
 * @brief Output of simplifyConstraints().
 */
struct ConstraintSimplificationResult {
    /**
     * @var ok
     * @brief The call finished without input error or cancellation, including runs that
     * kept the originals because a budget ran out.
     * @var cancelled
     * @brief ConstraintSimplificationOptions::cancelled stopped the run; ok is false.
     */
    bool ok = false, cancelled = false;
    /// Empty unless the input was rejected; then an English message, usually prefixed with
    /// the feature id.
    Text error;
    /// One feature per input, in input order; the original wherever no edit was accepted.
    /// Retained vertices keep their exact coordinates and elevations.
    Vector<SimplificationFeature> features;
    Vector<SimplificationChange> changes; ///< One entry per input feature, in input order.
    /**
     * @var sharedSegments
     * @brief Exact segments owned by more than one feature (shared-edge path only; can be
     * partial when the run kept the originals).
     * @var sharedChains
     * @brief Canonical chains owned by more than one feature (shared-edge path only).
     * @var workUsed
     * @brief Work units consumed.
     */
    std::uint64_t sharedSegments = 0, sharedChains = 0, workUsed = 0;
};
// All features share one immutable segment index. Input must describe the
// intended original graph in one planar frame. Coordinates never move. Every
// accepted shortcut is checked against its entire original span (XY and Z),
// original obstacles/pins and swept pockets; output shortcuts are checked
// against each other. Exact shared-edge chains are simplified once under the
// strictest owner policies and stitched back with source mappings. Differently
// sampled overlaps remain conservative; coordinates are never proximity-welded.
/**
 * @brief Remove redundant constraint vertices within each feature's policy without moving
 * any coordinate.
 *
 * Validates every policy, coordinate, elevation count and protected point first. Open-line
 * endpoints, fixed vertices, corners above maxTurnDegrees, ring vertex 0 (plus two more when
 * a ring would keep fewer than three) are always kept. A shortcut over a span must keep every
 * removed vertex within maxDeviation/maxZDeviation with strictly increasing projection inside
 * the shortcut, and must not cross, overlap, sweep over, lose a crossing with or (with
 * minimumClearance) approach other original segments or protected points. A rejected span is
 * split and retried. Rings failing the area limits, and features whose output shortcuts
 * intersect, are restored.
 *
 * @param[in] features Input features in one planar frame; read only, not retained.
 * @param[in] options Pins, cancellation and limits; the callback runs synchronously.
 * @return One output feature and one change per input. Invalid input: error set, ok false.
 * Work or workspace exhaustion: all originals, ok true, budgetRetained set. Cancellation:
 * all originals, ok false. No IO; no shared mutable state besides one-time exact-predicate
 * initialization.
 */
ConstraintSimplificationResult
simplifyConstraints(const Vector<SimplificationFeature> &features,
                    const ConstraintSimplificationOptions &options = {});

namespace detail
{
// Internal shared-chain adapter; the public entry point validates input first.
/**
 * @brief Shared-edge path of simplifyConstraints(). Groups segments with identical endpoints
 * (either direction, +/-0 folded) into canonical chains, simplifies each chain once under
 * the strictest policy of its owners and maps the result back onto every owner.
 *
 * A feature retracing one of its own segments, inconsistent shared ordering, or conflicting
 * shared heights set error. A ring-area failure restores every feature of the connected
 * shared component. Without shared segments it runs the plain path on the remaining budget.
 * @param[in] features Input already validated by simplifyConstraints(); read only.
 * @param[in] options As for simplifyConstraints(); canonicalizeSharedEdges is not consulted.
 * @return As simplifyConstraints(), with sharedSegments and sharedChains filled.
 */
ConstraintSimplificationResult
simplifySharedConstraints(const Vector<SimplificationFeature> &features,
                          const ConstraintSimplificationOptions &options);
}
}
