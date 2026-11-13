# SPDX-License-Identifier: Apache-2.0
"""Qt-free mixed meshing with native terrain, read-only buffers and bounded jobs."""
from ._meshkit import MeshResult, capabilities, prepare_sources, read, start, write
__version__ = "0.2.0"

def run(request, *, raster=None, provider_identity="", progress=None, poll_interval=0.1):
    """Run in a native worker; deliver bounded progress batches on the caller thread.

    Inputs accept native float64 (N,2) buffers or point rows. Result arrays are
    owned, read-only strided buffers. NumPy is optional: np.asarray(view) shares
    storage. Polling/cancellation never call Python from a native inner loop.

    Any exception raised while waiting, including :class:`KeyboardInterrupt` and
    exceptions from ``progress``, cancels the job before it propagates.

    Parameters
    ----------
    request : dict
        The job description; see :ref:`requests`.
    raster : dict, optional
        Terrain grid; see :ref:`terrain`.
    provider_identity : str
        Identity of the terrain dataset and version, copied into the report.
    progress : callable, optional
        Called on the calling thread with each progress event; see
        :ref:`progress-events`.
    poll_interval : float
        Seconds between progress polls (at least 0.001).

    Returns
    -------
    MeshResult
        The finished mesh. A refused job returns empty geometry; check
        ``result.report["status"]``.

    Raises
    ------
    ValueError
        The request or raster is invalid.
    """
    job = start(request, raster=raster, provider_identity=provider_identity)
    try:
        while True:
            try:
                result = job.result(max(0.001, poll_interval))
                if progress:
                    for event in job.poll():
                        progress(event)
                return result
            except TimeoutError:
                if progress:
                    for event in job.poll():
                        progress(event)
    except BaseException:
        job.cancel()
        raise


def prepared_request(prepared, *, options=None, metadata=None, **extra):
    """Build a request from an audited source graph without per-vertex Python rows.

    Parameters
    ----------
    prepared : dict
        A successful result of :func:`prepare_sources`.
    options : dict, optional
        Job options; see :ref:`options`.
    metadata : dict, optional
        Units and descriptive metadata; see :ref:`requests`.
    **extra
        Further request keys, such as ``quad_regions``, ``size_field`` or ``labels``.

    Returns
    -------
    dict
        A request for :func:`start` or :func:`run` that uses the prepared points,
        heights and paths.

    Raises
    ------
    ValueError
        ``prepared`` reports a failure.
    """
    if not prepared.get("ok"):
        raise ValueError(prepared.get("error", "Source preparation failed"))
    paths = [{key: row[key] for key in ("vertices", "kind", "id", "marker", "closed")}
             for row in prepared["features"]]
    return dict(points=prepared["points"], point_elevations=prepared["point_elevations"], paths=paths, options=options or {},
                metadata=metadata or {}, **extra)
