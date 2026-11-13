Requests and options
====================

A meshing job is described by one :class:`dict`, passed to :func:`meshkit.run`
or :func:`meshkit.start`. Every mapping in it must be a :class:`dict`; an unknown
key or a value of the wrong type raises :class:`ValueError` before the job
starts. Values that are well-typed but inconsistent (an option out of range,
mismatched array lengths, missing units) do not raise: the job finishes with
status ``infeasible`` and a code such as ``option``, ``size_range`` or
``units_required`` in its report.

.. _requests:

The request
-----------

.. list-table::
   :header-rows: 1
   :widths: 22 28 50

   * - Key
     - Type
     - Meaning
   * - ``points``
     - float64 buffer ``(N, 2)`` or rows ``[x, y]``
     - **Required.** Source points in one Cartesian XY frame, at most 20 million.
       Paths index them from zero. Exact XY duplicates share one vertex.
   * - ``paths``
     - list of :class:`dict`
     - Constraint paths over ``points``; see `Paths`_. An ``outer`` path is
       required.
   * - ``options``
     - :class:`dict`
     - Sizing, cell and quality options; see `Options`_.
   * - ``metadata``
     - :class:`dict`
     - Units and description; see `Metadata`_.
   * - ``point_elevations``
     - sequence of float, one per point
     - Authored heights. NaN takes the height from the terrain raster (or
       ``flat_elevation`` without one). Coincident points must agree.
   * - ``point_ids``
     - sequence of int64, one per point
     - External IDs of the vertices made from these points
       (:attr:`~meshkit.MeshResult.vertex_ids`); default: one-based point
       order. Generated vertices get IDs above the highest. A point that shares
       a vertex with an earlier one keeps its ID as an alias
       (:attr:`~meshkit.MeshResult.alias_ids`).
   * - ``point_markers``
     - sequence of int32, one per point
     - Markers of those vertices (:attr:`~meshkit.MeshResult.vertex_markers`),
       kept the same way.
   * - ``quad_regions``
     - list of :class:`dict`
     - Areas to fill with quadrilaterals; see `Quadrilateral regions`_.
   * - ``size_field``
     - :class:`dict`
     - Sizes that grade away from the constraints; see `Size field`_.
   * - ``region_seeds``
     - list of ``{"point": [x, y], "region": int}``
     - Assign a region number to the constrained area containing each point.
       A seed must lie strictly inside the domain; conflicting seeds fail.
   * - ``labels``
     - list of ``{"id": int, "name": str}``
     - Names for region numbers, carried into the result and ``.mkb`` files.

Paths
~~~~~

.. list-table::
   :header-rows: 1
   :widths: 22 18 60

   * - Key
     - Default
     - Meaning
   * - ``vertices``
     - —
     - **Required.** Indexes into ``points``.
   * - ``kind``
     - ``"line"``
     - ``"outer"`` bounds the domain (outer paths form a union), ``"hole"`` is
       subtracted from it, ``"region"`` and ``"line"`` force mesh edges along
       themselves.
   * - ``closed``
     - ``False``
     - Outer, hole and region paths close implicitly; a line closes only when
       this is true.
   * - ``marker``
     - ``0``
     - Integer carried with the path.
   * - ``id``
     - ``0``
     - Integer identifier carried with the path.

Crossing paths must be split at their intersections before meshing:
:func:`meshkit.prepare_sources` does not node crossings, and the job rejects
them. Exactly shared vertices and overlapping stretches need no welding.

Metadata
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Key
     - Default
     - Meaning
   * - ``horizontal_metres_per_unit``
     - ``1``
     - Metres per XY unit. Must be positive.
   * - ``vertical_metres_per_unit``
     - ``1``
     - Metres per Z unit. Must be positive.
   * - ``units_known``
     - ``True``
     - False refuses the job (code ``units_required``).
   * - ``crs``
     - ``""``
     - Coordinate reference system text, stored, never used to transform.
   * - ``description``
     - ``""``
     - Free text, stored with the mesh.

Sizes and distances are in XY units; heights and terrain tolerances in Z units.

.. _options:

Options
-------

