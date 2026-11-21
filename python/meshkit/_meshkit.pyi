# SPDX-License-Identifier: Apache-2.0
"""Native MeshKit meshing and streaming I/O (``meshkit._meshkit``).

The extension is written against the CPython C API and carries no docstrings of
its own. This stub states its signatures and documents its behavior for type
checkers and for the Sphinx reference, which imports it instead of the compiled
module. Import the public names from :mod:`meshkit`.

Every mapping argument must be a :class:`dict`. Unknown keys raise
:class:`ValueError` (``Unknown field: <key>``), as do values of the wrong type or
range; native failures raise :class:`RuntimeError`.
"""
from __future__ import annotations

import os
from collections.abc import Mapping, Sequence
from typing import Any

PathLike = str | bytes | os.PathLike[str] | os.PathLike[bytes]

class MeshResult:
    """A mesh and its report: returned by :meth:`Job.result` and :func:`meshkit.read`.

    Array attributes are read-only :class:`memoryview` objects over the native
    arrays. Each access returns a new view that keeps the native storage alive, also
    after the result object is deleted. Views with a cell or edge stride are not
    C-contiguous: :func:`numpy.asarray` shares them as strided arrays, and
    ``numpy.array(view, copy=True)`` makes an editable copy. Asking for a contiguous
    or writable buffer raises :class:`BufferError`.

    A result whose report status is ``infeasible``, ``cancelled``,
    ``resource_limit`` or ``failed`` has empty geometry and mappings.
    """

    @property
    def vertices(self) -> memoryview:
        """Vertex coordinates: float64, shape ``(N, 3)`` (X, Y, Z)."""
    @property
    def cells(self) -> memoryview:
        """Cell connectivity: uint32, shape ``(M, 4)``, zero-based, counter-clockwise.

        Triangles use the first three slots; the fourth is unused. Strided by cell.
        """
    @property
    def cell_types(self) -> memoryview:
        """Corner count of each cell: uint8, shape ``(M,)``, 3 or 4. Strided by cell."""
    @property
    def regions(self) -> memoryview:
        """Region of each cell: int32, shape ``(M,)``. Strided by cell."""
    @property
    def vertex_ids(self) -> memoryview:
        """External vertex IDs: int64, shape ``(N,)``.

        Empty when the IDs are the implicit one-based sequence.
        """
    @property
    def cell_ids(self) -> memoryview:
        """External cell IDs: int64, shape ``(M,)``.

        Empty when the IDs are the implicit one-based sequence.
        """
    @property
    def vertex_markers(self) -> memoryview:
        """Vertex markers: int32, shape ``(N,)``. Empty means every marker is zero."""
    @property
    def edges(self) -> memoryview:
        """Edges stored with the mesh: uint32 vertex pairs, shape ``(E, 2)``. Strided by edge."""
    @property
    def edge_markers(self) -> memoryview:
        """Marker of each stored edge: int32, shape ``(E,)``. Strided by edge."""
    @property
    def input_mapping(self) -> memoryview:
        """Output vertex of each request point: uint32, shape ``(P,)``.

        Filled for job results when the ``retain_input_mapping`` option is on (the
        default); empty for meshes read from files.
        """
    @property
    def alias_vertices(self) -> memoryview:
        """Vertex of each source alias: uint32, shape ``(A,)``.

        An alias records a source point that coincided exactly with another one and
        so shares its vertex.
        """
    @property
    def alias_ids(self) -> memoryview:
        """Source ID of each alias: int64, shape ``(A,)``."""
    @property
    def alias_markers(self) -> memoryview:
        """Source marker of each alias: int32, shape ``(A,)``."""
    @property
    def report(self) -> dict[str, Any]:
        """The job or read report, a new :class:`dict` on each access.

        Holds ``status``, ``complete``, counts, conflicts, planning decisions,
        bounded examples and the resolved options; see :ref:`reports`.
        """
    @property
    def metadata(self) -> dict[str, Any]:
        """Units and descriptive metadata.

        Keys ``horizontal_metres_per_unit``, ``vertical_metres_per_unit`` (float),
        ``units_known`` (bool), ``crs`` and ``description`` (str).
        """
    @property
    def paths(self) -> list[dict[str, Any]]:
        """Constraint paths carried by the mesh.

        Each row has ``vertices`` (uint32 :class:`memoryview`), ``id`` (int),
        ``marker`` (int), ``closed`` (bool) and ``kind`` (``"outer"``, ``"hole"``,
        ``"region"`` or ``"line"``).
        """
    @property
    def labels(self) -> list[dict[str, Any]]:
        """Region labels: rows with ``id`` (int) and ``name`` (str)."""

