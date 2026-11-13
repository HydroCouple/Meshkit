# Contributing to MeshKit

Thank you for your interest in contributing to **MeshKit**! This document outlines the governance model, contribution workflow, and standards that keep the project healthy, reproducible, and stable for the applications that depend on it. Please read it carefully before opening an issue, submitting a pull request, or proposing a change to the library's API or algorithms.

MeshKit is a **library**. Applications such as the OpenSWMM GUI and HydroCoupleSDK compile against its headers and depend on the meshes it produces, so a change to its public API, its file formats, or the meshes it generates for a given request reaches every one of them. Section 8 and Section 15 describe the bar for such changes.

---

## Table of Contents

1. [Community Governance](#1-community-governance)
2. [Repository & Technical Management](#2-repository--technical-management)
3. [Succession & Delegation](#3-succession--delegation)
4. [Licensing, Intellectual Property & CLA](#4-licensing-intellectual-property--cla)
5. [Author Acknowledgment](#5-author-acknowledgment)
6. [How to Cite MeshKit](#6-how-to-cite-meshkit)
7. [Versioning Strategy](#7-versioning-strategy)
8. [API Stability & Backward Compatibility](#8-api-stability--backward-compatibility)
9. [Branching Model](#9-branching-model)
10. [Bug Fixes & Small Improvements](#10-bug-fixes--small-improvements)
11. [Pull Request Process](#11-pull-request-process)
12. [Continuous Integration & Automated Testing](#12-continuous-integration--automated-testing)
13. [Issue Reporting](#13-issue-reporting)
14. [Review Timeline Expectations](#14-review-timeline-expectations)
15. [Proposing API, Format & Algorithm Changes](#15-proposing-api-format--algorithm-changes)
16. [Conflict of Interest in Reviews](#16-conflict-of-interest-in-reviews)
17. [Documentation Standards](#17-documentation-standards)
18. [Dependency Evaluation Policy](#18-dependency-evaluation-policy)
19. [Security Vulnerability Reporting](#19-security-vulnerability-reporting)
20. [Code of Conduct](#20-code-of-conduct)

---

## 1. Community Governance

MeshKit is a community-driven open source project. All contributors are welcome regardless of affiliation, background, or experience level. The community operates on a model of open discussion, merit-based evaluation, and transparent decision-making. Key decisions affecting the project direction are made openly on GitHub, and all community members are encouraged to participate.

---

## 2. Repository & Technical Management

**Repository and technical management is the responsibility of the Technical Manager (currently [@cbuahin](https://github.com/cbuahin)).**

The Technical Manager is responsible for:

- Maintaining the integrity and health of the `main` branch and any release or experimental branches.
- Setting and enforcing coding standards, testing requirements, and documentation guidelines.
- Triaging issues and pull requests in a timely manner.
- Making final decisions on merges, releases, and branch management.
- Adjudicating proposed changes to the public API, the file formats, and the meshing algorithms (Section 15).
- Coordinating with the community on roadmap items and governance questions.

Community members who wish to take on elevated responsibilities (e.g., becoming a recurring reviewer or branch maintainer) may express interest by opening a discussion on GitHub.

---

## 3. Succession & Delegation

The Technical Manager role is a single point of authority, which must be protected against absence or unavailability.

- In the event the Technical Manager is unavailable for an extended period (e.g., travel, illness, or leave), they may designate a **Temporary Delegate** with equivalent merge and release authority. The delegation will be announced publicly in GitHub Discussions.
- If the Technical Manager steps down permanently, the outgoing manager will nominate a successor from the active contributor community. The nomination is subject to a community feedback period of at least two weeks before taking effect.
- In the absence of a named delegate and in urgent situations (e.g., a critical security fix), any two senior community reviewers may jointly approve and merge a patch to `main`, with a written rationale posted to GitHub Discussions immediately afterward.

---

## 4. Licensing, Intellectual Property & CLA

MeshKit is released under the **Apache License, Version 2.0**. Attribution requirements are recorded in [NOTICE](./NOTICE), which also identifies the robust geometric predicates in [vendor/predicates](./vendor/predicates/) that their author placed in the public domain. The rights holder's authorization for the Apache-2.0 license is recorded in [RELICENSING.md](./RELICENSING.md), and the origin of each extracted file in [PROVENANCE.md](./PROVENANCE.md).

All contributors must sign the project **Contributor License Agreement (CLA)** before their pull request can be merged. The CLA is detailed in [CLA.md](./CLA.md). Key points:

- **You retain your copyright.** The CLA grants a license; it does not transfer ownership.
- **Broad license grant.** You grant the Technical Manager a perpetual, irrevocable license to use, distribute, and **relicense** your contribution (e.g., for future dual open-source/commercial licensing).
- **Patent grant.** You grant a royalty-free patent license covering patents necessarily infringed by your contribution.
- **Representation of authority.** You confirm you have the right to submit the contribution and that it contains no unlicensed third-party material.
- **Corporate contributors** must additionally submit a Corporate CLA (CCLA) — see [CLA.md §6](./CLA.md#6-corporate-contributors).

### Signing the CLA

Sign by posting the following comment on your first pull request:

> I have read the CLA Document and I hereby sign the CLA.

You may instead sign by email, as described in [CLA.md §9](./CLA.md#9-how-to-sign). The project is adopting the [CLA Assistant GitHub Action](https://github.com/contributor-assistant/github-action); once it is enabled, a bot will ask you to sign on your first pull request and record the signature for you.

Once signed, the CLA covers all future contributions. You do not need to sign again.

If your contribution includes third-party code or data, you are responsible for ensuring that the third-party license is compatible with the Apache License, Version 2.0 and that proper attribution is added to [NOTICE](./NOTICE).

The full license text is available in [LICENSE](./LICENSE), and required attribution notices in [NOTICE](./NOTICE).

---

## 5. Author Acknowledgment

All contributors whose work is incorporated into MeshKit will be recognized in [AUTHORS.md](./AUTHORS.md).

- **Name, affiliation (if any), and scope of contribution** will be documented for each contributor.
- The Technical Manager maintains `AUTHORS.md` and will update it at each release.
- If you believe your contribution has been omitted or mis-described, please open an issue or contact the Technical Manager directly.

Contributors are encouraged to add themselves to `AUTHORS.md` as part of their pull request, subject to review and formatting consistency.

---

## 6. How to Cite MeshKit

If you use MeshKit in published research, teaching materials, or engineering reports, please cite the version you used. A machine-readable citation record is maintained in [CITATION.cff](./CITATION.cff); GitHub's "Cite this repository" button formats it for you. When citing a specific version, reference the corresponding release tag so that your citation is reproducible.

If your work depends on MeshKit's exact arithmetic, please also cite the robust predicates it uses:

```
Shewchuk, J.R., 1997. Adaptive Precision Floating-Point Arithmetic and Fast Robust
Geometric Predicates. Discrete & Computational Geometry 18(3), 305-363.
https://doi.org/10.1007/PL00009321
```

---

## 7. Versioning Strategy

MeshKit follows **Semantic Versioning (SemVer)** as defined at [semver.org](https://semver.org), with formal pre-release trajectories during testing periods.

### Version Format

```
MAJOR.MINOR.PATCH[-PRERELEASE]
```

| Component | When to increment |
|-----------|-------------------|
| `MAJOR`   | Incompatible changes to the public API, a file format, or the meshes produced for a given request |
| `MINOR`   | New functionality added in a backward-compatible manner |
| `PATCH`   | Backward-compatible bug fixes, documentation updates, or build-system changes |

**Before 1.0.** While the major version is `0`, a `MINOR` release may contain incompatible changes (SemVer item 4). Every such change is listed under a **Breaking** heading in [CHANGELOG.md](./CHANGELOG.md), and `PATCH` releases remain backward-compatible.

The version is declared in two places that must agree: `project(MeshKit VERSION …)` in [CMakeLists.txt](./CMakeLists.txt) and `version=` in [setup.py](./setup.py). Release tags are `v<version>` (e.g., `v0.2.0`); pushing a tag builds and publishes the Python wheels (Section 12).

### Pre-Release Trajectories

During active release testing periods, the following pre-release labels are used in sequence:

| Stage              | Label example     | Purpose                                                                      |
|--------------------|-------------------|------------------------------------------------------------------------------|
| Alpha              | `1.0.0-alpha.1`   | Early feature-complete builds, internal or limited testing; may be unstable  |
| Beta               | `1.0.0-beta.1`    | Feature-frozen; broader community testing; known issues may exist            |
| Release Candidate  | `1.0.0-rc.1`      | Final candidate for release; only critical bug fixes accepted                |
| Stable             | `1.0.0`           | Fully validated, production-ready release                                    |

Pre-release builds are tagged in Git and published to the appropriate distribution channel with clear labels (PEP 440 forms such as `1.0.0b1` for Python wheels) to prevent unintended use in production workflows.

---

## 8. API Stability & Backward Compatibility

MeshKit has three kinds of public surface, each with its own compatibility promise:

| Surface | What it covers | Promise |
|---|---|---|
| **Native API** | `meshkit::runtime`, `meshkit::core`, `meshkit::io` — headers under `include/meshkit/`, documented in [NATIVE_API.md](./NATIVE_API.md); the Python package | SemVer, as in Section 7 |
| **File formats** | MKB (versioned schema) and SMS 2DM as read and written by `meshkit::io`; the job report schema | Files written by a release stay readable by every later release of the same `MAJOR`; a schema change bumps its version number |
| **Qt targets** | `meshkit::core_qt`, `model_qt`, `io_qt` — headers under `include/core/` and `include/mesh/` | **Transitional.** They serve the OpenSWMM GUI and may change in any `MINOR` release, with a CHANGELOG entry |

### What counts as a breaking change

The following are breaking changes to the native API or file formats:

- Removing or renaming a public type, function, member, or enumerator.
- Changing a signature: parameter types or order, return type, constness, or exception behavior.
- Reordering or renumbering existing enumerators, or changing the meaning of a status or error code.
- Changing units, coordinate conventions, ownership, or lifetime rules documented in NATIVE_API.md.
- Changing a file format so that a file written by an earlier release of the same `MAJOR` can no longer be read, or reads differently.
- Changing the mesh produced for the same request and inputs, other than a documented bug fix. Downstream models and their regression baselines depend on meshes being reproducible.

### Deprecation process

1. **Announcement** — An API element to be removed is first marked as deprecated in the release notes and in code (via `[[deprecated]]` and a Doxygen `@deprecated` note). The announcement states the planned removal version and the replacement.
2. **Minimum notice period** — Deprecated items are not removed for at least **one full minor version cycle** following the announcement.
3. **Removal** — After 1.0, deprecated items are removed only in a `MAJOR` version bump. Before 1.0, they may be removed in a later `MINOR` release, listed under **Breaking** in the CHANGELOG.

---

## 9. Branching Model

| Branch                    | Purpose                                                                                     |
|---------------------------|---------------------------------------------------------------------------------------------|
| `main`                    | Integration branch. All feature and bug-fix branches target here; releases are tagged from it. |
| `bugfix/<issue-id>-desc`  | Short-lived branches forked from `main` to address specific bug reports.                    |
| `feature/<name>`          | Short-lived branches for additions, forked from `main`.                                     |
| `experimental/<name>`     | Sandboxed branches for exploratory or breaking work. See Section 15.                        |
| `release/<version>`       | Release preparation branches (alpha → beta → rc), cut from `main` when a release needs stabilizing. |

---

## 10. Bug Fixes & Small Improvements

For small, well-scoped bug fixes and minor improvements, follow this workflow:

1. **Open or reference an issue.** Verify the problem is reproducible and link the relevant issue number in all subsequent commits and pull requests.
2. **Fork the `main` branch.** Name your branch `bugfix/<issue-id>-short-description` (e.g., `bugfix/42-2dm-reader-overflow`).
3. **Write a failing test first.** The test must demonstrate the defect before the fix is applied.
4. **Implement the minimal fix.** Touch only the code necessary to resolve the issue. Do not refactor adjacent code.
5. **Confirm the change is not a breaking change** under Section 8. If it is, it belongs in Section 15, not here.
6. **Ensure the full test suite passes** — the Qt-free build, and the Qt build if you touched the Qt targets — on all three supported platforms, or document any failure and why it is acceptable in the pull request description.
7. **Submit a pull request** against `main` following the process described in Section 11.

---

## 11. Pull Request Process

All contributions enter the codebase through a pull request (PR). PRs must satisfy the following requirements before they can be merged:

### Required Approvals

Every PR requires **all three** of the following approvals:

| Reviewer               | Role                                                                                                                               |
|------------------------|------------------------------------------------------------------------------------------------------------------------------------|
| **Technical Manager**  | [@cbuahin](https://github.com/cbuahin) (current Technical Manager) — final authority on design, code quality, and correctness |
| **AI Copilot Review**  | Automated AI-assisted review for style, logic, and common error patterns                                                            |
| **Community Reviewer** | At least one community contributor **other than the PR author** must approve                                                       |

Self-approvals are not permitted. The PR author may not count toward the community reviewer requirement.

### PR Checklist

Before requesting review, confirm that your PR:

- [ ] Targets `main` (or the `experimental/<name>` branch agreed in a Section 15 discussion)
- [ ] CLA signed — first-time contributors must sign the [CLA](./CLA.md) before the PR can be merged
- [ ] Includes a clear description of what was changed and why
- [ ] References any related issues (e.g., `Closes #42`)
- [ ] States explicitly whether it changes the native API, a file format, or the meshes produced, and if so, links the Section 15 discussion
- [ ] Includes new or updated tests covering the change
- [ ] Builds and passes `ctest` with `-DMESHKIT_BUILD_QT=OFF` on Linux, macOS, and Windows, and with the Qt targets if they were touched
- [ ] Passes `tests/python_test.py` against a wheel built from the branch, if the Python bindings or the native API changed
- [ ] CI pipeline passes (see Section 12)
- [ ] Doxygen comments, [NATIVE_API.md](./NATIVE_API.md), and [python/README.md](./python/README.md) updated where the change affects them
- [ ] [CHANGELOG.md](./CHANGELOG.md) entry added under **Unreleased**
- [ ] `AUTHORS.md` entry has been added or updated if this is your first contribution

### Merge Policy

- Merges into `main` are performed by the Technical Manager.
- Squash merging is preferred for `bugfix` and `feature` branches to keep the `main` history clean.
- Merge commits are used when integrating a `release` branch back into `main` to preserve the full history.

---

## 12. Continuous Integration & Automated Testing

All pull requests are automatically tested via the project's CI pipeline (GitHub Actions) before human review begins. A failing CI pipeline will block merge regardless of approvals.

| Workflow file | Name | When | What it does |
|---|---|---|---|
| `ci.yml` | **CI** | Push and PR to `main` | Builds the Qt-free targets with tests and runs `ctest` on ubuntu-latest, macos-14, and windows-latest; builds the Qt targets with tests and runs `ctest` on ubuntu-latest (Qt 6.5) |
| `codeql.yml` | **CodeQL** | Push and PR to `main`, weekly | Security analysis of the C++ (all targets, built) and Python sources |
| `scorecard.yml` | **Scorecard supply-chain security** | Push to `main`, weekly | OpenSSF Scorecard supply-chain checks |
| `release_wheels.yml` | **Release Wheels** | `v*` tags, manual (manual runs build without publishing) | cibuildwheel builds `meshkit` for CPython 3.10–3.13 on Linux, macOS, and Windows, tests each wheel, builds an sdist, and publishes them to PyPI by trusted publishing |

Contributors are expected to build and run the test suite locally before opening a PR to minimize unnecessary CI failures:

```sh
cmake -S . -B build -DMESHKIT_BUILD_QT=OFF -DMESHKIT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Further build instructions are in the [README](./README.md#build-and-use) and [NATIVE_API.md](./NATIVE_API.md).

---

## 13. Issue Reporting

### Bug Reports

A complete bug report must include:

- A clear, descriptive title.
- The version of MeshKit affected (e.g., `0.2.0`) or the commit SHA.
- How MeshKit is used: C++ native API, Qt targets, Python package, or the `meshkit` command-line tool.
- The operating system, compiler, and (for Python) the Python version.
- A minimal reproducible example — the smallest input (mesh or constraint file, or the request in code) that demonstrates the problem.
- Observed vs. expected behavior, including the operation report's status, code, and message.

Incomplete bug reports may be closed or deprioritized pending clarification.

### Feature Requests & Ideas

Open an issue for small, well-scoped additions. For proposals that would change the native API, a file format, or the meshes produced, use the GitHub Discussions **Ideas** section instead (see Section 15).

### Issue Triage

The Technical Manager or a designated reviewer will triage new issues within **two weeks** of submission, assigning labels (e.g., `bug`, `enhancement`, `needs-clarification`, `wontfix`) and priority. Contributors are encouraged to comment on existing issues before opening duplicates.

---

## 14. Review Timeline Expectations

MeshKit is maintained by volunteers. The following timelines are targets, not guarantees, but the Technical Manager is committed to keeping them:

| Action                                    | Target Timeline          |
|-------------------------------------------|--------------------------|
| Initial issue triage                      | Within 2 weeks           |
| First review response on a PR             | Within 3 weeks           |
| Follow-up review after requested changes  | Within 2 weeks           |
| API, format, or algorithm change discussion response | Within 4 weeks |

If you have not received a response within the stated window, you are welcome to post a polite follow-up comment on the issue or PR. PRs that become stale (no activity for 60 days) may be closed with a note that they can be reopened when the contributor is ready to continue.

---

## 15. Proposing API, Format & Algorithm Changes

Any change that is breaking under Section 8 — to the native API, to a file format, or to the meshes produced for a given request — and any new meshing algorithm follow this process rather than the ordinary bug-fix workflow.

### Step 1 — Open a Community Discussion

Open a thread in the GitHub Discussions **Ideas** section describing:

- The meshing capability that is missing or the defect in the current design.
- The proposed API or format change, in the form of the actual declarations or schema.
- Which downstream users would be affected (for example the OpenSWMM GUI or HydroCoupleSDK), and how.
- Whether the change is backward-compatible under Section 8, and if not, why a breaking change is warranted.
- Prior art: the published method the change follows, and how other meshing libraries address the same problem.

### Step 2 — Community Feedback

Allow at least **four weeks** for feedback before proceeding. Changes to what downstream applications compile against or read are difficult to reverse, so the discussion period is deliberately longer than for ordinary features.

### Step 3 — Experimental Branch Implementation

Implement the proposal on an `experimental/<name>` branch, including:

- The headers with complete Doxygen documentation, and NATIVE_API.md updates.
- The corresponding Python bindings, where the native API changes.
- Tests demonstrating the change, including exact checks on the meshes produced (counts, constraint and size audits) for representative inputs.
- For an algorithm change, before-and-after evidence on the cases recorded in [VALIDATION.md](./VALIDATION.md): cell quality, constraint conformance, run time, and peak memory.

### Step 4 — Peer Review

These changes receive review from the Technical Manager plus at least one maintainer of a downstream application, who evaluates the migration cost from that side.

### Step 5 — Documentation & Migration Notes

Before merge, the proposal must include:

- Updated Doxygen documentation, NATIVE_API.md, and python/README.md.
- A [CHANGELOG.md](./CHANGELOG.md) entry.
- For a breaking change, migration notes describing what downstream users must do to adapt.

### Step 6 — Merge & Release Targeting

The Technical Manager determines which release the change targets, per the Section 7 versioning rules.

---

## 16. Conflict of Interest in Reviews

Scientific software reviews can intersect with contributors' professional interests. To protect the integrity of the review process:

- A reviewer who has a competing or closely related implementation, active publication, or financial interest in the outcome of a review **must disclose this conflict** before participating in the review.
- A conflicted reviewer may participate in discussion but **may not serve as one of the three required approvers** for that PR.
- The Technical Manager makes the final determination on whether a disclosed conflict disqualifies a reviewer on a case-by-case basis.
- Undisclosed conflicts of interest, if discovered after a merge, may trigger a re-review of the affected contribution.

---

## 17. Documentation Standards

MeshKit's reference documentation is its headers, [NATIVE_API.md](./NATIVE_API.md) for the native contract, and [python/README.md](./python/README.md) for the Python package. Validation evidence is recorded in [VALIDATION.md](./VALIDATION.md).

All public-facing code elements (classes, structs, functions, enumerations, and their members) must include Doxygen-compatible documentation comments.

Minimum documentation per element:

- `@brief` — one-line summary.
- `@param` — description of each parameter including units where applicable.
- `@return` — description of the return value.
- `@note` / `@warning` — any important behavioral caveats.
- Ownership and lifetime semantics for any pointer, reference, or buffer crossing the API boundary.
- Thread-safety and cancellation behavior where they apply.
- `@cite` or a reference to the published method the code follows, where one exists.

Documentation is reviewed as part of the pull request process. PRs with undocumented public API will not be approved.

---

## 18. Dependency Evaluation Policy

Adding a third-party dependency increases the maintenance burden and risk surface of the project. The Qt-free targets (`runtime`, `core`, `io`) depend only on the C++17 standard library, threads, and the vendored public-domain predicates, and the Python wheel bundles nothing else; downstream applications rely on that.

All proposed new dependencies must be evaluated against the following criteria before inclusion:

| Criterion                | Requirement                                                                                      |
|--------------------------|--------------------------------------------------------------------------------------------------|
| **License compatibility**| The dependency's license must be compatible with the Apache License, Version 2.0, and must not impose obligations on downstream users that MeshKit's own license does not. |
| **Maintenance status**   | The dependency must be actively maintained with a responsive upstream community.                 |
| **Stability**            | The dependency must have a stable, versioned API. Unpinned or volatile dependencies are not acceptable. |
| **Target exposure**      | A new dependency of the Qt-free targets, or one whose types appear in their public headers, requires an exceptionally strong justification. |
| **Platform support**     | The dependency must support all officially targeted platforms (Linux, macOS, Windows).           |

Proposals to add a new dependency should be raised in a GitHub Discussion or issue before implementation. Optional or test-only dependencies that are not required for the core build may be included under a CMake option, with lower scrutiny, provided they meet the license requirement.

---

## 19. Security Vulnerability Reporting

**Do not report security vulnerabilities through public GitHub issues.**

If you discover a potential security vulnerability in MeshKit, please report it privately so that a fix can be prepared before the issue is publicly disclosed. [SECURITY.md](./SECURITY.md) describes the channels, scope, and timelines:

- **GitHub Private Advisory:** Use the [Security Advisories](https://github.com/HydroCouple/Meshkit/security/advisories/new) tab to submit a draft advisory directly on GitHub. Tag [@cbuahin](https://github.com/cbuahin) (current Technical Manager) in the advisory.
- **Email:** [support@hydrocouple.org](mailto:support@hydrocouple.org).

Please include in your report:

- A description of the vulnerability and its potential impact.
- Steps to reproduce, including any relevant input files or code.
- The version(s) of MeshKit affected.
- Any known mitigations or workarounds.

The Technical Manager will acknowledge receipt within **five business days** and work with you on a coordinated disclosure timeline. Credit for responsibly disclosed vulnerabilities will be given in the release notes and security advisory, unless the reporter prefers to remain anonymous.

---

## 20. Code of Conduct

MeshKit is committed to providing a welcoming, respectful, and inclusive environment for all contributors. All participants are expected to follow the [Code of Conduct](./CODE_OF_CONDUCT.md) (Contributor Covenant 3.0) and to:

- Engage constructively and professionally in all project spaces (issues, pull requests, discussions).
- Respect differing viewpoints and scientific perspectives.
- Accept constructive criticism of their contributions in good faith.
- Prioritize the long-term health of the project over individual preferences.

Harassment, personal attacks, or exclusionary behavior of any kind will not be tolerated. Violations may be reported as described in the Code of Conduct, or to the Technical Manager at [@cbuahin](https://github.com/cbuahin). Reported incidents will be reviewed and addressed promptly and confidentially.

---

*This document is maintained by the Technical Manager of MeshKit (currently [@cbuahin](https://github.com/cbuahin)). Last updated: November 2026.*
