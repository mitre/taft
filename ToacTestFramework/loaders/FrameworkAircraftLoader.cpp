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

#include "framework/FrameworkAircraftLoader.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "framework/NullAircraftPerformance.h"
#include "public/AircraftControllerFactory.h"
#include "public/FullWindTrueWeatherOperator.h"

#ifdef MITRE_BADA3_LIBRARY
#include "bada/Bada3Factory.h"
#endif

namespace mitre::oss::taft {

using namespace mitre::oss::simcore;

FrameworkAircraftLoader::FrameworkAircraftLoader(const AircraftConfiguration &configuration, int aircraft_id,
                                                 std::shared_ptr<FmsTestBench> bench)
   : m_aircraft_id(aircraft_id),
     m_start_time(configuration.initial_time_seconds),
     m_mass_fraction(configuration.initial_mass_fraction),
     m_ac_type(configuration.ac_type),
     m_speed_management_type(configuration.speed_management_type),
     m_env_csv_file(configuration.env_csv_file.string()),
     m_env_csv_data_index(configuration.env_data_index),
     m_fms_test_bench(std::move(bench)) {
   if (!m_fms_test_bench) throw std::invalid_argument("An FMS test bench is required");
}

std::shared_ptr<TestFrameworkAircraft> FrameworkAircraftLoader::BuildAircraft() {
   const auto initial_conditions = m_fms_test_bench->GetInitialConditions();
   if (!initial_conditions.position_estimator) {
      throw std::runtime_error("Supplied FMS test bench did not provide a position estimator for aircraft " +
                               std::to_string(m_aircraft_id));
   }
   auto bada_calculator = BuildAircraftPerformance(m_ac_type);
   auto true_weather = BuildTrueWeather(m_env_csv_file, m_env_csv_data_index, initial_conditions.altitude_msl);
   auto dynamics = BuildAircraftDynamics(bada_calculator, true_weather, initial_conditions);
   auto control = BuildAircraftControl(m_speed_management_type, bada_calculator);
   const auto initial_state = BuildInitialState(dynamics, initial_conditions);

   TestFrameworkAircraft::Builder builder;
   return builder.WithInitialState(initial_state)
         ->WithAircraftPerformance(bada_calculator)
         ->WithAircraftDynamics(dynamics)
         ->WithAircraftControl(control)
         ->WithTrueWeather(true_weather)
         ->WithFmsTestBench(m_fms_test_bench)
         ->Build();
}

std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> FrameworkAircraftLoader::BuildAircraftPerformance(
      const std::string &bada_aircraft_code) {
#ifdef MITRE_BADA3_LIBRARY
   aaesim::bada::BadaPerformanceInitialConditions initial_conditions;
   initial_conditions.faf_altitude_msl = Units::FeetLength(1500);
   initial_conditions.initial_flap_configuration = mitre::oss::simcore::bada_utils::FlapConfiguration::CRUISE;
   initial_conditions.mass_percentile = m_mass_fraction;
   return aaesim::bada::Bada3Factory::CreateBada3ForwardProgression(bada_aircraft_code, initial_conditions);
#else
   return std::make_shared<NullAircraftPerformance>();
#endif
}

std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> FrameworkAircraftLoader::BuildAircraftDynamics(
      const std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance,
      const std::shared_ptr<WeatherTruthFromStaticData> &true_weather,
      const FmsTestBench::InitialConditions &initial_conditions) {
   const auto start_time = SimulationTime::Of(Units::SecondsTime(m_start_time));
   true_weather->Update(start_time, m_fms_test_bench->GetDistanceToGo(), initial_conditions.altitude_msl);
   const auto initial_tas =
         true_weather->getAtmosphere()->CAS2TAS(initial_conditions.indicated_airspeed, initial_conditions.altitude_msl);
   std::shared_ptr<mitre::oss::simcore::TrueWeatherOperator> true_weather_operator =
         std::make_shared<mitre::oss::simcore::FullWindTrueWeatherOperator>(true_weather);
   auto dynamics = std::make_shared<mitre::oss::simcore::ThreeDOFDynamics>();
   dynamics->Initialize(start_time, performance, initial_conditions.geodetic_position, initial_conditions.local_position,
                        initial_conditions.altitude_msl, initial_tas, initial_conditions.ground_course_enu,
                        m_mass_fraction, initial_conditions.position_estimator, true_weather_operator);
   return dynamics;
}

std::shared_ptr<WeatherTruthFromStaticData> FrameworkAircraftLoader::BuildTrueWeather(
      const std::string &env_csv_file, const std::string &env_csv_data_index, Units::Length initial_altitude) {
   if (!env_csv_file.empty()) {
      auto weather_truth = std::make_shared<WeatherTruthFromStaticData>();
      weather_truth->Initialize(env_csv_file, initial_altitude,
                                WeatherTruthFromStaticData::DataIndexFromString(env_csv_data_index));
      return weather_truth;
   } else {
      return std::make_shared<WeatherTruthFromStaticData>(
            WeatherTruthFromStaticData::CreateZeroTruthWind());
   }
}

std::shared_ptr<mitre::oss::simcore::AircraftControl> FrameworkAircraftLoader::BuildAircraftControl(
      std::string control_method, std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &bada_calculator) {
   AircraftControllerFactory::DescentSpeedControlStrategy descent_strategy{
         AircraftControllerFactory::DescentSpeedControlStrategy::NONE};
   std::transform(control_method.cbegin(), control_method.cend(), control_method.begin(),
                  [](unsigned char c) { return std::tolower(c); });
   if (control_method == "thrust") {
      descent_strategy = AircraftControllerFactory::DescentSpeedControlStrategy::THRUST;
   } else if (control_method == "pitch") {
      descent_strategy = AircraftControllerFactory::DescentSpeedControlStrategy::PITCH;
   }
   auto descent_config = AircraftControllerFactory::DescentSpeedControlConfig{descent_strategy, Units::KnotsSpeed(20.0),
                                                                              Units::FeetLength(500.0)};
   auto config = AircraftControllerFactory::AircraftControllerConfig{descent_config, Units::DegreesAngle{30}};
   auto aircraft_control = AircraftControllerFactory::BuildForCruiseDescentOnly(config);
   aircraft_control->Initialize(bada_calculator);
   return aircraft_control;
}

mitre::oss::simcore::AircraftState FrameworkAircraftLoader::BuildInitialState(
      const mitre::oss::simcore::ThreeDOFDynamics &dynamics, const FmsTestBench::InitialConditions &initial_conditions) {
   const auto wind_components = dynamics.GetWindComponents();
   const auto dynamics_state = dynamics.GetDynamicsState();
   return mitre::oss::simcore::AircraftState::Builder(m_aircraft_id, Units::SecondsTime(m_start_time))
         .Position(initial_conditions.local_position.x, initial_conditions.local_position.y)
         ->Latitude(initial_conditions.geodetic_position.latitude)
         ->Longitude(initial_conditions.geodetic_position.longitude)
         ->AltitudeMsl(initial_conditions.altitude_msl)
         ->GroundSpeed(dynamics_state.xd, dynamics_state.yd)
         ->SensedWindComponents(wind_components.first, wind_components.second)
         ->DynamicsState(dynamics_state)
         ->Build();
}

}
