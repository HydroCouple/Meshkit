Reading and writing files
=========================

.. code-block:: python

   report = meshkit.write("site.mkb", result)
   mesh = meshkit.read("site.mkb")
   report = meshkit.write("site.2dm", mesh, options={"allow_attribute_loss": True})
   print(report["losses"])

Formats
-------

``.mkb`` — MeshKit binary
   Lossless for everything a :class:`~meshkit.MeshResult` holds: coordinates,
   cells, edges, paths, IDs, markers, region labels, source aliases and
   metadata. Little-endian, versioned, with a trailing checksum that detects
   accidental corruption (it is not a signature). :func:`meshkit.read` selects
   it by the ``.mkb`` extension.

``.2dm`` — SMS 2DM
   Read and written with ``MESH2D``, ``ND``, ``E3T``, ``E4Q`` and ``NS``
   records. Clockwise cells are reoriented; degenerate or non-convex cells
   fail. 2DM cannot hold units, CRS, description, labels, aliases, vertex or
   edge markers or path roles and IDs, so writing a mesh that has them is
   refused unless ``allow_attribute_loss`` is true; the report's ``losses``
   lists what was dropped. Any extension other than ``.mkb`` is read as 2DM.

Writes are atomic: the file is written to a temporary in the destination folder
and published only when complete, so a failed or cancelled write leaves an
existing file as it was. A failed read returns no partial mesh.

.. _file-options:

File options
------------

.. list-table::
   :header-rows: 1
   :widths: 28 12 60

   * - Key
     - Default
     - Meaning
   * - ``overwrite``
     - ``False``
     - Replace an existing file (write). Without it, writing to an existing
       path returns a report whose ``complete`` is false.
   * - ``durable``
     - ``False``
     - Flush the temporary file to storage before publishing it (write).
   * - ``allow_attribute_loss``
     - ``False``
     - Let a 2DM write drop attributes 2DM cannot hold (write).
   * - ``legacy_2dm``
     - ``False``
     - Accept historical non-positive IDs and optional or text material labels
       (2DM read).
   * - ``ignore_unknown_records``
     - ``False``
     - Skip unknown 2DM cards and count them, returning
       ``complete_with_exceptions``, instead of failing (2DM read).
   * - ``limits``
     - see :ref:`options`
     - Count, workspace and record-size limits.
