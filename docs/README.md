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

The initialization baseline pins `aircraft_simulation_core` to `1.0.0` and
`fsloader` to `1.1.0`. Input configuration currently uses the inherited fsloader
text format. Changes to these APIs and input formats are separate development
work; consult the root
[CMakeLists.txt](https://github.com/mitre/taft/blob/main/CMakeLists.txt)
for the actual dependency versions.

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

The inherited
[FixedMassAircraftPerformance.h reference](https://github.com/mitre/FMACM/blob/master/include/public/FixedMassAircraftPerformance.h)
is retained pending the planned API refactor. For current interfaces, inspect
the pinned dependency and the TAFT loader code rather than relying on that
legacy link.

The existing CMake integration hooks are:

| Option | Purpose |
| --- | --- |
| `TAFT_BADA_TARGET` | Link an existing BADA implementation target and enable the BADA-specific loader path. |
| `TAFT_FIM_SAMPLE_ALGORITHM_TARGET` | Link an existing FIM sample-algorithm target and enable that loader path. |

A parent CMake project must define a supplied target before adding TAFT. For a
custom provider, connect its construction through the aircraft builder or
[FrameworkAircraftLoader.cpp](https://github.com/mitre/taft/blob/main/AircraftDynamicsTestFramework/loaders/FrameworkAircraftLoader.cpp);
linking a target alone does not register an arbitrary implementation with the
command-line loader.

## Run a scenario

The [entry point](https://github.com/mitre/taft/blob/main/AircraftDynamicsTestFramework/taft.cpp)
accepts exactly one argument: either `--version` or a configuration-file path.

A configuration file starts with the number of scenarios, followed by the
scenario-file paths. For example:

```text
1
./test-framework-scenario.txt
```

Scenario files describe aircraft, guidance, weather inputs, and application
settings. Associated HFP, VFP, ENV, and speed-command data use CSV files. The
[Run_Files directory](https://github.com/mitre/taft/tree/main/Run_Files)
contains the inherited configuration, scenario template, and example CSV inputs.
The scenario template includes a machine-specific BADA path and an `{actype}`
placeholder: adapt these and check the scenario fields against the current
loader before attempting a provider-backed run.

Relative scenario paths are resolved from the process's working directory.
For the supplied configuration layout, run from `Run_Files` after preparing the
provider and scenario inputs:

```bash
cd Run_Files
../build/bin/TAFT ./test-framework-configuration.txt
```

Use `./` or an absolute path for the configuration argument. A bare filename
without a slash activates the inherited `../Run_Files/` lookup behavior.
CSV results are written in the working directory. A public build without an
appropriate provider cannot complete this simulation.

Logging is configured by `log4cplus.properties`; the repository and binary
packages include a starting configuration.

## Repository layout

| Location | Contents |
| --- | --- |
| `AircraftDynamicsTestFramework/` | Scenario execution, readers, loaders, writers, and the command-line entry point. |
| `include/framework/` | Framework headers and builder interfaces. |
| `unittest/` | GoogleTest sources and public test resources. |
| `Run_Files/` | Inherited scenario/configuration examples and CSV inputs. |
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
