
#include "meshkit/detail/planmodel.hpp"
/**
 * @file      patch.hpp
 * @brief     Structured quad patches (workplans/TRI_QUAD_MESHING_PLAN_2026-09-06.md §3.2, phase
 *            G3).
 * @author    Caleb Buahin <caleb.buahin@gmail.com>
 * @date      2026
 * @license   Apache-2.0
 *
 * Structured quad patches (workplans/TRI_QUAD_MESHING_PLAN_2026-09-06.md
 * §3.2, phase G3). Two authoring forms:
 *
 *  - \ref meshkit::plan::StructuredPatch — a four-sided polygon meshed by transfinite
 *    (Coons) interpolation between opposite sides, n × m quads;
 *  - \ref meshkit::plan::SweptPatch — a centerline polyline swept with a width, quads
 *    laid across the channel at each station (channel / street case).
 *
 * Both produce a \ref meshkit::plan::PatchMesh in LOCAL vertex indices. The native job
 * plans and places patches through preparePatches() before triangulation; the Qt
 * MeshGenerator::addPatch stitches the mesh:: mirror types into the triangle domain (the
 * boundary segments become PSLG constraints, the interior a hole, and the quads are
 * appended after the triangles).
 * @copyright Copyright (c) 2026 Caleb Buahin. All rights reserved.
 */

#ifndef MESHKIT_NATIVE_MESHPATCH_HPP
#define MESHKIT_NATIVE_MESHPATCH_HPP

