# Test Automation Framework for TOAC (TAFT)

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
**Important: This work is in progress and is NOT ready-for-use today.**
TAFT is a C++ framework for running aircraft-dynamics scenarios and collecting
CSV results for TOAC and avionics research. It was initialized from a source
copy of [FMACM](https://github.com/mitre/FMACM).

The public source supports building and unit testing without licensed aircraft
performance data. Running a simulation requires an external aircraft-performance
provider and its data.

## Documentation

Read the [TAFT guide](https://mitre.github.io/taft/) for prerequisites, building,
scenario inputs, provider integration, CI, and releases. The same guide is
available as [repository Markdown](docs/README.md).

Download published packages from [GitHub Releases](https://github.com/mitre/taft/releases).

## License and contact

TAFT is distributed under the [Apache License 2.0](LICENSE).
If you use this software in a deployment or embed it in another project, MITRE
requests that you notify [opensource@mitre.org](mailto:opensource@mitre.org).
