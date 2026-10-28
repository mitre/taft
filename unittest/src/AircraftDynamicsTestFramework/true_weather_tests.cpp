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

#include "framework/WeatherTruthFromStaticData.h"
#include "gtest/gtest.h"

namespace mitre::oss::taft {
namespace test {

TEST(WeatherTruthFromStaticData, updated_using_simtime) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData();
   test_weather.Initialize("resources/test_env_file.csv", Units::zero(),
                           WeatherTruthFromStaticData::DataIndexParameter::SIMULATION_TIME);
   test_weather.Update(aaesim::open_source::SimulationTime::Of(Units::SecondsTime(1.0)), Units::zero(), Units::zero());
   Units::KelvinTemperature actual_temp = test_weather.GetTemperature();
   const Units::KelvinTemperature expected_temp(226);
   EXPECT_NEAR(actual_temp.value(), expected_temp.value(), 1e-10);
}

TEST(WeatherTruthFromStaticData, updated_using_dtg) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData();
   test_weather.Initialize("resources/test_env_file.csv", Units::zero(),
                           WeatherTruthFromStaticData::DataIndexParameter::DISTANCE_TO_GO);
   test_weather.Update(aaesim::open_source::SimulationTime::Of(Units::ZERO_TIME), Units::MetersLength(900.0),
                       Units::zero());
   Units::KelvinTemperature actual_temp = test_weather.GetTemperature();
   const Units::KelvinTemperature expected_temp(226);
   EXPECT_NEAR(actual_temp.value(), expected_temp.value(), 1e-10);
}

TEST(WeatherTruthFromStaticData, updated_interpolate_using_dtg) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData();
   test_weather.Initialize("resources/test_env_file.csv", Units::zero(),
                           WeatherTruthFromStaticData::DataIndexParameter::DISTANCE_TO_GO);
   test_weather.Update(aaesim::open_source::SimulationTime::Of(Units::ZERO_TIME), Units::MetersLength(900.0),
                       Units::zero());
   test_weather.LoadConditionsAt(Units::ZERO_ANGLE, Units::ZERO_ANGLE, Units::ZERO_LENGTH);
   Units::KelvinTemperature actual_temp = test_weather.GetTemperature();
   const Units::KelvinTemperature expected_temp(226);
   EXPECT_NEAR(actual_temp.value(), expected_temp.value(), 1e-10);
}

TEST(WeatherTruthFromStaticData, create_zero) {
   WeatherTruthFromStaticData test_weather = WeatherTruthFromStaticData::CreateZeroTruthWind();
   test_weather.Update(aaesim::open_source::SimulationTime::Of(Units::ZERO_TIME), Units::zero(), Units::zero());
   Units::KelvinTemperature expected_temp(288.15);
   EXPECT_EQ(test_weather.GetTemperature().value(), expected_temp.value());
}

}  // namespace test
}  // namespace mitre::oss::taft