class Job:
    """A meshing job running on a native worker thread; returned by :func:`meshkit.start`.

    The worker runs without the GIL and never calls Python. Deleting the job
    requests cancellation without waiting; the worker stops at its next
    cancellation check. The type is not exported by the extension; obtain jobs from
    :func:`meshkit.start`.
    """

    @property
    def done(self) -> bool:
        """True once the worker has finished, successfully or not."""
    def cancel(self) -> None:
        """Request cancellation; a running job stops at its next check with status ``cancelled``."""
    def poll(self) -> list[dict[str, Any]]:
        """Drain the progress events queued since the last call; see :ref:`progress-events`."""
    def result(self, timeout: float = -1.0, /) -> MeshResult:
        """Wait for the job and return its mesh.

        Parameters
        ----------
        timeout
            Seconds to wait; negative (the default) waits until the job is done.
            Positional only.

        Returns
        -------
        MeshResult
            The mesh and report. A refused or canceled job returns empty geometry
            with the reason in :attr:`MeshResult.report`; it does not raise.

        Raises
        ------
        TimeoutError
            The job is still running after ``timeout`` seconds.
        RuntimeError
            The native worker failed unexpectedly.
        """

def start(request: Mapping[str, Any], raster: Mapping[str, Any] | None = None,
          provider_identity: str = "") -> Job:
    """Validate a request, copy it into native storage and start a meshing job.

    The request and raster are read and copied before the function returns, so
    later changes to the caller's objects do not affect the job. Use
    :func:`meshkit.run` to wait with progress callbacks.

    Parameters
    ----------
    request
        The job description: source points, paths and options. See :ref:`requests`.
    raster
        Optional terrain grid; see :ref:`terrain`. Without it the job has no
        terrain provider and terrain tolerances cannot be certified.
    provider_identity
        Caller-supplied identity of the terrain dataset and version, copied into the
        report. MeshKit does not compute or check it.

    Returns
    -------
    Job
        The running job.

    Raises
    ------
    ValueError
        The request or raster is invalid.
    """

def read(path: PathLike, options: Mapping[str, Any] | None = None) -> MeshResult:
    """Read a mesh file.

    A ``.mkb`` extension selects the lossless MeshKit binary format; any other
    extension is read as SMS 2DM. A failed read returns no partial mesh: the report
    holds the reason.

    Parameters
    ----------
    path
        File to read (:class:`str`, :class:`bytes` or path-like).
    options
        File options; ``legacy_2dm``, ``ignore_unknown_records`` and ``limits`` apply
        to reading. See :ref:`file-options`.

    Returns
    -------
    MeshResult
        The mesh, with an empty :attr:`MeshResult.input_mapping`.

    Raises
    ------
    ValueError
        The path contains a NUL character or an option is invalid.
    """

def write(path: PathLike, mesh: MeshResult,
          options: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Write a mesh atomically as ``.mkb`` or ``.2dm``.

    The file is written to a temporary in the destination folder and published
    only when complete. An existing file is kept unless ``overwrite`` is true, and a
    2DM write refuses to drop attributes the format cannot hold unless
    ``allow_attribute_loss`` is true; both refusals return a report whose
    ``complete`` is false.

    Parameters
    ----------
    path
        Destination; its extension must be ``.mkb`` or ``.2dm``.
    mesh
        The mesh to write.
    options
        File options; see :ref:`file-options`.

    Returns
    -------
    dict
        Report with ``status``, ``code``, ``message``, ``complete``, ``seconds``,
        ``work``, ``vertices``, ``triangles``, ``quads``, ``bytes`` (bytes written)
        and ``losses`` (attribute categories dropped by an allowed lossy 2DM write).

    Raises
    ------
    TypeError
        ``mesh`` is not a :class:`MeshResult`.
    ValueError
        The extension is not ``.mkb`` or ``.2dm``, or an option is invalid.
    """

def prepare_sources(features: Sequence[Mapping[str, Any]],
                    options: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Simplify source polylines and rings, then audit the result independently.

    Shared stretches of features are simplified once under the strictest of
    their owners' policies. Coordinates never move: simplification only removes
    vertices. Pass the result to :func:`meshkit.prepared_request` to build a job
    request. See :ref:`source-preparation`.

    Parameters
    ----------
    features
        Source features, each a :class:`dict`.
    options
        Optional ``protected_points``, ``max_work`` and ``max_workspace_bytes``.

    Returns
    -------
    dict
        ``{"ok": False, "error": str}`` when simplification or its audit fails;
        otherwise ``ok`` (True), ``points`` (float64 ``(N, 2)``),
        ``point_elevations`` (float64 ``(N,)``, NaN where no height was given),
        ``features`` (one row per input feature) and ``shared_chains`` (int).

    Raises
    ------
    ValueError
        A feature is invalid, or features that share a point give it different
        heights.
    """

def capabilities() -> dict[str, Any]:
    """Describe this build.

    Returns
    -------
    dict
        ``qt_free``, ``triangles``, ``automatic_mixed`` and ``native_raster``
        (all True), ``license`` (``"Apache-2.0"``) and ``version``.
    """
