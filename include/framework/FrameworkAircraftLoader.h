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

#include <memory>
#include <string>

#include "framework/Configuration.h"
#include "framework/FmsTestBench.h"
#include "framework/TestFrameworkAircraft.h"
#include "public/FixedMassAircraftPerformance.h"

namespace mitre::oss::taft {
class FrameworkAircraftLoader final {
  public:
   FrameworkAircraftLoader(const AircraftConfiguration &configuration, int aircraft_id,
                           std::shared_ptr<FmsTestBench> bench);
   std::shared_ptr<TestFrameworkAircraft> BuildAircraft();

  private:
   int m_aircraft_id;
   int m_start_time{0};
   double m_mass_fraction{0};
   std::string m_ac_type{};
   std::string m_speed_management_type{};
   std::string m_env_csv_file{};
   std::string m_env_csv_data_index{};
   std::shared_ptr<FmsTestBench> m_fms_test_bench;

   std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> BuildAircraftPerformance(
         const std::string &bada_aircraft_code);
   std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> BuildAircraftDynamics(
         const std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance,
         const std::shared_ptr<WeatherTruthFromStaticData> &true_weather,
         const FmsTestBench::InitialConditions &initial_conditions);
   std::shared_ptr<mitre::oss::simcore::AircraftControl> BuildAircraftControl(
         std::string control_method,
         std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &bada_calculator);
   std::shared_ptr<WeatherTruthFromStaticData> BuildTrueWeather(const std::string &env_csv_file,
                                                                       const std::string &env_csv_data_index,
                                                                       Units::Length initial_altitude);
   mitre::oss::simcore::AircraftState BuildInitialState(
         const mitre::oss::simcore::ThreeDOFDynamics &dynamics, const FmsTestBench::InitialConditions &initial_conditions);
};
}  // namespace mitre::oss::taft
