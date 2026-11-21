.. _source-preparation:

Preparing source geometry
=========================

Surveyed or digitized outlines often carry far more vertices than a mesh needs.
:func:`meshkit.prepare_sources` removes vertices within per-feature tolerances,
audits the result independently, and returns buffers that
:func:`meshkit.prepared_request` turns into a job request without building
Python rows per vertex.

.. code-block:: python

   prepared = meshkit.prepare_sources([
       {"id": "domain", "role": "outer", "xy": outline,
        "policy": {"mode": "simplify", "maximum_deviation": 0.5}},
       {"id": "levee", "role": "line", "xy": levee_xy, "z": levee_z,
        "policy": {"mode": "simplify", "maximum_deviation": 0.2,
                   "maximum_z_deviation": 0.05}},
   ])
   if not prepared["ok"]:
       raise RuntimeError(prepared["error"])
   request = meshkit.prepared_request(prepared, options={"maximum_size": 10.})
   result = meshkit.run(request)

Coordinates never move: simplification only removes vertices, and every
shortcut is checked against the whole stretch it replaces, in XY and Z. A
stretch shared by several features is simplified once, under the strictest of
their policies. Crossing features are not split; node them before meshing.

Features
--------

.. list-table::
   :header-rows: 1
   :widths: 16 22 62

   * - Key
     - Default
     - Meaning
   * - ``xy``
     - —
     - **Required.** Vertices as rows ``[x, y]`` or a float64 ``(N, 2)``
       buffer.
   * - ``id``
     - ``""``
     - Source identifier, returned as ``source_id``.
   * - ``z``
     - none
     - Heights, one per vertex. Features that share a point must give it the
       same height (otherwise :class:`ValueError`).
   * - ``fixed``
     - none
     - One flag per vertex; flagged vertices are never removed.
   * - ``role``
     - ``"line"``
     - ``"outer"`` (domain outline) and ``"hole"`` are closed rings;
       ``"line"`` is a constraint polyline. ``"protected"`` is accepted and
       currently treated like ``"line"``.
   * - ``closed``
     - ``False``
     - Close a line into a ring (implied for ``outer`` and ``hole``).
   * - ``marker``
     - ``0``
     - Integer carried to the path.
   * - ``policy``
     - preserve
     - Simplification policy, below.

Policy
------

The default mode, ``"preserve"``, keeps every vertex; the limits apply with
``"mode": "simplify"``.

.. list-table::
   :header-rows: 1
   :widths: 34 14 52

   * - Key
     - Default
     - Meaning
   * - ``mode``
     - ``"preserve"``
     - ``"preserve"`` or ``"simplify"``.
   * - ``maximum_deviation``
     - ``0``
     - Largest XY distance between a shortcut and the vertices it removes.
   * - ``maximum_z_deviation``
     - ``0``
     - Largest height difference, in Z units; 0 keeps heights exact.
   * - ``maximum_turn_degrees``
     - ``5``
     - Largest direction change at a removed vertex.
   * - ``maximum_area_change_fraction``
     - ``0.001``
     - Largest change of a ring's area, relative to it; -1 disables.
   * - ``maximum_area_change``
     - ``-1``
     - Largest absolute area change, in squared XY units; -1 disables.
   * - ``minimum_clearance``
     - ``0``
     - Smallest distance a shortcut keeps from other geometry.

The second argument takes ``protected_points`` (points no shortcut may pass
over), ``max_work`` (0 allows 256 operations per input vertex plus 100,000)
and ``max_workspace_bytes`` (default 1 GiB). Running out of work keeps the
original features.

Result
------

``prepared["ok"]`` is false with an ``error`` text when simplification or its
audit fails. Otherwise:

* ``points`` (float64 ``(N, 2)``) and ``point_elevations`` (float64 ``(N,)``,
  NaN where no ``z`` was given) hold every distinct retained vertex once;
* ``features`` has one row per input feature: ``id`` (one-based path ID),
  ``source_id``, ``kind``, ``closed``, ``marker``, ``vertices`` (indexes into
  ``points``), ``source_vertices`` (for each retained vertex, its index in the
  input feature; empty when the feature is unchanged) and ``removed``
  (vertices removed);
* ``shared_chains`` counts stretches shared by several features.
