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

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "test_support.h"

namespace mitre::oss::taft::test {
namespace {
const std::string scenario_yaml = R"(aircraft:
  - ac_type: JET
    speed_management_type: pitch
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

class ConfigurationTest : public TemporaryDirectoryTest {};

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
   EXPECT_TRUE(config.bada_data_path.empty());
   EXPECT_EQ(config.GetScenarioName(), "scenario");
}

TEST_F(ConfigurationTest, ResolvesNestedInputsFromTheirOwnYamlFiles) {
   const auto run_path = Write("runs/run.yaml", "scenarios: [../scenarios/first.yml, ../scenarios/second.yaml]\n");
   const auto text = scenario_yaml + R"(    initial_time_seconds: 60
    initial_mass_fraction: 0.7
    env_csv_file: ../weather.csv
    env_data_index: dtg
)";
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

TEST_F(ConfigurationTest, AcceptsMultipleAircraft) {
   const auto text = scenario_yaml + "  - ac_type: OTHER\n    speed_management_type: thrust\n";
   const auto config = LoadScenarioConfiguration(Write("multiple.yaml", text));
   ASSERT_EQ(config.aircraft.size(), 2);
   EXPECT_EQ(config.aircraft[0].ac_type, "JET");
   EXPECT_EQ(config.aircraft[1].ac_type, "OTHER");
   EXPECT_EQ(config.aircraft[1].speed_management_type, "thrust");
}

TEST_F(ConfigurationTest, ParsesShippedExamplesWithoutConstructingModels) {
   const auto example_directory = std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files";
   const auto scenarios = LoadRunScenarios(example_directory / "test-framework-configuration.yaml");
   ASSERT_EQ(scenarios.size(), 1);
   ASSERT_EQ(scenarios.front().aircraft.size(), 1);
   EXPECT_EQ(scenarios.front().aircraft.front().env_csv_file, example_directory / "example-weather.csv");
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

TEST_F(ConfigurationTest, RejectsCollidingOutputNamesBeforeSimulation) {
   const std::string scenario = scenario_yaml;
   std::filesystem::create_directories(directory / "a");
   std::filesystem::create_directories(directory / "b");
   Write("a/scenario.yaml", scenario);
   Write("b/scenario.yaml", scenario);
   const auto manifest = Write("run.yaml", "scenarios: [a/scenario.yaml, b/scenario.yaml]\n");
   ASSERT_NO_THROW(LoadScenarioConfiguration(directory / "a/scenario.yaml"));
   try {
      LoadRunScenarios(manifest);
      FAIL() << "Expected duplicate output-name error";
   } catch (const std::runtime_error &error) {
      EXPECT_NE(std::string(error.what()).find("duplicate output name"), std::string::npos) << error.what();
   }
}

class InvalidScenarioTest : public ConfigurationTest,
                            public ::testing::WithParamInterface<std::pair<std::string, std::string>> {};

TEST_P(InvalidScenarioTest, RejectsInputWithFileAndFieldContext) {
   SCOPED_TRACE(GetParam().first);
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
            std::make_pair(Replace(scenario_yaml, "JET", "''"), "ac_type"),
            std::make_pair(Replace(scenario_yaml, "    speed_management_type: pitch\n", ""), "speed_management_type"),
            std::make_pair(Replace(scenario_yaml, "pitch", "other"), "speed_management_type"),
            std::make_pair(scenario_yaml + "    initial_time_seconds: -1\n", "initial_time_seconds"),
            std::make_pair(scenario_yaml + "    initial_time_seconds: 1.5\n", "initial_time_seconds"),
            std::make_pair(scenario_yaml + "    initial_mass_fraction: 1.1\n", "initial_mass_fraction"),
            std::make_pair(scenario_yaml + "    initial_mass_fraction: .nan\n", "initial_mass_fraction"),
            std::make_pair(scenario_yaml + "    env_data_index: other\n", "env_data_index"),
            std::make_pair(scenario_yaml + "    guidance: {}\n", "aircraft[0].guidance: unknown field"),
            std::make_pair(scenario_yaml + "    speed_commands: {csv_file: speeds.csv}\n",
                           "aircraft[0].speed_commands: unknown field"),
            std::make_pair(scenario_yaml + "    traffic_csv_file: traffic.csv\n",
                           "aircraft[0].traffic_csv_file: unknown field"),
            std::make_pair(scenario_yaml + "    im_speed_commands_from_file: {}\n", "im_speed_commands_from_file"),
            std::make_pair(scenario_yaml + "    unused: {}\n", "unused"),
            std::make_pair(scenario_yaml + "    ac_type: DUPLICATE\n", "aircraft[0].ac_type: duplicate field")));

class InvalidRunTest : public ConfigurationTest, public ::testing::WithParamInterface<std::string> {};
TEST_P(InvalidRunTest, RejectsMalformedManifest) {
   SCOPED_TRACE(GetParam());
   const auto filename = Write("run.yaml", GetParam());
   EXPECT_THROW(LoadRunConfiguration(filename), std::runtime_error);
}
INSTANTIATE_TEST_SUITE_P(YamlValidation, InvalidRunTest,
                         ::testing::Values("", "1\nscenario.txt\n", "{}", "scenarios: []", "scenarios: wrong",
                                           "scenarios: [null]", "scenarios: ['']",
                                           "scenarios: [one.yaml]\nunknown: true",
                                           "scenarios: [one.yaml]\nscenarios: [two.yaml]"));
}
