# Security Policy

The MeshKit maintainers take security seriously. This document describes
how to report a vulnerability and what to expect once a report has been
received.

## Supported Versions

Security fixes target the latest **0.x** release and the `main` branch.
Until 1.0, only the most recent minor release line receives fixes.

| Version line      | Supported          |
| ----------------- | ------------------ |
| 0.2.x (current)   | :white_check_mark: |
| Earlier releases  | :x:                |

MeshKit code that is still built inside the OpenSWMM GUI repository
(`libraries/meshkit`) is covered by that repository's policy until the GUI
consumes this repository's releases.

## Reporting a Vulnerability

**Please do not file public GitHub issues for security problems.**

Use one of the private channels below so a fix can be prepared before
public disclosure:

1. **Preferred — GitHub private security advisory.**
   Open a draft advisory in the
   [Security tab](https://github.com/HydroCouple/Meshkit/security/advisories/new)
   of this repository. The maintainers receive a notification immediately
   and the advisory remains private until coordinated disclosure.
2. **Email** [support@hydrocouple.org](mailto:support@hydrocouple.org).

Please include, where possible:

- A clear description of the issue and the affected component
  (e.g. a file reader in `src/native/`, the Python bindings, the CLI).
- Affected version, commit SHA, or release tag.
- Reproduction steps and a minimal input — the mesh file (`.mkb`, `.2dm`,
  `.inp`) or the request that triggers the problem — and the observed vs.
  expected behaviour.
- Any proof-of-concept exploit, payload, or crash artifact.
- Your assessment of impact (information disclosure, integrity,
  availability, code execution) and which deployment scenarios are exposed.

## What to Expect

- **Acknowledgement** within **5 business days** of report.
- **Initial assessment** (confirmation, severity, scope) within
  **10 business days**.
- **Fix or mitigation plan** communicated within **30 days** of
  confirmation for high/critical issues; lower severities are scheduled
  into the next release.
- **Coordinated disclosure**: a public advisory and patched release are
  published together. Reporters are credited unless they request
  otherwise.

If a report does not receive a response within the acknowledgement
window, please re-send and CC a maintainer listed in
[`AUTHORS.md`](AUTHORS.md).

## Scope

In scope:

- The standard C++ library (`include/meshkit/`, `src/native/`): meshing
  jobs, the MKB and SMS 2DM readers and writers, and atomic file
  publication. Readers parse files that may come from untrusted sources,
  so memory-safety defects there are the main concern.
- The optional Qt targets (`include/core/`, `include/mesh/`, `src/core/`,
  `src/mesh/`), including the INP mesh-section and SMS 2DM readers.
- The Python bindings (`src/native/python.cpp`, `python/meshkit/`) and
  the `meshkit` command-line tool (`tools/`).
- Build, packaging, and release tooling: `.github/`, `CMakeLists.txt`,
  `cmake/`, `setup.py`, `pyproject.toml`.

Out of scope (will be triaged but not necessarily fixed in this repo):

- Issues in Qt, Python, or NumPy — please report to the dependency's own
  security channel.
- Resource exhaustion from deliberately enormous requests within
  `meshkit::Limits` a caller has explicitly raised. Requests and files that
  exceed the limits (vertices, cells, workspace, metadata and record sizes)
  are refused with a reason; a crash or memory corruption instead of that
  refusal is in scope.

## Hardening Already in Place

- Automated **CodeQL** analysis on every push and PR to `main`
  (see [`.github/workflows/codeql.yml`](.github/workflows/codeql.yml)).
- **OpenSSF Scorecard** weekly supply-chain scan
  (see [`.github/workflows/scorecard.yml`](.github/workflows/scorecard.yml)).
- Output temporaries are created exclusively and, on Linux and macOS,
  readable and writable only by their owner; a failed or cancelled write
  never leaves a partial file in place of the target.
- No third-party runtime dependencies in the Qt-free targets beyond the
  vendored public-domain predicates.
- Releases are built from tagged commits in CI and published to PyPI by
  trusted publishing; no manual artifact uploads.
