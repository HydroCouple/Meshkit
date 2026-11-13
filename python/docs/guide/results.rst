Results and reports
===================

A job (and :func:`meshkit.read`) returns a :class:`~meshkit.MeshResult`: the
mesh as read-only buffers, and a report.

Buffers and ownership
---------------------

Array attributes are :class:`memoryview` objects over arrays owned by native
code. Each access returns a new view, and every view keeps the native storage
alive, even after the result object is gone.

.. code-block:: python

   import numpy as np

   xyz = np.asarray(result.vertices)          # float64 (N, 3), zero-copy, read-only
   cells = np.asarray(result.cells)           # uint32 (M, 4), strided by cell
   editable = np.array(result.vertices, copy=True)

* Views are read-only; asking for a writable buffer raises :class:`BufferError`.
  Copy to edit.
* ``cells``, ``cell_types`` and ``regions`` are strided over one cell array,
  and ``edges`` and ``edge_markers`` over one edge array. NumPy shares them as
  strided arrays; a consumer that demands a C-contiguous buffer gets a
  :class:`BufferError` and must copy.
* Cells are zero-based and counter-clockwise. ``cell_types`` holds 3 or 4; a
  triangle's fourth index is unused.
* ``vertex_ids`` and ``cell_ids`` are empty when the IDs are the implicit
  one-based sequence, and ``vertex_markers`` is empty when every marker is 0.

NumPy is optional: :class:`memoryview` supports indexing, ``tolist()`` and the
buffer protocol on its own.

.. _reports:

Reports
-------

``result.report`` is a :class:`dict`. Start with ``status`` and ``complete``:

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - ``status``
     - Meaning
   * - ``complete``
     - Every requirement met.
   * - ``complete_with_exceptions``
     - A mesh is returned, but advisory requirements (such as the minimum
       target size) or other accepted exceptions were not met; see the conflict
       counts and ``examples``.
   * - ``infeasible``
     - The request cannot be met, including invalid option values; no geometry.
   * - ``resource_limit``
     - A limit in ``options["limits"]`` was reached; no geometry.
   * - ``cancelled``
     - The job was cancelled; no geometry.
   * - ``failed``
     - Any other failure; no geometry.

``complete`` is true for ``complete`` and ``complete_with_exceptions``; the
other fields explain the outcome.

.. list-table::
   :header-rows: 1
   :widths: 32 68

   * - Key
     - Meaning
   * - ``code``, ``message``
     - Machine-readable reason and its text.
   * - ``seconds``, ``work``
     - Elapsed time and work units used.
   * - ``vertices``, ``triangles``, ``quads``
     - Counts in the result.
   * - ``algorithm_version``, ``provider_identity``
     - Algorithm version and the caller's terrain identity, for provenance.
   * - ``inserted``, ``removed``, ``removal_attempts``, ``queue_peak``
     - Refinement and coarsening activity.
   * - ``below_minimum``
     - Cells smaller than the minimum target (advisory).
   * - ``edge_conflicts``, ``size_conflicts``, ``transition_conflicts``
     - Edges shorter than ``minimum_edge``, cells larger than their size limit,
       neighbours beyond ``adjacent_ratio``.
   * - ``terrain_conflicts``, ``terrain_unknown``
     - Cells whose terrain bound exceeds the tolerance, or could not be
       established.
   * - ``largest_adjacent_ratio``, ``largest_terrain_ratio``
     - Worst measured values.
   * - ``planned_patches``, ``planning_retries``, ``shared_sides``
     - Quadrilateral planning outcome.
   * - ``planning``
     - Up to 256 decisions: ``source``, ``reason``, ``region``,
       ``repair_rounds``, ``required``.
   * - ``constraint_segments``, ``constraint_audit_passed``
     - Constraint recovery and its independent audit.
   * - ``examples``
     - Up to 64 failing checks: ``code``, ``cell``, ``measured``, ``allowed``.
       The counters above hold the totals.
   * - ``resolved_options``
     - The options the job actually used, defaults filled in.

A report from :func:`meshkit.read` has the same keys, with the job-specific
counters at zero.

.. _progress-events:

Progress events
---------------

:meth:`Job.poll() <meshkit._meshkit.Job.poll>` and the ``progress`` callback of
:func:`meshkit.run` deliver events as dictionaries:

.. list-table::
   :header-rows: 1
   :widths: 24 76

   * - Key
     - Meaning
   * - ``stage``, ``message``
     - Current stage and a short description.
   * - ``percent``
     - Progress of the stage, 0–100.
   * - ``status``
     - Run status, as in the report.
   * - ``severity``
     - ``"info"``, ``"warning"`` or ``"error"``.
   * - ``vertices``, ``triangles``, ``quads``
     - Counts so far.
   * - ``sequence``, ``job``
     - Event sequence number and job number.
   * - ``elapsed_ms``
     - Milliseconds since the job started.
   * - ``dropped``
     - Events lost so far because the queue (2,048 events) was full.

The worker never waits for Python: when nobody polls, events beyond the queue
are dropped and counted.
