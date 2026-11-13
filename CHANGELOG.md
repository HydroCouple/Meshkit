# Changelog

## Unreleased

## 0.2.0 — first standalone release

MeshKit moves out of the OpenSWMM GUI repository (`libraries/meshkit`) into its
own repository, with its history. The library is unchanged from the GUI's
2026-11-12 copy; what is new is the repository around it.

- Builds, tests and installs on its own (`cmake -S .`); consumers use
  `find_package(MeshKit)` or fetch a tagged release with `FetchContent`.
- Continuous integration: the Qt-free targets on Linux, macOS and Windows, and
  the Qt targets on Linux.
- Release wheels: on a `v*` tag, cibuildwheel builds `meshkit` for CPython
  3.10–3.13 on Linux, macOS and Windows and publishes it, with an sdist, to PyPI.
- Apache-2.0 (rights-holder authorization recorded in RELICENSING.md); the
  vendored robust predicates keep their public-domain notice.
- Project governance shared with the other HydroCouple repositories:
  contribution guide, individual and corporate CLAs, Code of Conduct, security
  policy, privacy statement, authors, citation metadata (CITATION.cff,
  .zenodo.json), issue templates, and CodeQL and OpenSSF Scorecard workflows.
