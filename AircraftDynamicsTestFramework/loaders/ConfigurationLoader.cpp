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

#include "framework/Configuration.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <set>
#include <stdexcept>

namespace mitre::oss::taft {
namespace {
class ConfigurationReader {
  public:
   explicit ConfigurationReader(const std::filesystem::path &filename)
      : filename_(std::filesystem::absolute(filename).lexically_normal()) {}

   YAML::Node Load() const {
      try {
         return YAML::LoadFile(filename_.string());
      } catch (const YAML::Exception &error) {
         Fail("$", error.what());
      }
   }

   [[noreturn]] void Fail(const std::string &field, const std::string &message) const {
      throw std::runtime_error(filename_.string() + ": " + field + ": " + message);
   }

   void Map(const YAML::Node &node, const std::string &field, std::initializer_list<std::string> allowed) const {
      if (!node || !node.IsMap()) Fail(field, "expected a mapping");
      std::set<std::string> seen;
      for (const auto &entry : node) {
         const auto key = Scalar<std::string>(entry.first, field);
         if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) Fail(field + "." + key, "unknown field");
         if (!seen.insert(key).second) Fail(field + "." + key, "duplicate field");
      }
   }

   void Sequence(const YAML::Node &node, const std::string &field) const {
      if (!node || !node.IsSequence() || node.size() == 0) Fail(field, "expected a nonempty sequence");
   }

   template <typename T>
   T Scalar(const YAML::Node &node, const std::string &field) const {
      if (!node || !node.IsScalar()) Fail(field, "expected a scalar value");
      try {
         return node.as<T>();
      } catch (const YAML::Exception &error) {
         Fail(field, error.what());
      }
   }

   template <typename T>
   T Optional(const YAML::Node &node, const std::string &key, const std::string &field, T fallback) const {
      return node[key] ? Scalar<T>(node[key], field + "." + key) : fallback;
   }

   std::string String(const YAML::Node &node, const std::string &field) const {
      auto value = Scalar<std::string>(node, field);
      if (value.empty()) Fail(field, "must not be empty");
      return value;
   }

   std::filesystem::path Path(const YAML::Node &node, const std::string &field) const {
      std::filesystem::path path(String(node, field));
      return (path.is_absolute() ? path : filename_.parent_path() / path).lexically_normal();
   }

   std::filesystem::path OptionalPath(const YAML::Node &node, const std::string &key, const std::string &field) const {
      return node[key] ? Path(node[key], field + "." + key) : std::filesystem::path{};
   }

   void Finite(double value, const std::string &field) const {
      if (!std::isfinite(value)) Fail(field, "must be finite");
   }

   GuidanceConfiguration Guidance(const YAML::Node &node, const std::string &field) const {
      Map(node, field,
          {"vfp_csv_file", "hfp_csv_file", "waypoint_sequence_csv_file", "hfp_compute_xy",
           "planned_transition_altitude_feet", "planned_cruise_mach"});
      GuidanceConfiguration config;
      config.vfp_csv_file = Path(node["vfp_csv_file"], field + ".vfp_csv_file");
      config.hfp_csv_file = OptionalPath(node, "hfp_csv_file", field);
      config.waypoint_sequence_csv_file = OptionalPath(node, "waypoint_sequence_csv_file", field);
      if (config.hfp_csv_file.empty() == config.waypoint_sequence_csv_file.empty())
         Fail(field, "provide exactly one of hfp_csv_file or waypoint_sequence_csv_file");
      config.hfp_compute_xy = Optional<bool>(node, "hfp_compute_xy", field, true);
      if (static_cast<bool>(node["planned_transition_altitude_feet"]) != static_cast<bool>(node["planned_cruise_mach"]))
         Fail(field, "planned_transition_altitude_feet and planned_cruise_mach must be specified together");
      if (node["planned_cruise_mach"]) {
         const double mach = Scalar<double>(node["planned_cruise_mach"], field + ".planned_cruise_mach");
         const double altitude =
               Scalar<double>(node["planned_transition_altitude_feet"], field + ".planned_transition_altitude_feet");
         Finite(mach, field + ".planned_cruise_mach");
         Finite(altitude, field + ".planned_transition_altitude_feet");
         if (mach < 0 || mach > 1) Fail(field + ".planned_cruise_mach", "must be between 0 and 1");
         config.planned_cruise_mach = mach;
         config.planned_transition_altitude_feet = altitude;
      }
      return config;
   }

