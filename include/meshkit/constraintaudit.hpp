#include "meshkit/detail/planmodel.hpp"
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "meshkit/constraints.hpp"
namespace meshkit::plan
{
struct MeshResult;
/**
 * @brief Pins, cancellation and resource limits for the constraint audits.
 */
struct ConstraintAuditOptions {
    /// XY pins that simplified shortcuts must not cross, sweep over or (with the feature's
    /// minimumClearance) approach; a non-finite pin is a violation. Used only by
    /// auditConstraintSimplification().
    Vector<Point2> protectedPoints;
    /// Optional; polled before work starts and every 4096 work units. Returning true stops
    /// the audit with cancelled set; the report is then incomplete and never ok.
    std::function<bool()> cancelled;
    /// Work units allowed; 0 selects 256 per input item plus 100,000 (items: original and
    /// candidate vertices and pins, or constraint vertices, delivered edges and cells).
    /// Exhaustion sets resourceLimited.
    std::uint64_t workBudget = 0;
    /// Estimated workspace limit in bytes (default 1 GiB); exceeding it sets
    /// resourceLimited. Not a limit on total process memory.
    std::uint64_t maximumWorkspaceBytes = std::uint64_t(1) << 30;
    /// Violations kept in ConstraintAuditReport::examples, clamped to [0, 1000]; all are
    /// still counted. Default 20.
    int maximumExamples = 20;
};
/**
 * @brief One recorded audit violation.
 */
struct ConstraintAuditExample {
    /**
     * @var featureId
     * @brief Id of the offending feature, or "pin", "graph" or "mesh" for violations not
     * tied to one feature.
     * @var message
     * @brief English description of the violation.
     */
    Text featureId, message;
    /// Index of the related vertex, segment, pin or delivered edge record (meaning depends
    /// on the message); -1 when none.
    int segment = -1;
};
/**
 * @brief Result of auditConstraintSimplification() or auditMeshConstraints().
 */
struct ConstraintAuditReport {
    /**
     * @var ok
     * @brief complete and no violations: the only state that certifies the input.
     * @var complete
     * @brief Not cancelled and not resource-limited. Structural violations can still end
     * the audit early with complete set.
     * @var cancelled
     * @brief ConstraintAuditOptions::cancelled stopped the audit.
     * @var resourceLimited
     * @brief The work budget or workspace estimate ran out.
     */
    bool ok = false, complete = false, cancelled = false, resourceLimited = false;
    /// Empty when no violation was found and the audit finished; otherwise "<id>: <message>"
    /// of the first violation, or the cancellation / resource-limit message.
    Text error;
    /**
     * @var featuresChecked
     * @brief Features examined (simplification audit), or required features examined
     * (mesh audit).
     * @var segmentsChecked
     * @brief Output segments (simplification audit) or required source segments (mesh
     * audit) examined.
     * @var cellsChecked
     * @brief Delivered cells visited; mesh audit only.
     * @var violations
     * @brief Total violations found, including those beyond maximumExamples.
     * @var workUsed
     * @brief Work units consumed.
     */
    std::uint64_t featuresChecked = 0, segmentsChecked = 0, cellsChecked = 0, violations = 0,
                  workUsed = 0;
    /**
     * @var maximumXYDeviation
     * @brief Largest XY distance found: of a removed vertex from its output shortcut
     * (simplification audit), or of a matched delivered edge from its source line (mesh
     * audit).
     * @var maximumZDeviation
     * @brief Largest Z difference found, in Z units: of a removed vertex from linear
     * interpolation, or of a delivered endpoint from its authored elevation.
     * @var maximumAreaChange
     * @brief Largest absolute ring area change in squared XY units; simplification audit
     * only.
     * @var coordinateRoundoff
     * @brief Largest XY matching tolerance used, 32 machine epsilons times the segment's
     * coordinate scale; mesh audit only.
     */
    double maximumXYDeviation = 0, maximumZDeviation = 0, maximumAreaChange = 0,
           coordinateRoundoff = 0;
    /// First violations, at most maximumExamples (clamped to [0, 1000]).
    Vector<ConstraintAuditExample> examples;
};
// Independent read-only checks: no calls into the simplifier's acceptance or
// triangulator's constraint-recovery code. Source mappings are validated before
// use. Work/memory exhaustion is incomplete, never a successful certificate.
/**
 * @brief Independently check a simplification candidate against its original features.
 *
 * Per feature: valid policy; unchanged id, closed, role, marker, requiredInMesh and
 * elevation presence; a strictly increasing source mapping; retained vertices and heights
 * exactly equal to their sources; endpoints and ring closure kept; no Preserve-mode, fixed
 * or above-maxTurnDegrees vertex removed; consistent removed count; ring orientation and
 * area limits; XY/Z deviation and monotone projection of each removed vertex. Then source
 * rings must not touch themselves and, when anything was removed, each new shortcut is
 * checked against other output edges, pins and the original graph for crossings, collapsed
 * distinct paths, lost junctions, swept pockets and minimumClearance.
 * @param[in] original Features given to simplifyConstraints(); read only, not retained.
 * @param[in] candidate Its result; must be ok with one feature and change per original.
 * @param[in] options Pins, cancellation and limits; the callback runs synchronously.
 * @return Report; ok only when complete without violations. Some structural violations end
 * the audit at once. Inputs are not modified; no IO.
 */
ConstraintAuditReport auditConstraintSimplification(const Vector<SimplificationFeature> &original,
                                                    const ConstraintSimplificationResult &candidate,
                                                    const ConstraintAuditOptions &options = {});
// Checks required domain/hole/line paths against delivered, incident cell edges
// after all mesh edits/reordering. It is a constraint-coverage/side audit, not a
// general proof that arbitrary cells do not overlap. Coordinate roundoff is
// explicitly measured; simplification tolerances never excuse missing edges.
/**
 * @brief Check that a delivered mesh still carries every required source constraint.
 *
 * Each non-degenerate segment of a feature with requiredInMesh must be covered end to end,
 * from its first to its last vertex, by a chain of delivered edges (mesh.boundaryEdges)
 * lying on it within coordinateRoundoff, each with one or two incident cells and, for a
 * nonzero feature marker, the same marker. Domain and Hole edges need their cells on the
 * correct side (see SimplificationFeature::Role); delivered endpoint z must match authored
 * z to roundoff. Not a general cell-overlap proof.
 * @param[in] constraints Source features; Domain/Hole rings also define the inside used for
 * side checks. Read only, not retained.
 * @param[in] mesh Delivered mesh; must have ok set. Read only, not retained.
 * @param[in] options Cancellation and limits; protectedPoints is not used.
 * @return Report; ok only when complete without violations. Invalid indices or geometry end
 * the audit at once. Inputs are not modified; no IO.
 */
ConstraintAuditReport auditMeshConstraints(const Vector<SimplificationFeature> &constraints,
                                           const MeshResult &mesh,
                                           const ConstraintAuditOptions &options = {});
}
