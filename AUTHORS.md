# Authors

MeshKit began as the meshing code of the OpenSWMM GUI and became a standalone
library in 2026 (see [PROVENANCE.md](./PROVENANCE.md)). It is developed and
maintained by the contributors listed below.

## Project Lead

- **Caleb Buahin** [@cbuahin](https://github.com/cbuahin) ([ORCID 0000-0002-9859-2264](https://orcid.org/0000-0002-9859-2264)) — Author and maintainer of MeshKit: constrained Delaunay triangulation and refinement, mixed triangle/quadrilateral planning and mapped patches, size fields and terrain certificates, constraint audits, the MKB and SMS 2DM codecs, the Qt adapters, the Python bindings, the command-line tool, and the tests.

## Contributors

No contributions from outside the project lead have been merged to date. Contributors
whose work is incorporated into MeshKit will be recognized here with their name,
affiliation (if any), and the scope of their contribution, per
[CONTRIBUTING.md §5](./CONTRIBUTING.md#5-author-acknowledgment).

If you believe your contribution has been omitted or mis-described, please open an issue
or contact the Technical Manager directly.

## Third-Party Material

- **Jonathan Richard Shewchuk** — the robust geometric predicates in
  [vendor/predicates](./vendor/predicates/), placed in the public domain by their author
  (*Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates*,
  Discrete & Computational Geometry 18(3), 305–363, 1997,
  [doi:10.1007/PL00009321](https://doi.org/10.1007/PL00009321)).

## AI-Assisted Development

Portions of the MeshKit codebase were developed with the assistance of:

- **Claude** (Anthropic) -- Code generation, algorithm implementation, test development, and packaging.

See [CITATION.cff](./CITATION.cff) for the machine-readable citation record.