namespace meshkit::plan
{

/*! \brief Four-sided patch: corners in cyclic order (either orientation);
 *  n subdivisions along corner0→corner1 (and corner3→corner2), m along
 *  corner0→corner3 (and corner1→corner2). */
struct StructuredPatch {
    Vector<Point2> corners; ///< Exactly 4, cyclic.
    /// \var n
    /// \brief Subdivisions along corner0→corner1; must be >= 1.
    /// \var m
    /// \brief Subdivisions along corner0→corner3; must be >= 1. The vertex, cell and boundary
    /// counts (n+1)×(m+1), n×m and 2(n+m) must fit in int.
    int n = 0, m = 0;
    Text tag; ///< Label copied to PatchMesh::tag and to every generated quad.
};

/*! \brief Swept patch: the centerline is offset ±width/2 (mitred at interior
 *  vertices) and cut into \p across quads per station. \p along > 0
 *  resamples every centerline segment to that target spacing (original
 *  vertices are kept); 0 = one station per centerline vertex. */
struct SweptPatch {
    Vector<Point2> centreline; ///< >= 2 points.
    double width = 0.0; ///< Full strip width, XY units; must be finite and > 0.
    int across = 0;     ///< Quads across the strip at every station; must be >= 1.
    /// Target station spacing along the centerline, XY units; must be finite and >= 0. Each
    /// segment is cut into ceil(length/along) equal parts (a ratio within a tiny tolerance of
    /// an integer is snapped to it).
    double along = 0.0;
    Text tag; ///< Label copied to PatchMesh::tag and to every generated quad.
};

/*! \brief Two open bank polylines with independently supplied directions.
 *  Endpoints are paired by minimum total connection length. Stations use the
 *  union of normalized arc-length vertices, retaining every bank bend exactly.
 *  Positive along subdivides that union to the target spacing; zero retains
 *  only the union. This is normalized arc correspondence, not surveyed stations. */
struct BankPairPatch {
    /// \var bankA
    /// \brief First bank: open polyline in XY units with >= 2 finite points, no repeated
    /// consecutive vertex and distinct ends; either direction.
    /// \var bankB
    /// \brief Second bank, same rules. The banks and the two end connectors must form one
    /// simple outline; generation may reverse or swap the banks.
    Vector<Point2> bankA, bankB;
    int across = 0; ///< Quads across between the banks at every station; must be >= 1.
    /// Target station spacing, XY units, measured on the longer bank; must be finite and >= 0.
    /// Each interval of the station union is cut into ceil(advance/along) parts (a ratio within
    /// a tiny tolerance of an integer is snapped to it).
    double along = 0.0;
    Text tag; ///< Label copied to PatchMesh::tag and to every generated quad.
};

/*! \brief A polyline through a patch's vertices that must be reported as
 *  mesh edges with a marker — a conduit running along a strip's middle row. */
struct PatchLine {
    Vector<int> path; ///< Local vertex indices, consecutive ones joined by quad edges.
    int marker = 0;   ///< Marker given to every edge emitted for the line.
    /// Label for the emitted edges. When empty the GUI generator resolves one from #marker;
    /// the native job keeps only the marker.
    Text tag;
};

/*! \brief A generated patch in local indices. Quads are emitted CCW. */
struct PatchMesh {
    /// Vertex positions, XY units, indexed locally by #quads, #boundarySegments and #lines.
    /// No elevations are stored.
    Vector<Point2> xy;
    Vector<MeshTriangle> quads;                   ///< v3 >= 0, local indices.
    Vector<std::pair<int, int>> boundarySegments; ///< Local index pairs around the patch.
    /// Patch label. The generators copy it to every quad, and validation messages quote it.
    Text tag;
    /*! Interior lines emitted as MeshResult::boundaryEdges with their marker
     *  and tag, and bounding region tags like any constraint. */
    Vector<PatchLine> lines;
};

/*! \brief Input checks. Empty string = valid. */
Text validate(const StructuredPatch &p);
/*! \brief Input checks for a swept patch: >= 2 finite centerline points with no repeated
 *  consecutive vertex, finite width > 0, across >= 1, finite along >= 0, and station, cell
 *  and boundary counts that fit in int. Folded or overlapping offsets are not detected here
 *  (makeSweptPatch() checks them). Empty string = valid, otherwise the first problem found;
 *  the parameter and return value are documented with the definition. */
Text validate(const SweptPatch &p);
/*! \brief Requires finite convex CCW quads, consistently shared cell edges,
 *  and one simple boundary matching exactly the outer cell edges. Boundary
 *  crossings, remote touches, holes and disconnected pieces are rejected;
 *  an indexed boundary check avoids comparing every cell pair.
 *  Empty string = valid. */
Text validate(const PatchMesh &pm);

/*! \brief Validate the complete patch and return its CCW boundary, without
 *  a repeated closing vertex. Segment input orientation/order is immaterial.
 *  On failure returns an empty ring and sets \p err. */
Polygon orderedPatchBoundary(const PatchMesh &pm, Text *err = nullptr);

/*! \brief Transfinite interpolation on the four straight sides. The result
 *  has (n+1)×(m+1) vertices and n×m quads. On invalid input returns an
 *  empty PatchMesh and sets *err. */
PatchMesh makeTransfinitePatch(const StructuredPatch &p, Text *err = nullptr);

/*! \brief Sweep the centerline: (stations)×(across+1) vertices,
 *  (stations-1)×across quads. Station and index counts are checked before
 *  allocation. Invalid or globally overlapping/touching offsets return an
 *  empty PatchMesh and a diagnostic identifying the affected stations. */
PatchMesh makeSweptPatch(const SweptPatch &p, Text *err = nullptr);

/*! \brief Build a conforming strip between two simple, disjoint banks.
 *  Rejects ambiguous endpoint pairing, touching/crossing banks, folded cells,
 *  unrepresentable station geometry and unsafe subdivision counts. */
PatchMesh makeBankPairPatch(const BankPairPatch &p, Text *err = nullptr);

// ── Quad-region redesign (QUAD_MESHING_REDESIGN_PLAN_2026-09-06.md §4.2) ───

/*! \brief Transfinite (Coons) interpolation on four POLYLINE sides forming a
 *  loop: sides[0] runs c0→c1, sides[1] c1→c2, sides[2] c2→c3, sides[3] c3→c0
 *  (each side's last point equals the next side's first). Sides are
 *  arc-length parametrized; n subdivisions along sides 0/2, m along 1/3.
 *  Boundary vertices are placed ON the polylines (so the ring the caller
 *  emits as PSLG segments is exactly this patch's boundary loop). Returns an
 *  empty PatchMesh + *err on invalid input (side with < 2 points, endpoints
 *  not matching, n or m < 1, folded quad). */
PatchMesh makeTransfinitePatch(const Vector<Vector<Point2>> &sides, int n, int m, const Text &tag,
                               Text *err = nullptr);
/*! \brief Coons patch on four already-stationed sides that keeps every station exactly.
 *  \param[in] sides Four cyclic sides, each with >= 2 finite points and nonzero length;
 *  sides[k] must end exactly where sides[(k+1)%4] starts. Sides 0 and 2 (and 1 and 3) need
 *  equal point counts: station i of side 0 faces station n-i of side 2, and station j of
 *  side 1 faces station m-j of side 3.
 *  \param[in] tag Label copied to the patch and to every quad.
 *  \param[out] err Optional; set on failure and left unchanged on success.
 *  \return Patch of n = sides[0].size()-1 by m = sides[1].size()-1 CCW quads whose boundary
 *  vertices are exactly the given stations and whose interior comes from Coons interpolation;
 *  an empty PatchMesh on invalid input or when the full patch fails validate(). */
// Four already-stationed cyclic sides. Opposite side counts must agree.
// Retains every boundary coordinate exactly; shared owners can reuse/reverse
// one canonical sequence. No independent boundary resampling occurs.
PatchMesh makeTransfinitePatchAtStations(const Vector<Vector<Point2>> &sides, const Text &tag,
                                         Text *err = nullptr);

/*! \brief Split a ring (either winding) at the four \p corners (ring vertex indices, ring
 *  order) into polyline sides, choose n = max(1, round(mean(len0, len2)/h)),
 *  m likewise from sides 1/3, and call the polyline overload. */
PatchMesh makeMappedPatch(const Polygon &ring, const Vector<int> &corners, double h,
                          const Text &tag, Text *err = nullptr);

/*! \brief Mapped patch with independent physical-axis spacing. The axis is
 *  undirected, measured counterclockwise from +x in degrees. The opposite
 *  logical side pair most aligned with it receives hAlong; the other receives
 *  hAcross. Equal association scores are rejected. Ring rotation/reversal must
 *  not exchange the physical spacings. Widths use the ring's coordinate units. */
PatchMesh makeMappedPatch(const Polygon &ring, const Vector<int> &corners, double hAlong,
                          double hAcross, double alongAngleDeg, const Text &tag,
                          Text *err = nullptr);

} // namespace meshkit::plan

#endif // MESHKIT_NATIVE_MESHPATCH_HPP
