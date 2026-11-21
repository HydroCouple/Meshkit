=======
MeshKit
=======

**Mixed triangle and quadrilateral meshing with terrain certificates — Python package.**

MeshKit builds two-dimensional meshes from source points and constraint paths:
constrained Delaunay triangles, or mixed meshes with mapped quadrilateral patches
and triangle infill. Sizes follow a global limit, local size fields and an
optional terrain tolerance checked against a raster. Every job returns a report
that says whether each requirement was met, and meshes are read and written as
lossless MeshKit binary (``.mkb``) or SMS 2DM files.

The :mod:`meshkit` package wraps the Qt-free C++ library. Jobs run on native
worker threads without the GIL, results are owned read-only buffers that NumPy
can share without copying, and NumPy itself is optional.

.. code-block:: sh

   python -m pip install hydrocouple.meshkit

The distribution is named ``hydrocouple.meshkit``; the package imports as
``meshkit``.

----

Where to start
==============

.. grid:: 2

    .. grid-item-card:: Quickstart
        :link: guide/quickstart
        :link-type: doc

        Mesh a square with a quadrilateral region, read the report and save
        the result.

    .. grid-item-card:: Requests and options
        :link: guide/requests
        :link-type: doc

        Points, paths, sizing options, quadrilateral regions and terrain.

    .. grid-item-card:: Results and reports
        :link: guide/results
        :link-type: doc

        Result buffers, ownership, job status and progress events.

    .. grid-item-card:: API Reference
        :link: api
        :link-type: doc

        Every function and type, generated from the package and its type stub.

----

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Getting started

   guide/install
   guide/quickstart

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: User guide

   guide/requests
   guide/results
   guide/sources
   guide/files

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Reference

   api
   license

----

Project at a glance
===================

.. list-table::
   :widths: 30 70

   * - **Audience**
     - Modellers and developers who need conforming 2D meshes for
       hydrodynamic and other simulations.
   * - **Language**
     - Python ≥ 3.10; NumPy optional.
   * - **Wheels**
     - CPython 3.10–3.13 on Linux (x86_64, aarch64), macOS (arm64, x86_64)
       and Windows (x64).
   * - **Build system**
     - setuptools + CMake; a C++17 compiler for source builds.
   * - **License**
     - Apache License, Version 2.0.

Related documentation
=====================

* `MeshKit C++ documentation <https://www.hydrocouple.org/Meshkit/>`_ — the
  native library these bindings wrap: API reference, the native contract,
  validation evidence and provenance. *(The "C++ API Docs" link in the top
  navigation also goes here.)*
* `Source repository on GitHub <https://github.com/HydroCouple/Meshkit>`_ —
  source, build instructions, contributing guide, issue tracker.
