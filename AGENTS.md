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
- `AircraftDynamicsTestFramework/`: scenario execution, YAML parsing, builders, readers, and writers.
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

### Test ownership

Keep tests with the component whose behavior their assertions verify:

- `configuration_tests.cpp`: YAML records, defaults, paths, validation, and batch naming; no model construction.
- `guidance_input_tests.cpp`: HFP/VFP/waypoint parsing and guidance construction from inputs.
- `static_guidance_tests.cpp`: guidance calculations, speed selection, endpoints, and shipped-route integration.
- `weather_truth_tests.cpp`: weather loading, sample selection, reloads, and zero wind.
- `adsb_replay_tests.cpp`: traffic loading, report selection, timing, and history.
- `speed_commands_tests.cpp`: in-memory replay and CSV/application loading in separate suites.
- `aircraft_state_writer_tests.cpp`: output columns, aircraft identifiers, and write failures.

Keep schema, file-loading, and runtime checks when they protect distinct entry
points. Share only temporary-file setup in `test_support.h`; keep domain helpers
local. Do not add duplicate cases to increase test counts.

## Current interfaces and simulation constraints

TAFT pins `aircraft_simulation_core` **2.1.2** and `yaml-cpp` **0.9.0**.
Simcore types use `mitre::oss::simcore`; keep provider-owned namespaces unchanged.
YAML parsing produces plain configuration records before constructing aircraft.
Keep yaml-cpp types private to the parser. 

The public `NullAircraftPerformance` implementation throws when used for
performance calculations. Public unit tests and `--version` work without BADA;
a full simulation needs a real provider and its data. Do not add licensed BADA
data, internal providers, private endpoints, or machine-specific paths to the
public repository.

The CLI accepts one argument: `--version` or a run-manifest path. The manifest
has a nonempty `scenarios` list of scenario YAML paths. Each scenario has a
nonempty `aircraft` list. Input paths resolve relative to the YAML containing
them; the command-line path and CSV output resolve from the working directory.
There is no special bare-filename lookup or legacy text parser. Output names
use the scenario filename stem for both `.yaml` and `.yml`.

Follow established AAESim application conventions when making shared design
decisions; check the related implementation before assuming its behavior.
Scenario output is one `<scenario>_AcStates.csv`. Multi-aircraft runs prepend
an `acid` column with IDs starting at 1 in YAML order; single-aircraft runs
retain their existing ten columns.

Keep CSV trajectory, truth-weather, ADS-B replay, and speed-command data formats.
Speed playback and nonnegative pilot delay are generic capabilities. When
replay has a valid sample it selects IAS; otherwise planned guidance applies.
`Run_Files` includes a BADA path and aircraft-code placeholder, rather than an
independent provider-free simulation. Test parsing and builders separately so
public configuration tests do not require licensed data.

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
- Keep the `FixedMassAircraftPerformance.h` documentation link aligned with the
  pinned simcore version. Optional providers must use compatible simcore types.

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
