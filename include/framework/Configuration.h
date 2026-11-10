// ****************************************************************************
// NOTICE
//
// This is the copyright work of The MITRE Corporation, and was produced for the U. S. Government under
// Contract Number 693KA8-22-C-00001, and is subject to Federal Aviation Administration Acquisition
// Management System Clause 3.5-13, Rights In Data-General (Oct. 2014), Alt. III and Alt. IV (Jan.
// 2009).

// No other use other than that granted to the U. S. Government, or to those acting on behalf of
// the U. S. Government, under that Clause is authorized without the express written permission of The
// MITRE Corporation.

// For further information, please contact The MITRE Corporation, Contracts
// Management Office, 7515 Colshire Drive, McLean, VA 22102-7539, (703) 983-6000.
//
// (c) 2026 The MITRE Corporation. All Rights Reserved.
// ****************************************************************************

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mitre::oss::taft {
struct RunConfiguration {
   std::vector<std::filesystem::path> scenario_files;
};

struct GuidanceConfiguration {
   std::filesystem::path vfp_csv_file;
   std::filesystem::path hfp_csv_file;
   std::filesystem::path waypoint_sequence_csv_file;
   bool hfp_compute_xy{true};
   std::optional<double> planned_transition_altitude_feet;
   std::optional<double> planned_cruise_mach;
};

struct SpeedCommandsConfiguration {
   std::filesystem::path csv_file;
   double pilot_delay_seconds{0};
};

struct AircraftConfiguration {
   std::string ac_type;
   std::string speed_management_type;
   int initial_time_seconds{0};
   double initial_mass_fraction{0.5};
   std::filesystem::path env_csv_file;
   std::string env_data_index{"time"};
   std::filesystem::path traffic_csv_file;
   GuidanceConfiguration guidance;
   std::optional<SpeedCommandsConfiguration> speed_commands;
};

struct ScenarioConfiguration {
   std::filesystem::path source_file;
   std::filesystem::path bada_data_path;
   std::vector<AircraftConfiguration> aircraft;

   std::string GetScenarioName() const { return source_file.stem().string(); }
};

std::vector<ScenarioConfiguration> LoadRunScenarios(const std::filesystem::path &filename);
RunConfiguration LoadRunConfiguration(const std::filesystem::path &filename);
ScenarioConfiguration LoadScenarioConfiguration(const std::filesystem::path &filename);
}
