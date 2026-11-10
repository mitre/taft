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

#include <filesystem>
#include <stdexcept>
#include <string>

#include "framework/WeatherTruthFromStaticData.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
class WeatherTruthTest : public TemporaryDirectoryTest {
  protected:
   const std::string weather_header = "time,dtg,wx,wy,dwx,dwy,temperature\n";
};

TEST_F(WeatherTruthTest, SelectsSampleBySimulationTime) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData();
   test_weather.Initialize((std::filesystem::path(TAFT_SOURCE_DIR) / "unittest/resources/test_env_file.csv").string(),
                           Units::zero(), WeatherTruthFromStaticData::DataIndexParameter::SIMULATION_TIME);
   test_weather.Update(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(1.0)), Units::zero(), Units::zero());
   Units::KelvinTemperature actual_temp = test_weather.GetTemperature();
   const Units::KelvinTemperature expected_temp(226);
   EXPECT_NEAR(actual_temp.value(), expected_temp.value(), 1e-10);
}

TEST_F(WeatherTruthTest, SelectsSampleByDistanceToGo) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData();
   test_weather.Initialize((std::filesystem::path(TAFT_SOURCE_DIR) / "unittest/resources/test_env_file.csv").string(),
                           Units::zero(), WeatherTruthFromStaticData::DataIndexParameter::DISTANCE_TO_GO);
   test_weather.Update(mitre::oss::simcore::SimulationTime::Of(Units::ZERO_TIME), Units::MetersLength(900.0),
                       Units::zero());
   Units::KelvinTemperature actual_temp = test_weather.GetTemperature();
   const Units::KelvinTemperature expected_temp(226);
   EXPECT_NEAR(actual_temp.value(), expected_temp.value(), 1e-10);
}

TEST_F(WeatherTruthTest, InitializesZeroWind) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData::CreateZeroTruthWind();
   test_weather.Update(mitre::oss::simcore::SimulationTime::Of(Units::ZERO_TIME), Units::zero(), Units::zero());
   Units::KelvinTemperature expected_temp(288.15);
   EXPECT_EQ(test_weather.GetTemperature().value(), expected_temp.value());
}

TEST_F(WeatherTruthTest, ReloadRemovesPreviousDistanceSamples) {
   WeatherTruthFromStaticData weather;
   const auto first =
         Write("weather-a.csv", weather_header + "0,0,0,0,0,0,250\n1,500,0,0,0,0,255\n2,1000,0,0,0,0,260\n");
   const auto second = Write("weather-b.csv", weather_header + "0,0,0,0,0,0,270\n1,1000,0,0,0,0,280\n");
   weather.Initialize(first.string(), Units::MetersLength(0), WeatherTruthFromStaticData::DISTANCE_TO_GO);
   weather.Initialize(second.string(), Units::MetersLength(0), WeatherTruthFromStaticData::DISTANCE_TO_GO);
   weather.Update(mitre::oss::simcore::SimulationTime(), Units::MetersLength(400), Units::MetersLength(0));
   EXPECT_DOUBLE_EQ(weather.GetTemperature().value(), 280);
}

TEST_F(WeatherTruthTest, RejectsMissingFile) {
   WeatherTruthFromStaticData weather;
   EXPECT_THROW(weather.Initialize((directory / "missing.csv").string(), Units::MetersLength(0),
                                   WeatherTruthFromStaticData::SIMULATION_TIME),
                std::runtime_error);
}

TEST_F(WeatherTruthTest, RejectsHeaderOnlyAndMalformedData) {
   for (const auto &data : {weather_header, weather_header + "0,0,bad,0,0,0,280\n",
                            weather_header + "0,0,0,0,0,0,nan\n", weather_header + "0,0,0,0,0,0,0\n"}) {
      SCOPED_TRACE(data);
      WeatherTruthFromStaticData weather;
      const auto filename = Write("weather.csv", data);
      EXPECT_THROW(
            weather.Initialize(filename.string(), Units::MetersLength(0), WeatherTruthFromStaticData::SIMULATION_TIME),
            std::runtime_error);
   }
}

TEST_F(WeatherTruthTest, HandlesUninitializedAndOutOfRangeSamples) {
   WeatherTruthFromStaticData weather;
   EXPECT_THROW(weather.Update({}, Units::MetersLength(0), Units::MetersLength(0)), std::runtime_error);
   const auto file = Write("weather.csv", weather_header + "10,1000,0,0,0,0,250\n20,0,0,0,0,0,260");
   weather.Initialize(file.string(), Units::MetersLength(0), WeatherTruthFromStaticData::SIMULATION_TIME);
   weather.Update(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(0)), Units::MetersLength(0),
                  Units::MetersLength(0));
   EXPECT_DOUBLE_EQ(weather.GetTemperature().value(), 250);
   weather.Update(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(100)), Units::MetersLength(0),
                  Units::MetersLength(0));
   EXPECT_DOUBLE_EQ(weather.GetTemperature().value(), 260);
}

}
