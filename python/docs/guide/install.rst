Installation
============

From PyPI
---------

.. code-block:: sh

   python -m pip install hydrocouple.meshkit

Wheels are published for CPython 3.10–3.13 on Linux (x86_64, aarch64), macOS
(arm64, x86_64) and Windows (x64). The distribution is named
``hydrocouple.meshkit`` because PyPI refuses ``meshkit`` as too close to an
unrelated project; the package still imports as ``meshkit``:

.. code-block:: python

   import meshkit
   print(meshkit.__version__, meshkit.capabilities())

NumPy is not a dependency. Results are buffers that :func:`numpy.asarray` shares
without copying when NumPy is installed, and plain :class:`memoryview` objects
otherwise.

From a checkout
---------------

A source build needs Python 3.10 or later with its development headers, a C++17
compiler and CMake. No Qt, GDAL or NumPy headers are used.

.. code-block:: sh

   git clone https://github.com/HydroCouple/Meshkit.git
   cd Meshkit
   python -m pip install .

The C++ library builds on its own with CMake; see the
`C++ documentation <https://www.hydrocouple.org/Meshkit/>`_.

Building this documentation
---------------------------

The documentation imports the type stub ``meshkit/_meshkit.pyi`` in place of the
compiled extension, so no build is needed:

.. code-block:: sh

   python -m pip install -r python/docs/requirements.txt
   sphinx-build -b html -W --keep-going python/docs python/docs/_build/html