.. list-table::
   :header-rows: 1
   :widths: 30 16 54

   * - Key
     - Default
     - Meaning
   * - ``cells``
     - ``"triangles"``
     - ``"triangles"``, or ``"mixed"`` to plan mapped quadrilateral patches with
       triangle infill.
   * - ``maximum_size``
     - ``0``
     - Largest nominal cell size; 0 sets no global limit.
   * - ``minimum_target``
     - ``0``
     - Advisory smallest target size; constraints may still need smaller cells.
       0 uses a quarter of ``maximum_size``. Must not exceed ``maximum_size``.
   * - ``minimum_edge``
     - ``0``
     - Shortest edge allowed in the result; 0 does not check.
   * - ``minimum_angle_degrees``
     - ``30``
     - Smallest triangle angle refinement aims for; at most 34.
   * - ``adjacent_ratio``
     - ``0``
     - Largest size ratio between neighbouring cells; 0 does not check,
       otherwise at least 1.
   * - ``flat_elevation``
     - ``0``
     - Height used where neither an authored height nor a raster applies.
   * - ``smoothing_passes``
     - ``0``
     - Vertex smoothing passes, 0–10.
   * - ``coarsen_passes``
     - ``0``
     - Passes that remove generated vertices where sizing allows (and, with a
       raster, where the terrain bound is certified), 0–10.
   * - ``coarsening_order``
     - ``"legacy_flatness"``
     - Order of coarsening candidates: ``"legacy_flatness"`` or
       ``"resolution_aware"``.
   * - ``coarsening_reference_size``
     - ``10``
     - Reference size for the ``resolution_aware`` order. Must be positive.
   * - ``sizing_repair_passes``
     - ``2``
     - Passes that refine cells breaking a size requirement, 0–3.
   * - ``repair_insertions``
     - ``200000``
     - Most vertices the repair passes may insert.
   * - ``terrain_tolerance``
     - ``0``
     - Largest allowed gap between the mesh surface and the raster, in Z
       units; 0 disables terrain checks. Needs a raster; see `Terrain`_.
   * - ``terrain_policy``
     - constant
     - How the tolerance varies with cell size; see `Terrain`_.
   * - ``strict_sizing``
     - ``True``
     - Refuse the job when maximum-size, edge or transition requirements
       remain unmet, instead of returning it with exceptions.
   * - ``require_terrain_certification``
     - ``False``
     - Refuse the job when any cell's terrain bound is unknown or unmet.
   * - ``retain_input_mapping``
     - ``True``
     - Fill :attr:`~meshkit.MeshResult.input_mapping`.
   * - ``automatic_blocks``
     - ``False``
     - With ``"mixed"``, also place quadrilateral blocks in open areas.
   * - ``maximum_patch_cells``
     - ``250000``
     - Cell limit per planned quadrilateral patch.
   * - ``maximum_automatic_blocks``
     - ``4096``
     - Limit on automatic blocks.
   * - ``limits``
     - see below
     - Resource limits.

Nominal size is the side of the equilateral triangle of equal area for a
triangle, and the square root of the area for a quadrilateral.

Limits
~~~~~~

``limits`` bounds counts, estimated workspace and orchestration work; it does
not bound process memory or time spent elsewhere. A job that reaches a limit
returns status ``resource_limit`` with no geometry.

.. list-table::
   :header-rows: 1
   :widths: 30 25 45

   * - Key
     - Default
     - Meaning
   * - ``max_vertices``
     - 20,000,000
     - Vertices in the result.
   * - ``max_cells``
     - 20,000,000
     - Cells in the result.
   * - ``max_edges``
     - 40,000,000
     - Edges in the result.
   * - ``max_workspace_bytes``
     - 4 GiB
     - Estimated working memory.
   * - ``max_work``
     - 0
     - Work units for the job's own counter (0 = 20 billion), also passed as
       the budget of patch planning and the final constraint audit (0 = sized
       from their inputs).
   * - ``max_metadata_bytes``
     - 8 MiB
     - Metadata size (file I/O).
   * - ``max_record_bytes``
     - 1 MiB
     - Size of one file record (file I/O).

.. _terrain:

Terrain
-------

Pass a raster to :func:`meshkit.run` or :func:`meshkit.start` to give the job
heights and a terrain error check:

.. code-block:: python

   raster = {"heights": dem,              # float32 or float64, shape (rows, columns)
             "origin": [x0, y0],          # coordinates of heights[0, 0]
             "spacing": [dx, dy]}         # positive
   result = meshkit.run(request, raster=raster, provider_identity="dem-2026-v1")

