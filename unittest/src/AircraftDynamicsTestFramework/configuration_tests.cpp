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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "framework/GuidanceDataLoader.h"
#include "framework/SpeedCommandsLoader.h"
#include "gtest/gtest.h"

namespace mitre::oss::taft::test {
namespace {
const std::string scenario_yaml = R"(aircraft:
  - ac_type: JET
    speed_management_type: pitch
    guidance:
      hfp_csv_file: data/path.csv
      vfp_csv_file: data/profile.csv
)";

std::string Replace(std::string text, const std::string &old, const std::string &replacement) {
   const auto pos = text.find(old);
   if (pos == std::string::npos) throw std::logic_error("Missing test replacement: " + old);
   return text.replace(pos, old.size(), replacement);
}

class WorkingDirectory {
  public:
   explicit WorkingDirectory(const std::filesystem::path &path) : original_(std::filesystem::current_path()) {
      std::filesystem::current_path(path);
   }
   ~WorkingDirectory() { std::filesystem::current_path(original_); }

  private:
   std::filesystem::path original_;
};
}

class ConfigurationTest : public ::testing::Test {
  protected:
   void SetUp() override {
      directory = std::filesystem::temp_directory_path() /
                  ("taft-yaml-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(directory);
   }
   void TearDown() override { std::filesystem::remove_all(directory); }
   std::filesystem::path Write(const std::string &name, const std::string &text) {
      const auto path = directory / name;
      std::filesystem::create_directories(path.parent_path());
      std::ofstream stream(path);
      stream << text;
      return path;
   }
   std::filesystem::path directory;
};

TEST_F(ConfigurationTest, AppliesDefaultsWithoutConstructingAircraft) {
   const auto path = Write("scenario.yaml", scenario_yaml);
   const auto config = LoadScenarioConfiguration(path);
   ASSERT_EQ(config.aircraft.size(), 1);
   const auto &aircraft = config.aircraft.front();
   EXPECT_EQ(aircraft.ac_type, "JET");
   EXPECT_EQ(aircraft.speed_management_type, "pitch");
   EXPECT_EQ(aircraft.initial_time_seconds, 0);
   EXPECT_DOUBLE_EQ(aircraft.initial_mass_fraction, 0.5);
   EXPECT_EQ(aircraft.env_data_index, "time");
   EXPECT_TRUE(aircraft.env_csv_file.empty());
   EXPECT_TRUE(aircraft.traffic_csv_file.empty());
   EXPECT_FALSE(aircraft.speed_commands.has_value());
   EXPECT_TRUE(aircraft.guidance.hfp_compute_xy);
   EXPECT_FALSE(aircraft.guidance.planned_cruise_mach.has_value());
   EXPECT_FALSE(aircraft.guidance.planned_transition_altitude_feet.has_value());
   EXPECT_TRUE(config.bada_data_path.empty());
   EXPECT_EQ(config.GetScenarioName(), "scenario");
}

TEST_F(ConfigurationTest, ResolvesNestedInputsFromTheirOwnYamlFiles) {
   const auto run_path = Write("runs/run.yaml", "scenarios: [../scenarios/first.yml, ../scenarios/second.yaml]\n");
   auto text = Replace(scenario_yaml, "    guidance:", R"(    initial_time_seconds: 60
    initial_mass_fraction: 0.7
    env_csv_file: ../weather.csv
    env_data_index: dtg
    traffic_csv_file: traffic.csv
    speed_commands:
      csv_file: speed.csv
      pilot_delay_seconds: 2.5
    guidance:)");
   text = Replace(text, "      vfp_csv_file: data/profile.csv", R"(      vfp_csv_file: data/profile.csv
      hfp_compute_xy: false
      planned_transition_altitude_feet: 28000
      planned_cruise_mach: 0.78)");
   Write("scenarios/first.yml", "bada_data_path: ../bada\n" + text);
   Write("scenarios/second.yaml", scenario_yaml);
   std::filesystem::create_directories(directory / "elsewhere");
   WorkingDirectory cwd(directory / "elsewhere");
   const auto run = LoadRunConfiguration(run_path);
   ASSERT_EQ(run.scenario_files.size(), 2);
   EXPECT_EQ(run.scenario_files[0], directory / "scenarios/first.yml");
   EXPECT_EQ(run.scenario_files[1], directory / "scenarios/second.yaml");
   const auto scenario = LoadScenarioConfiguration(run.scenario_files[0]);
   const auto &aircraft = scenario.aircraft.front();
   EXPECT_EQ(scenario.GetScenarioName(), "first");
   EXPECT_EQ(scenario.bada_data_path, directory / "bada");
   EXPECT_EQ(aircraft.env_csv_file, directory / "weather.csv");
   EXPECT_EQ(aircraft.traffic_csv_file, directory / "scenarios/traffic.csv");
   EXPECT_EQ(aircraft.guidance.hfp_csv_file, directory / "scenarios/data/path.csv");
   EXPECT_EQ(aircraft.guidance.vfp_csv_file, directory / "scenarios/data/profile.csv");
   EXPECT_FALSE(aircraft.guidance.hfp_compute_xy);
   EXPECT_DOUBLE_EQ(*aircraft.guidance.planned_cruise_mach, 0.78);
   EXPECT_DOUBLE_EQ(*aircraft.guidance.planned_transition_altitude_feet, 28000);
   ASSERT_TRUE(aircraft.speed_commands.has_value());
   EXPECT_EQ(aircraft.speed_commands->csv_file, directory / "scenarios/speed.csv");
   EXPECT_DOUBLE_EQ(aircraft.speed_commands->pilot_delay_seconds, 2.5);
   EXPECT_EQ(aircraft.initial_time_seconds, 60);
   EXPECT_DOUBLE_EQ(aircraft.initial_mass_fraction, 0.7);
   EXPECT_EQ(aircraft.env_data_index, "dtg");
}

TEST_F(ConfigurationTest, AcceptsAbsolutePathsAndBareRunFilename) {
   const auto scenario_path = Write("scenario.yaml", scenario_yaml);
   Write("run.yaml", "scenarios: ['" + scenario_path.string() + "']\n");
   WorkingDirectory cwd(directory);
   const auto run = LoadRunConfiguration("run.yaml");
   ASSERT_EQ(run.scenario_files.size(), 1);
   EXPECT_EQ(run.scenario_files.front(), scenario_path);
}

TEST_F(ConfigurationTest, AcceptsMultipleAircraftAndWaypointGuidance) {
   auto text = scenario_yaml + scenario_yaml.substr(scenario_yaml.find("  - ac_type"));
   text = Replace(text, "hfp_csv_file: data/path.csv", "waypoint_sequence_csv_file: waypoints.csv");
   const auto config = LoadScenarioConfiguration(Write("multiple.yaml", text));
   ASSERT_EQ(config.aircraft.size(), 2);
   EXPECT_TRUE(config.aircraft[0].guidance.hfp_csv_file.empty());
   EXPECT_EQ(config.aircraft[0].guidance.waypoint_sequence_csv_file, directory / "waypoints.csv");
   EXPECT_FALSE(config.aircraft[1].guidance.hfp_csv_file.empty());
}

TEST_F(ConfigurationTest, DefaultsReplayDelayToZero) {
   const auto text =
         Replace(scenario_yaml, "    guidance:", "    speed_commands: {csv_file: speeds.csv}\n    guidance:");
   const auto config = LoadScenarioConfiguration(Write("scenario.yaml", text));
   ASSERT_TRUE(config.aircraft.front().speed_commands.has_value());
   EXPECT_DOUBLE_EQ(config.aircraft.front().speed_commands->pilot_delay_seconds, 0);
}

TEST_F(ConfigurationTest, LoadsShippedExamplesWithoutPerformanceData) {
   const auto run =
         LoadRunConfiguration(std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files/test-framework-configuration.yaml");
   ASSERT_FALSE(run.scenario_files.empty());
   for (const auto &filename : run.scenario_files) {
      const auto config = LoadScenarioConfiguration(filename);
      ASSERT_FALSE(config.aircraft.empty());
      for (const auto &aircraft : config.aircraft) {
         GuidanceDataLoader guidance(aircraft.guidance);
         const auto calculator = guidance.BuildGuidanceCalculator();
         EXPECT_FALSE(calculator->GetHorizontalTrajectory().empty());
         EXPECT_FALSE(calculator->GetVerticalData().m_ias_mps.empty());
         if (aircraft.speed_commands)
            EXPECT_TRUE(loader::SpeedCommandsLoader(*aircraft.speed_commands).Build().IsActive());
      }
   }
}

TEST_F(ConfigurationTest, ReportsMissingFile) {
   const auto filename = directory / "missing.yaml";
   try {
      LoadRunConfiguration(filename);
      FAIL() << "Expected missing-file error";
   } catch (const std::runtime_error &error) {
      EXPECT_NE(std::string(error.what()).find(filename.string()), std::string::npos);
   }
}

class InvalidScenarioTest : public ConfigurationTest,
                            public ::testing::WithParamInterface<std::pair<std::string, std::string>> {};

TEST_P(InvalidScenarioTest, RejectsInputWithFileAndFieldContext) {
   const auto filename = Write("invalid.yaml", GetParam().first);
   try {
      LoadScenarioConfiguration(filename);
      FAIL() << "Expected invalid configuration";
   } catch (const std::runtime_error &error) {
      EXPECT_NE(std::string(error.what()).find(filename.string()), std::string::npos);
      EXPECT_NE(std::string(error.what()).find(GetParam().second), std::string::npos) << error.what();
   }
}

INSTANTIATE_TEST_SUITE_P(
      YamlValidation, InvalidScenarioTest,
      ::testing::Values(
            std::make_pair("", "$"), std::make_pair("aircraft: [", "$"), std::make_pair("aircraft: []", "aircraft"),
            std::make_pair("{}", "aircraft"), std::make_pair("aircraft: wrong", "aircraft"),
            std::make_pair("aircraft: [null]", "aircraft[0]"), std::make_pair("aircraft: [{}]", "ac_type"),
            std::make_pair("aircraft: [{ac_type: JET, speed_management_type: pitch}]", "guidance"),
            std::make_pair(Replace(scenario_yaml, "JET", "''"), "ac_type"),
            std::make_pair(Replace(scenario_yaml, "    speed_management_type: pitch\n", ""), "speed_management_type"),
            std::make_pair(Replace(scenario_yaml, "pitch", "other"), "speed_management_type"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    initial_time_seconds: -1\n    guidance:"),
                           "initial_time_seconds"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    initial_time_seconds: 1.5\n    guidance:"),
                           "initial_time_seconds"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    initial_mass_fraction: 1.1\n    guidance:"),
                           "initial_mass_fraction"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    initial_mass_fraction: .nan\n    guidance:"),
                           "initial_mass_fraction"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    env_data_index: other\n    guidance:"),
                           "env_data_index"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    traffic_csv_file: ''\n    guidance:"),
                           "traffic_csv_file"),
            std::make_pair(Replace(scenario_yaml,
                                   "    guidance:", "    im_speed_commands_from_file: {}\n    guidance:"),
                           "im_speed_commands_from_file"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    guidance: {}\n    unused:"), "unused"),
            std::make_pair(Replace(scenario_yaml, "      hfp_csv_file: data/path.csv\n", ""), "guidance"),
            std::make_pair(Replace(scenario_yaml, "      vfp_csv_file: data/profile.csv\n", ""), "vfp_csv_file"),
            std::make_pair(scenario_yaml + "      waypoint_sequence_csv_file: points.csv\n", "guidance"),
            std::make_pair(scenario_yaml + "      hfp_compute_xy: other\n", "hfp_compute_xy"),
            std::make_pair(scenario_yaml + "      planned_cruise_mach: 0.78\n", "guidance"),
            std::make_pair(scenario_yaml + "      planned_transition_altitude_feet: 28000\n", "guidance"),
            std::make_pair(scenario_yaml +
                                 "      planned_transition_altitude_feet: 28000\n      planned_cruise_mach: 1.1\n",
                           "planned_cruise_mach"),
            std::make_pair(scenario_yaml +
                                 "      planned_transition_altitude_feet: .inf\n      planned_cruise_mach: 0.78\n",
                           "planned_transition_altitude_feet"),
            std::make_pair(scenario_yaml + "      hfp_csv_file: duplicate.csv\n", "hfp_csv_file"),
            std::make_pair(Replace(scenario_yaml, "    guidance:", "    speed_commands: {}\n    guidance:"),
                           "csv_file"),
            std::make_pair(
                  Replace(scenario_yaml, "    guidance:",
                          "    speed_commands: {csv_file: speeds.csv, pilot_delay_seconds: -1}\n    guidance:"),
                  "pilot_delay_seconds")));

class InvalidRunTest : public ConfigurationTest, public ::testing::WithParamInterface<std::string> {};
TEST_P(InvalidRunTest, RejectsMalformedManifest) {
   const auto filename = Write("run.yaml", GetParam());
   EXPECT_THROW(LoadRunConfiguration(filename), std::runtime_error);
}
INSTANTIATE_TEST_SUITE_P(YamlValidation, InvalidRunTest,
                         ::testing::Values("", "1\nscenario.txt\n", "{}", "scenarios: []", "scenarios: wrong",
                                           "scenarios: [null]", "scenarios: ['']",
                                           "scenarios: [one.yaml]\nunknown: true",
                                           "scenarios: [one.yaml]\nscenarios: [two.yaml]"));
}
