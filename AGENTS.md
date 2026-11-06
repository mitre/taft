# Working in TAFT

## Scope and purpose

This repository is **TAFT**, the Test Automation Framework for TOAC. It is a
C++20 framework and command-line application, initialized by copying the public
FMACM source into a new repository. It uses `aircraft_simulation_core` for the
simulation models. Read `README.md`, `docs/README.md`, and `CMakeLists.txt` before
changing behavior.

TAFT is a separate Git repository. Run Git and build commands from this repository root.
Public builds must not depend on sibling checkouts, MITRE-internal services, or licensed data.

## Code organization

- `AircraftDynamicsTestFramework/taft.cpp`: CLI entry point and scenario-file loading.
- `AircraftDynamicsTestFramework/`: scenario execution, loaders, readers, and writers.
- `include/framework/`: framework headers and aircraft builders.
- `unittest/src/` and `unittest/resources/`: GoogleTest code and public fixtures.
- `Run_Files/`: inherited scenario templates and CSV examples.
- `.cmake/ReleaseVersion.cmake`: version-tag validation and executable version.
- `.cmake/Packaging.cmake`: binary archive contents and dependency notices.
- `.github/workflows/`: CI, the reusable build matrix, and release publication.
- `docs/README.md` and `docs/_config.yml`: guide and GitHub Pages configuration.

Use `mitre::oss::taft` for TAFT-owned C++ declarations; `loader` and `test` may be
nested namespaces. `main()` stays global. Preserve namespaces belonging to dependencies.
Match nearby code and the repository's `.clang-format`; avoid unrelated formatting.

## Build and verification

Requirements: a C++20 compiler, Git, Make or Ninja, and CMake >= 3.29. CI uses
CMake **3.31.10** because the inherited dependency scripts need compatibility
with CMake 3.x. CMake downloads dependencies through CPM on first configuration.
A populated `CPM_SOURCE_CACHE` can be reused for offline work; do not commit local
cache paths or generated build output.

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTAFT_BUILD_TESTING=ON
cmake --build build --target TAFT --parallel 2
./build/bin/TAFT --version
cmake --build build --target run_tests --parallel 2
```

Tests use the custom `run_tests` target, not CTest registration. The test report
is `build/unittest/taft_unit_test_results.xml`; failures must return a nonzero
status. If invoking `taft_test` directly, run it from `unittest/` so its relative
resource paths work.

Useful CMake options:

| Option/target | Meaning |
| --- | --- |
| `TAFT_BUILD_EXECUTABLE` | Build the `TAFT` CLI; defaults on. |
| `TAFT_BUILD_TESTING` | Build unit tests; defaults on for top-level builds. |
| `TAFT_BADA_TARGET` | Existing BADA provider target supplied by a parent project. |
| `TAFT_FIM_SAMPLE_ALGORITHM_TARGET` | Existing sample-algorithm target supplied by a parent project. |
| `mitre::oss::taft::framework` | Framework library alias. |
| `mitre::oss::taft` | Executable alias. |

For packaging checks, build TAFT and run:

```bash
cpack --config build/CPackConfig.cmake -B build/packages
```

For C++ changes, run relevant tests and the full public `run_tests` target when
feasible. For documentation-only edits, validate the Markdown, links, Pages
configuration, and docs workflow; a simulation rebuild is not normally needed.
Report the checks actually run and any environment limitations.

## Current interfaces and simulation constraints

The initialization baseline pins `aircraft_simulation_core` **1.0.0** and
`fsloader` **1.1.0**. Inputs currently use fsloader text, not YAML. Dependency/API
migration and YAML conversion are separate work; do not fold them into unrelated
cleanup. Do not silently change dependency versions.

The public `NullAircraftPerformance` implementation throws when used for
performance calculations. Public unit tests and `--version` work without BADA;
a full simulation needs a real provider and its data. Do not add licensed BADA
data, internal providers, private endpoints, or machine-specific paths to the
public repository.

The CLI accepts one argument: `--version` or a configuration-file path. That
file begins with a scenario count followed by scenario paths. Paths inside it
resolve against the working directory. Use `./` or an absolute configuration
path to avoid the legacy bare-filename lookup. `Run_Files` contains templates,
including a BADA path and `{actype}` placeholder, rather than an independent
provider-free demonstration.

## CI, releases, and documentation

- `ci.yml` runs PR/main/merge-queue validation. **CI required** must pass all of
  the shared platform checks and the documentation build.
- `build.yml` uses Ubuntu and macOS latest runners, plus a `rockylinux:9`
  container job named `rocky-latest`. All three build, test, package, and check
  an extracted executable.
- `release.yml` validates pushed version tags, reuses the same matrix, and
  publishes complete draft releases. Stable commits must be reachable from
  `main`; prereleases can test branches.
- CMake's numeric project version must match a release tag, with an optional
  `v` prefix and prerelease suffix. Keep CLI and archive versions consistent.
- Keep the root README brief. Put detailed usage and development information in
  `docs/README.md`, which GitHub Pages publishes automatically from `main:/docs`.
- Configure the site in `docs/_config.yml`. Keep its `/taft` base path and use
  absolute GitHub URLs for repository files outside the published docs folder.
- The old `FixedMassAircraftPerformance.h` URL is intentionally deferred until
  the API refactor. Do not rewrite it as part of a routine link cleanup.

Use PRs to advance `main`. Do not merge, tag, publish a release, or change
repository protection settings unless the current task authorizes it.

## Notices and provenance

Preserve the existing approved MITRE notice wording and formatting, including
contract **693KA8-22-C-00001**, the **2026** copyright, and public-release number
**26-2350** in both READMEs. Do not replace the year with a range or insert
initialization/modification comments into the legends. Build and configuration
files intentionally have no MITRE legend; do not add one there. Preserve
third-party license and attribution notices in bundled helpers and packages.

The initial import matches FMACM commit
`2cb07ec06b02c68b640e04eff5a47c1462da0167`. Keep the origin section in the guide
and distinguish upstream FMACM links from links to TAFT-owned source.