.. list-table::
   :header-rows: 1
   :widths: 28 18 54

   * - Key
     - Default
     - Meaning
   * - ``heights``
     - —
     - **Required.** Node samples, row index increasing in +Y, at least 2 × 2 and
       at most 100 million. NaN marks missing data.
   * - ``origin``
     - ``[0, 0]``
     - XY of the first sample.
   * - ``spacing``
     - ``[1, 1]``
     - Sample spacing in X and Y.
   * - ``maximum_cells_per_query``
     - 4,000,000
     - Raster cells one bound query may visit.

The raster is a bilinear surface. For each triangular facet MeshKit checks its
whole intersection with the raster cells, including extremes along clipped
edges, and reports the largest gap. Missing data is unknown, not interpolated
across. The check concerns this surface, not the accuracy of the source DEM.
``provider_identity`` is copied into the report as evidence of which dataset
was used; MeshKit does not compute or check it.

``terrain_policy`` makes the tolerance grow with cell size:

.. list-table::
   :header-rows: 1
   :widths: 28 18 54

   * - Key
     - Default
     - Meaning
   * - ``mode``
     - ``"constant"``
     - ``"constant"`` uses ``terrain_tolerance`` everywhere. ``"target_size"``
       scales it by size / ``reference_size``, clamped to the range below.
   * - ``reference_size``
     - ``10``
     - Size at which the tolerance equals ``terrain_tolerance``.
   * - ``minimum_tolerance``
     - ``0``
     - Lower clamp.
   * - ``maximum_tolerance``
     - ``0``
     - Upper clamp; 0 uses ``terrain_tolerance``.

Quadrilateral regions
---------------------

Each entry in ``quad_regions`` describes an area to fill with mapped
quadrilaterals when ``cells`` is ``"mixed"``.

.. list-table::
   :header-rows: 1
   :widths: 24 16 60

   * - Key
     - Default
     - Meaning
   * - ``ring``
     - —
     - **Required.** Outline as rows ``[x, y]`` or a float64 ``(N, 2)`` buffer;
       simple, either orientation, closing point optional.
   * - ``cells``
     - ``"auto"``
     - ``"required_quads"`` fails the job if the region cannot be mapped;
       ``"auto"`` falls back to triangles; ``"triangles"`` keeps the outline
       as a constraint but meshes the inside with triangles.
   * - ``spacing``
     - ``0``
     - Quadrilateral edge length; 0 derives it from the sizing.
   * - ``h_along``, ``h_across``
     - ``0``
     - Separate spacings along and across ``along_angle_degrees``; setting
       either makes the spacing directional, with no isotropic fallback.
   * - ``along_angle_degrees``
     - ``0``
     - Direction of the along axis, degrees counter-clockwise from +x.
   * - ``aspect_max``
     - ``-1``
     - Limit on the ratio of opposite side means; negative inherits the global
       limit, 0 is unbounded.
   * - ``corners``
     - automatic
     - Four ring vertex indexes, in ring order, to use as the patch corners.
   * - ``holes``
     - none
     - Rings subtracted from the region.
   * - ``attribute``
     - ``0``
     - Integer stored with the region.
   * - ``tag``
     - ``""``
     - Text tag for the region's cells; empty inherits the region marker's.

Size field
----------

``size_field`` grades sizes from ``near_size`` at the constraints outward at
the rate ``gradation``, with polygon overrides.

.. list-table::
   :header-rows: 1
   :widths: 24 16 60

   * - Key
     - Default
     - Meaning
   * - ``near_size``
     - ``0``
     - Edge length at the constraints. Must be positive.
   * - ``gradation``
     - ``0.25``
     - Growth of size per unit distance; typically 0.1–0.5.
   * - ``max_size``
     - ``0``
     - Upper clamp on the size; 0 is unbounded.
   * - ``area_floor``
     - ``0``
     - Smallest cell area the field asks for; 0 sets no floor.
   * - ``max_grid_cells``
     - 4,000,000
     - Background grid budget.
   * - ``regions``
     - none
     - ``{"ring": ..., "size": h}`` rows; inside a ring the size is at most
       ``h``.
   * - ``terrain_regions``
     - none
     - Rows of the same form, used only by terrain footprint queries.