   AircraftConfiguration Aircraft(const YAML::Node &node, const std::string &field) const {
      Map(node, field,
          {"ac_type", "speed_management_type", "initial_time_seconds", "initial_mass_fraction", "env_csv_file",
           "env_data_index", "traffic_csv_file", "guidance", "speed_commands"});
      AircraftConfiguration config;
      config.ac_type = String(node["ac_type"], field + ".ac_type");
      config.speed_management_type = String(node["speed_management_type"], field + ".speed_management_type");
      if (config.speed_management_type != "pitch" && config.speed_management_type != "thrust")
         Fail(field + ".speed_management_type", "expected pitch or thrust");
      config.initial_time_seconds = Optional<int>(node, "initial_time_seconds", field, 0);
      if (config.initial_time_seconds < 0) Fail(field + ".initial_time_seconds", "must not be negative");
      config.initial_mass_fraction = Optional<double>(node, "initial_mass_fraction", field, 0.5);
      Finite(config.initial_mass_fraction, field + ".initial_mass_fraction");
      if (config.initial_mass_fraction < 0 || config.initial_mass_fraction > 1)
         Fail(field + ".initial_mass_fraction", "must be between 0 and 1");
      config.env_csv_file = OptionalPath(node, "env_csv_file", field);
      config.env_data_index = Optional<std::string>(node, "env_data_index", field, "time");
      if (config.env_data_index != "time" && config.env_data_index != "dtg")
         Fail(field + ".env_data_index", "expected time or dtg");
      config.traffic_csv_file = OptionalPath(node, "traffic_csv_file", field);
      config.guidance = Guidance(node["guidance"], field + ".guidance");
      if (node["speed_commands"]) {
         const auto speed = node["speed_commands"];
         const auto speed_field = field + ".speed_commands";
         Map(speed, speed_field, {"csv_file", "pilot_delay_seconds"});
         SpeedCommandsConfiguration commands;
         commands.csv_file = Path(speed["csv_file"], speed_field + ".csv_file");
         commands.pilot_delay_seconds = Optional<double>(speed, "pilot_delay_seconds", speed_field, 0);
         Finite(commands.pilot_delay_seconds, speed_field + ".pilot_delay_seconds");
         if (commands.pilot_delay_seconds < 0) Fail(speed_field + ".pilot_delay_seconds", "must not be negative");
         config.speed_commands = commands;
      }
      return config;
   }

   const std::filesystem::path &Filename() const { return filename_; }

  private:
   std::filesystem::path filename_;
};
}

RunConfiguration LoadRunConfiguration(const std::filesystem::path &filename) {
   ConfigurationReader reader(filename);
   const auto root = reader.Load();
   reader.Map(root, "$", {"scenarios"});
   reader.Sequence(root["scenarios"], "scenarios");
   RunConfiguration config;
   for (std::size_t i = 0; i < root["scenarios"].size(); ++i)
      config.scenario_files.push_back(reader.Path(root["scenarios"][i], "scenarios[" + std::to_string(i) + "]"));
   return config;
}

ScenarioConfiguration LoadScenarioConfiguration(const std::filesystem::path &filename) {
   ConfigurationReader reader(filename);
   const auto root = reader.Load();
   reader.Map(root, "$", {"bada_data_path", "aircraft"});
   reader.Sequence(root["aircraft"], "aircraft");
   ScenarioConfiguration config;
   config.source_file = reader.Filename();
   config.bada_data_path = reader.OptionalPath(root, "bada_data_path", "$");
   for (std::size_t i = 0; i < root["aircraft"].size(); ++i)
      config.aircraft.push_back(reader.Aircraft(root["aircraft"][i], "aircraft[" + std::to_string(i) + "]"));
   return config;
}
}
