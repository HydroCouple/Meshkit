# Changelog

## Unreleased

## 0.2.0 — first standalone release

MeshKit moves out of the OpenSWMM GUI repository (`libraries/meshkit`) into its
own repository, with its history. Apart from the fixes below, the library is
unchanged from the GUI's 2026-11-12 copy; what is new is the repository around it.

Fixed:

- The robust predicates are compiled without floating-point contraction
  (`-ffp-contract=off`). GCC fuses multiply-adds by default wherever the CPU has
  FMA, which every arm64 CPU does; a fused multiply-add skips a rounding the
  exact `orient2d`/`incircle` arithmetic depends on. On Linux arm64 that made
  constraint insertion and point location fail (`meshkit_meshcdt_test`,
  `meshkit_meshgenerator_v2_test`); Apple Clang can fuse within an expression.
- macOS wheels for x86_64, which cibuildwheel cross-builds on arm64 runners,
  now contain x86_64 code: `setup.py` passes the architecture named in
  `ARCHFLAGS` to CMake, and always sets one, so a reused build tree cannot keep
  another.

Repository:


- Builds, tests and installs on its own (`cmake -S .`); consumers use
  `find_package(MeshKit)` or fetch a tagged release with `FetchContent`.
- Continuous integration: the Qt-free targets on Linux, macOS and Windows, and
  the Qt targets on Linux. macOS jobs run on `macos-latest` (GitHub retired the
  `macos-14` runners).
- Release wheels: on a `v*` tag, cibuildwheel builds the Python package for
  CPython 3.10–3.13 on Linux, macOS and Windows and publishes it, with an sdist,
  to PyPI as `hydrocouple.meshkit` (`pip install hydrocouple.meshkit`, then
  `import meshkit`; PyPI refuses the name `meshkit` as too close to the
  unrelated `mesh-kit`). The release refuses a tag that does not match the
  version in `CMakeLists.txt` and `setup.py`.
- Apache-2.0 (rights-holder authorization recorded in RELICENSING.md); the
  vendored robust predicates keep their public-domain notice.
- Project governance shared with the other HydroCouple repositories:
  contribution guide, individual and corporate CLAs, Code of Conduct, security
  policy, privacy statement, authors, citation metadata (CITATION.cff,
  .zenodo.json), issue templates, and CodeQL and OpenSSF Scorecard workflows.
