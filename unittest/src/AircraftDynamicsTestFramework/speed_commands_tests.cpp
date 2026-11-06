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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "framework/ApplicationLoader.h"
#include "gtest/gtest.h"

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
   const std::vector<std::vector<SpeedCommandsFromStaticData::SpeedRecord>> invalid = {
         {},
         {Record(0, 100)},
         {Record(0, 100), Record(0, 120)},
         {Record(1, 100), Record(0, 120)},
         {Record(nan, 100), Record(1, 120)},
         {Record(0, 100), Record(inf, 120)},
         {Record(0, -1), Record(1, 120)},
         {Record(0, 0), Record(1, 120)},
         {Record(0, 100), Record(1, nan)},
         {Record(0, inf), Record(1, 120)}};
   for (const auto &records : invalid)
      EXPECT_THROW(SpeedCommandsFromStaticData(records, Units::SecondsTime(0)), std::invalid_argument);
   for (double delay : {-1.0, nan, inf})
      EXPECT_THROW((SpeedCommandsFromStaticData({Record(0, 100), Record(1, 120)}, Units::SecondsTime(delay))),
                   std::invalid_argument);
}

class SpeedFileTest : public ::testing::Test {
  protected:
   void SetUp() override {
      filename = std::filesystem::temp_directory_path() /
                 ("taft-speed-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".csv");
   }
   void TearDown() override { std::filesystem::remove(filename); }
   void Write(const std::string &text) { std::ofstream(filename) << text; }
   std::shared_ptr<mitre::oss::simcore::FlightDeckApplication> Load(double delay = 0) {
      return ApplicationLoader(SpeedCommandsConfiguration{filename, delay}).CreateApplication();
   }
   std::filesystem::path filename;
};

TEST_F(SpeedFileTest, CreatesReplayAndOverridesMachSelection) {
   Write("Time(sec),Vias_c(m/s)\n10,100\n20,120");
   const auto application = Load(2);
   ASSERT_TRUE(application->IsActive());
   const auto command = Update(*application, 17);
   EXPECT_TRUE(command.IsValid());
   EXPECT_EQ(command.GetSelectedSpeedType(), SpeedValueType::INDICATED_AIR_SPEED);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(command.m_ias_command).value(), 110, 1e-9);
}

TEST_F(SpeedFileTest, ReportsMissingCsv) {
   try {
      Load();
      FAIL() << "Expected missing-file error";
   } catch (const std::runtime_error &error) {
      EXPECT_NE(std::string(error.what()).find(filename.string()), std::string::npos);
   }
}

TEST_F(SpeedFileTest, RejectsMalformedAndInsufficientCsvData) {
   for (const auto &contents : {"", "time,speed\n", "time,speed\n0,100\n", "time,speed\n0,bad\n1,120\n",
                                "time,speed\n0,100,extra\n1,120\n", "time,speed\n0,100\n0,120\n"}) {
      Write(contents);
      try {
         Load();
         FAIL() << "Expected invalid CSV error";
      } catch (const std::runtime_error &error) {
         EXPECT_NE(std::string(error.what()).find(filename.string()), std::string::npos);
      }
   }
}
}
