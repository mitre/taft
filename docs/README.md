# TAFT documentation

**TAFT** (Test Automation Framework for TOAC) is a C++ framework for running
configured aircraft-dynamics scenarios and writing CSV results for research and
test automation. It uses the public
[aircraft_simulation_core](https://github.com/mitre/aircraft_simulation_core)
library for aircraft models and simulation infrastructure.

- [Origin: a copy of FMACM](#origin-a-copy-of-fmacm)
- [Requirements](#requirements)
- [Build and test](#build-and-test)
- [Aircraft-performance providers](#aircraft-performance-providers)
- [Run a scenario](#run-a-scenario)
- [Repository layout](#repository-layout)
- [Continuous integration](#continuous-integration)
- [Releases and packages](#releases-and-packages)
- [Maintaining this documentation](#maintaining-this-documentation)
- [References, license, and public release](#references-license-and-public-release)

## Origin: a copy of FMACM

TAFT started as a copy of the `master` branch of
[MITRE's FMACM repository](https://github.com/mitre/FMACM), imported into a new
repository with a clean Git history. It retains FMACM's aircraft-dynamics,
control, input-reading, and scenario-running code as its initial baseline.

New TAFT work uses the `mitre::oss::taft` C++ namespace, the `TAFT` executable,
and an independent version sequence beginning at `0.0.1`.

## Requirements

- A C++20 compiler, Git, and a build tool such as Make or Ninja.
- CMake 3.29 or newer. CI uses **3.31.10** to support the inherited dependency
  build scripts; use that version when reproducing CI locally.
- Internet access on the first configure, or a populated CPM source cache.
  CMake fetches the public dependencies automatically.
- An aircraft-performance provider and its data for simulation runs. These are
  not required for the public unit tests or `TAFT --version`.

TAFT pins `aircraft_simulation_core` to `2.1.2` and `yaml-cpp` to `0.9.0`.
Run manifests and scenario settings use YAML. Trajectory, truth-weather,
traffic, and speed-command time series remain CSV inputs. The root
[CMakeLists.txt](https://github.com/mitre/taft/blob/main/CMakeLists.txt)
contains the dependency versions.

## Build and test

Run these commands from the TAFT repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTAFT_BUILD_TESTING=ON
cmake --build build --target TAFT --parallel 2
./build/bin/TAFT --version
cmake --build build --target run_tests --parallel 2
```

The executable is **`build/bin/TAFT`**. The test report is
`build/unittest/taft_unit_test_results.xml`. Tests run through the custom
`run_tests` target, which builds the test executable and runs it with the
correct resource directory.

Add `-G Ninja` to the configure command if Ninja is installed and you prefer that
build tool. The Makefile also provides `make release` and `make tests` shortcuts.

For an embedded library build without the command-line executable or tests:

```bash
cmake -S . -B build-library -DCMAKE_BUILD_TYPE=Release \
  -DTAFT_BUILD_EXECUTABLE=OFF -DTAFT_BUILD_TESTING=OFF
cmake --build build-library --target taft_framework --parallel 2
```

The framework's CMake alias is `mitre::oss::taft::framework`; the executable's
alias is `mitre::oss::taft`.

## Aircraft-performance providers

Propagating the aircraft dynamics requires an aircraft-performance
implementation. The public build supplies `NullAircraftPerformance` so that the
project can compile and run provider-independent tests, but its performance
methods throw when called. A successful build or `--version` check does not
mean that a full simulation can run without a provider.

MITRE's internal implementation uses EUROCONTROL BADA data. That implementation
and its licensed data are not included in the public repository or release
packages. Obtain appropriate data and provide the corresponding implementation
separately.

Extend the pinned simcore 2.1.2
[`mitre::oss::simcore::FixedMassAircraftPerformance` interface](https://github.com/mitre/aircraft_simulation_core/blob/2.1.2/include/public/FixedMassAircraftPerformance.h)
in your provider library. Providers must use compatible simcore types; rebuild
providers compiled against simcore 1.x before linking them with TAFT.

The existing CMake integration hooks are:

| Option | Purpose |
| --- | --- |
| `TAFT_BADA_TARGET` | Link an existing BADA implementation target and enable the BADA-specific loader path. |

A parent CMake project must define a supplied target before adding TAFT. For a
custom provider, connect its construction through the aircraft builder or
[FrameworkAircraftLoader.cpp](https://github.com/mitre/taft/blob/main/AircraftDynamicsTestFramework/loaders/FrameworkAircraftLoader.cpp);
linking a target alone does not register an arbitrary implementation with the
command-line loader.

## Run a scenario

The [entry point](https://github.com/mitre/taft/blob/main/AircraftDynamicsTestFramework/taft.cpp)
accepts exactly one argument: either `--version` or a configuration-file path.

A run manifest lists one or more scenario YAML files in execution order:

```yaml
scenarios:
  - test-framework-scenario.yaml
```

Each scenario contains a nonempty aircraft list. For example:

```yaml
bada_data_path: /path/to/bada
aircraft:
  - ac_type: REPLACE_WITH_AIRCRAFT_CODE
    speed_management_type: pitch
    guidance:
      hfp_csv_file: example-horizontal-path.csv
      vfp_csv_file: example-vertical-profile.csv
    speed_commands:
      csv_file: example-speed-commands.csv
      pilot_delay_seconds: 0
```

The [Run_Files directory](https://github.com/mitre/taft/tree/main/Run_Files)
contains a complete YAML template and public CSV inputs. Replace the aircraft
code and BADA path with values for your provider. A public build can parse the
configuration, but a full simulation requires a real performance provider and
its data. `bada_data_path` is required when using the BADA integration.

Launch from the repository root, for example:

```bash
./build/bin/TAFT Run_Files/test-framework-configuration.yaml
```

The command-line file path resolves from the working directory. Paths inside
each YAML file resolve relative to that file, including scenario paths in the
manifest and CSV/provider paths in scenarios. Absolute paths are accepted.
CSV output remains in the working directory; output names use the scenario
filename without its `.yaml` or `.yml` extension.

Aircraft settings:

| Setting | Meaning/default |
| --- | --- |
| `ac_type` | Required provider aircraft code. |
| `speed_management_type` | Required `pitch` or `thrust`. |
| `initial_time_seconds` | Nonnegative integer; defaults to `0`. |
| `initial_mass_fraction` | Fraction in `[0, 1]`; defaults to `0.5`. |
| `env_csv_file` | Optional truth-weather CSV; omission uses zero wind. |
| `env_data_index` | `time` (default) or `dtg`. |
| `traffic_csv_file` | Optional prerecorded ADS-B traffic CSV. |
| `guidance` | Required trajectory configuration described below. |
| `speed_commands` | Optional CSV speed playback and pilot delay. Omission uses planned guidance. |

Guidance requires `vfp_csv_file` and exactly one of `hfp_csv_file` or
`waypoint_sequence_csv_file`. `hfp_compute_xy` defaults to `true` for HFP data.
To select Mach at and above a transition altitude, specify both
`planned_transition_altitude_feet` and `planned_cruise_mach` (in `[0, 1]`).
Omitting the pair selects IAS guidance throughout the profile.

The `speed_commands` mapping requires `csv_file`; `pilot_delay_seconds` is
nonnegative and defaults to zero. Its CSV has a header followed by at least two
samples with time in seconds and IAS in meters per second. Times must be finite
and strictly increasing; speeds must be positive and finite. Playback overrides
planned IAS/Mach selection with the replayed IAS while a delayed sample is
available. Outside that interval, planned guidance applies.

Unknown YAML fields, missing required settings, invalid values, and conflicting
horizontal sources are errors reported with their file and field. Legacy text
configuration and sample-algorithm integration settings are no longer supported.

Logging is configured by `log4cplus.properties`; the repository and binary
packages include a starting configuration.

## Repository layout

| Location | Contents |
| --- | --- |
| `AircraftDynamicsTestFramework/` | Scenario execution, readers, loaders, writers, and the command-line entry point. |
| `include/framework/` | Framework headers and builder interfaces. |
| `unittest/` | GoogleTest sources and public test resources. |
| `Run_Files/` | YAML run/scenario templates and CSV inputs. |
| `.cmake/` | Dependency helpers, release-version validation, and packaging setup. |
| `.github/workflows/` | CI, shared builds, and release publication. |
| `docs/` | This guide and the GitHub Pages configuration. |
| `AGENTS.md` | Repository guidance for coding agents. |

## Continuous integration

GitHub Actions builds TAFT, runs unit tests, and checks extracted binary packages
on `ubuntu-latest`, `macos-latest`, and Rocky Linux 9. The Rocky job is named
`rocky-latest` and runs in `rockylinux:9` on an Ubuntu host; that container tag
tracks the latest minor release in the Rocky Linux 9 series.

The workflow layout follows `aircraft_simulation_core`:

- [ci.yml](https://github.com/mitre/taft/blob/main/.github/workflows/ci.yml)
  validates PRs targeting `main`, pushes to `main`, and merge-queue groups.
- [build.yml](https://github.com/mitre/taft/blob/main/.github/workflows/build.yml)
  provides the shared build, unit-test, and package-check matrix.
- [release.yml](https://github.com/mitre/taft/blob/main/.github/workflows/release.yml)
  validates pushed version tags and publishes tested packages.

CI also builds this guide with GitHub's Jekyll Pages builder. The **`CI required`**
check passes only when the documentation build and the complete platform matrix
succeed. Select that check in the `main` ruleset. Platform jobs upload their test
reports and binary archives; the docs job uploads a rendered site preview.

## Releases and packages

Download published packages from [GitHub Releases](https://github.com/mitre/taft/releases).

Tags must match the numeric project version in `CMakeLists.txt`. Both `0.0.1`
and `v0.0.1` are supported; a suffix such as `v0.0.1-rc.1` creates a prerelease.
The tagged commit must be the checkout being validated. Stable release commits
must be reachable from `main`; prereleases can test unmerged branch commits.

After merging the intended release commit into `main`, create and push its tag:

```bash
git tag v0.0.1
git push origin v0.0.1
```

Tag validation and all three platform builds must pass before publication.
CI assembles a draft release, attaches three binary archives, a source archive,
and `SHA256SUMS`, then publishes it with generated notes. Drafts can be resumed;
published releases are not replaced. Notes group changes by breaking changes,
features, fixes, documentation, and other changes.

Binary archives are named `taft-VERSION-PLATFORM-ARCH.tar.gz`. They include
`bin/TAFT`, logging configuration, this guide, example inputs, and license
notices. Extract an archive matching your OS and architecture and run
`./bin/TAFT --version` from the extracted top-level directory. Providers and
licensed performance data are supplied separately.

To build a binary archive locally after compiling TAFT:

```bash
cpack --config build/CPackConfig.cmake -B build/packages
```

## Maintaining this documentation

The published site is [mitre.github.io/taft](https://mitre.github.io/taft/).
GitHub Pages publishes the `docs` folder from `main`. Merging documentation
changes into `main` automatically triggers the Pages build and deployment; no
separate website repository or manual HTML edits are needed.

Edit **`docs/README.md`** for the guide and **`docs/_config.yml`** for its title,
description, theme, and `/taft` base path. Keep the root README brief and point
it here. Small-screen styling lives in `docs/assets/css/style.scss` and extends
the existing Minimal theme. Use full GitHub URLs for repository files outside `docs` so that those
links work on both GitHub and the published site. Retain FMACM URLs only for
upstream provenance and explicitly deferred legacy references.

Open a PR to preview the documentation build before merging. The `Documentation`
job checks the generated homepage and uploads a `documentation-preview`
artifact. Production publication continues through the existing Pages
`main:/docs` setting; the PR preview does not deploy the site.

## References, license, and public release

- MITRE's [point-mass aircraft-model derivation](https://www.mitre.org/publications/technical-papers/derivation-of-a-point-mass-aircraft-model-used-for-fast-time)
  documents the inherited model background.
- [RTCA](https://www.rtca.org) publishes the applicable standards, including
  DO-328B and DO-361A. Consult those documents for requirements and applicability.
- TAFT is distributed under the
  [Apache License 2.0](https://github.com/mitre/taft/blob/main/LICENSE).

> This is the copyright work of The MITRE Corporation, and was produced for the U. S. Government under
> Contract Number 693KA8-22-C-00001, and is subject to Federal Aviation Administration Acquisition
> Management System Clause 3.5-13, Rights In Data-General (Oct. 2014), Alt. III and Alt. IV (Jan.
> 2009).
>
> No other use other than that granted to the U. S. Government, or to those acting on behalf of
> the U. S. Government, under that Clause is authorized without the express written permission of The
> MITRE Corporation.
>
> For further information, please contact The MITRE Corporation, Contracts
> Management Office, 7515 Colshire Drive, McLean, VA 22102-7539, (703) 983-6000.
>
> (c) 2026 The MITRE Corporation. All Rights Reserved.
>
> Approved for Public Release; Distribution Unlimited. 26-2350

MITRE requests that deployments or embedded uses be reported to
[opensource@mitre.org](mailto:opensource@mitre.org). Questions about MITRE open
source technologies can be sent to the same address.
