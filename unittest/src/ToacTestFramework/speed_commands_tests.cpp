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

#include "framework/SpeedCommandsFromStaticData.h"

#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "framework/ApplicationLoader.h"
#include "framework/Configuration.h"
#include "framework/SpeedCommandsLoader.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
namespace {
SpeedCommandsFromStaticData::SpeedRecord Record(double time, double speed) {
   SpeedCommandsFromStaticData::SpeedRecord record;
   record.simtime = Units::SecondsTime(time);
   record.speed_command = Units::MetersPerSecondSpeed(speed);
   return record;
}

mitre::oss::simcore::Guidance Update(mitre::oss::simcore::FlightDeckApplication &application, double time) {
   mitre::oss::simcore::Guidance planned;
   planned.SetValid(true);
   planned.SetSelectedSpeedType(SpeedValueType::MACH_SPEED);
   planned.SetMachCommand(0.78);
   return application.Update(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(time)), planned,
                             mitre::oss::simcore::DynamicsState{}, mitre::oss::simcore::AircraftState{});
}
}

TEST(SpeedReplayTest, InterpolatesAndIncludesBothEndpoints) {
   SpeedCommandsFromStaticData replay({Record(10, 100), Record(20, 120), Record(30, 160)}, Units::SecondsTime(0));
   for (const auto &sample :
        std::vector<std::pair<double, double>>{{10, 100}, {15, 110}, {20, 120}, {25, 140}, {30, 160}}) {
      SCOPED_TRACE(sample.first);
      const auto command = Update(replay, sample.first);
      EXPECT_TRUE(command.IsValid());
      EXPECT_EQ(command.GetSelectedSpeedType(), SpeedValueType::INDICATED_AIR_SPEED);
      EXPECT_NEAR(Units::MetersPerSecondSpeed(command.m_ias_command).value(), sample.second, 1e-9);
   }
   EXPECT_FALSE(Update(replay, 9).IsValid());
   EXPECT_FALSE(Update(replay, 31).IsValid());
}

TEST(SpeedReplayTest, UsesDelayedTimeForLookupAndInterpolation) {
   SpeedCommandsFromStaticData replay({Record(10, 100), Record(20, 120), Record(30, 160)}, Units::SecondsTime(2));
   EXPECT_FALSE(Update(replay, 11).IsValid());
   EXPECT_NEAR(Units::MetersPerSecondSpeed(Update(replay, 12).m_ias_command).value(), 100, 1e-9);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(Update(replay, 17).m_ias_command).value(), 110, 1e-9);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(Update(replay, 22).m_ias_command).value(), 120, 1e-9);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(Update(replay, 32).m_ias_command).value(), 160, 1e-9);
   EXPECT_FALSE(Update(replay, 33).IsValid());
}

TEST(SpeedReplayTest, DefaultAndAbsentApplicationsAreInactive) {
   SpeedCommandsFromStaticData empty;
   EXPECT_FALSE(empty.IsActive());
   EXPECT_FALSE(Update(empty, 0).IsValid());
   const auto null_application = ApplicationLoader(std::nullopt).CreateApplication();
   EXPECT_FALSE(null_application->IsActive());
   EXPECT_FALSE(Update(*null_application, 0).IsValid());
}

TEST(SpeedReplayTest, RejectsInvalidSamplesAndDelay) {
   const double nan = std::numeric_limits<double>::quiet_NaN();
   const double inf = std::numeric_limits<double>::infinity();
   const std::vector<std::pair<const char *, std::vector<SpeedCommandsFromStaticData::SpeedRecord>>> invalid = {
         {"empty", {}},
         {"single sample", {Record(0, 100)}},
         {"duplicate times", {Record(0, 100), Record(0, 120)}},
         {"decreasing times", {Record(1, 100), Record(0, 120)}},
         {"NaN time", {Record(nan, 100), Record(1, 120)}},
         {"infinite time", {Record(0, 100), Record(inf, 120)}},
         {"negative speed", {Record(0, -1), Record(1, 120)}},
         {"zero speed", {Record(0, 0), Record(1, 120)}},
         {"NaN speed", {Record(0, 100), Record(1, nan)}},
         {"infinite speed", {Record(0, inf), Record(1, 120)}}};
   for (const auto &[label, records] : invalid) {
      SCOPED_TRACE(label);
      EXPECT_THROW(SpeedCommandsFromStaticData(records, Units::SecondsTime(0)), std::invalid_argument);
   }
   for (double delay : {-1.0, nan, inf}) {
      SCOPED_TRACE(delay);
      EXPECT_THROW((SpeedCommandsFromStaticData({Record(0, 100), Record(1, 120)}, Units::SecondsTime(delay))),
                   std::invalid_argument);
   }
}

class SpeedCommandsLoaderTest : public TemporaryDirectoryTest {
  protected:
   std::shared_ptr<mitre::oss::simcore::FlightDeckApplication> Load(double delay = 0) {
      return ApplicationLoader(SpeedCommandsConfiguration{directory / "speeds.csv", delay}).CreateApplication();
   }
};

TEST_F(SpeedCommandsLoaderTest, CreatesReplayAndOverridesMachSelection) {
   Write("speeds.csv", "Time(sec),Vias_c(m/s)\n10,100\n20,120");
   const auto application = Load(2);
   ASSERT_TRUE(application->IsActive());
   const auto command = Update(*application, 17);
   EXPECT_TRUE(command.IsValid());
   EXPECT_EQ(command.GetSelectedSpeedType(), SpeedValueType::INDICATED_AIR_SPEED);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(command.m_ias_command).value(), 110, 1e-9);
}

TEST_F(SpeedCommandsLoaderTest, ReportsMissingCsv) {
   try {
      Load();
      FAIL() << "Expected missing-file error";
   } catch (const std::runtime_error &error) {
      EXPECT_NE(std::string(error.what()).find((directory / "speeds.csv").string()), std::string::npos);
   }
}

TEST_F(SpeedCommandsLoaderTest, RejectsMalformedAndInsufficientCsvData) {
   for (const auto &contents : {"", "time,speed\n", "time,speed\n0,100\n", "time,speed\n0,bad\n1,120\n",
                                "time,speed\n0,100,extra\n1,120\n", "time,speed\n0,100\n0,120\n"}) {
      SCOPED_TRACE(contents);
      Write("speeds.csv", contents);
      try {
         Load();
         FAIL() << "Expected invalid CSV error";
      } catch (const std::runtime_error &error) {
         EXPECT_NE(std::string(error.what()).find((directory / "speeds.csv").string()), std::string::npos);
      }
   }
}

TEST_F(SpeedCommandsLoaderTest, LoadsShippedSpeedCommands) {
   const auto run =
         LoadRunConfiguration(std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files/test-framework-configuration.yaml");
   for (const auto &filename : run.scenario_files) {
      SCOPED_TRACE(filename.string());
      const auto configuration = LoadScenarioConfiguration(filename);
      for (const auto &aircraft : configuration.aircraft) {
         if (aircraft.speed_commands)
            EXPECT_TRUE(loader::SpeedCommandsLoader(*aircraft.speed_commands).Build().IsActive());
      }
   }
}
}
