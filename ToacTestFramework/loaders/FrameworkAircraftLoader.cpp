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

#include <scalar/Length.h>

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "framework/NullAircraftPerformance.h"
#include "framework/PreloadedAdsbReceiver.h"
#include "public/AircraftControllerFactory.h"
#include "public/EllipsoidalPositionEstimator.h"
#include "public/FullWindTrueWeatherOperator.h"
#include "public/LegacyPositionEstimator.h"
#include "public/NullADSBReceiver.h"
#include "public/SpeedOnPitchControl.h"
#include "public/SpeedOnThrustControl.h"

#ifdef MITRE_BADA3_LIBRARY
#include "bada/Bada3Factory.h"
#endif

namespace mitre::oss::taft {

using namespace mitre::oss::simcore;

FrameworkAircraftLoader::FrameworkAircraftLoader(const AircraftConfiguration &configuration, int aircraft_id)
   : m_aircraft_id(aircraft_id),
     m_start_time(configuration.initial_time_seconds),
     m_mass_fraction(configuration.initial_mass_fraction),
     m_ac_type(configuration.ac_type),
     m_speed_management_type(configuration.speed_management_type),
     m_traffic_csv_file(configuration.traffic_csv_file.string()),
     m_env_csv_file(configuration.env_csv_file.string()),
     m_env_csv_data_index(configuration.env_data_index),
     m_guidance_loader(configuration.guidance),
     m_flightdeck_application_loader(configuration.speed_commands) {}

std::shared_ptr<TestFrameworkAircraft> FrameworkAircraftLoader::BuildAircraft() {
   auto bada_calculator = BuildAircraftPerformance(m_ac_type);
   auto guidance_calculator = m_guidance_loader.BuildGuidanceCalculator();
   auto true_weather =
         BuildTrueWeather(m_env_csv_file, m_env_csv_data_index,
                          Units::MetersLength(guidance_calculator->GetVerticalData().m_altitude_meters.back()));
   m_initial_local_position = ComputeInitialPositionOnPath(guidance_calculator);
   EarthModel::GeodeticPosition wgs84;
   m_guidance_loader.GetTangentPlaneSequence()->ConvertLocalToGeodetic(m_initial_local_position, wgs84);
   auto dynamics = BuildAircraftDynamics(bada_calculator, true_weather, guidance_calculator, wgs84);
   auto control = BuildAircraftControl(m_speed_management_type, bada_calculator);
   auto receiver = BuildAdsbReceiver(m_traffic_csv_file);
   auto initial_state =
         BuildInitialState(dynamics, Units::SecondsTime(m_start_time),
                           Units::MetersLength(guidance_calculator->GetVerticalData().m_altitude_meters.back()), wgs84);
   auto flightdeck_application = m_flightdeck_application_loader.CreateApplication();

   TestFrameworkAircraft::Builder builder;
   return builder.WithInitialState(initial_state)
         ->WithAircraftPerformance(bada_calculator)
         ->WithAircraftDynamics(dynamics)
         ->WithAircraftControl(control)
         ->WithTrueWeather(true_weather)
         ->WithAdsbReceiver(receiver)
         ->WithGuidanceCalculator(guidance_calculator)
         ->WithFlightDeckApplication(flightdeck_application)
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
      std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance,
      std::shared_ptr<WeatherTruthFromStaticData> &true_weather, std::shared_ptr<GuidanceFromStaticData> &guidance,
      const EarthModel::GeodeticPosition &initial_wgs84_position) {
   auto initial_altitude = Units::MetersLength(guidance->GetVerticalData().m_altitude_meters.back());
   Units::KnotsSpeed initial_ias = Units::MetersPerSecondSpeed(guidance->GetVerticalData().m_ias_mps.back());
   DirectionOfFlightCourseCalculator course_calculator = DirectionOfFlightCourseCalculator(
         guidance->GetHorizontalTrajectory(), TrajectoryIndexProgressionDirection::UNDEFINED);
   Units::Angle initial_heading = course_calculator.GetCourseAtPathStart();
   true_weather->Update(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(m_start_time)), Units::infinity(),
                        initial_altitude);
   Units::KnotsSpeed initial_tas = true_weather->getAtmosphere()->CAS2TAS(initial_ias, initial_altitude);
   std::shared_ptr<mitre::oss::simcore::EllipsoidalPositionEstimator> position_estimator =
         std::make_shared<mitre::oss::simcore::LegacyPositionEstimator>(m_guidance_loader.GetTangentPlaneSequence(),
                                                                        initial_wgs84_position);
   std::shared_ptr<mitre::oss::simcore::TrueWeatherOperator> true_weather_operator =
         std::make_shared<mitre::oss::simcore::FullWindTrueWeatherOperator>(true_weather);
   auto dynamics = std::make_shared<mitre::oss::simcore::ThreeDOFDynamics>();
   dynamics->Initialize(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(m_start_time)), performance,
                        initial_wgs84_position, m_initial_local_position, initial_altitude, initial_tas,
                        initial_heading, m_mass_fraction, position_estimator, true_weather_operator);
   return dynamics;
}

std::tuple<bool, EarthModel::LocalPositionEnu> FrameworkAircraftLoader::IsDistanceAlongPathValid(
      std::shared_ptr<GuidanceFromStaticData> &guidance_calculator,
      const Units::Length &target_distance_along_path) const {
   static Units::MetersLength along_path_distance_tolerance(10);
   const auto expected_trajectory_index = guidance_calculator->GetHorizontalTrajectory().size() - 1;
   AlongPathDistanceCalculator along_path_calculator(guidance_calculator->GetHorizontalTrajectory(),
                                                     TrajectoryIndexProgressionDirection::DECREMENTING);
   PositionCalculator position_calculator(guidance_calculator->GetHorizontalTrajectory(),
                                          TrajectoryIndexProgressionDirection::DECREMENTING);

   Units::Length computed_along_path_distance;
   Units::Length on_path_position_x(Units::negInfinity()), on_path_position_y(Units::negInfinity());
   Units::UnsignedAngle course_at_point(Units::negInfinity());

   try {
      position_calculator.CalculatePositionFromAlongPathDistance(target_distance_along_path, on_path_position_x,
                                                                 on_path_position_y, course_at_point);
      along_path_calculator.CalculateAlongPathDistanceFromPosition(on_path_position_x, on_path_position_y,
                                                                   computed_along_path_distance);
      bool is_position_valid =
            (along_path_calculator.GetCurrentTrajectoryIndex() == expected_trajectory_index) &&
            (position_calculator.GetCurrentTrajectoryIndex() == expected_trajectory_index) &&
            (Units::abs(computed_along_path_distance - target_distance_along_path) < along_path_distance_tolerance);
      EarthModel::LocalPositionEnu local_position_enu =
            EarthModel::LocalPositionEnu::Of(on_path_position_x, on_path_position_y, Units::zero());
      return std::make_tuple(is_position_valid, local_position_enu);
   } catch (std::exception &e) {
      return std::make_tuple(false, EarthModel::LocalPositionEnu::OfZeros());
   }
}

EarthModel::LocalPositionEnu FrameworkAircraftLoader::ComputeInitialPositionOnPath(
      std::shared_ptr<GuidanceFromStaticData> &guidance_calculator) {
   static Units::MetersLength along_path_distance_step(1);

   Units::Length dtg_guess =
         Units::MetersLength(guidance_calculator->GetHorizontalTrajectory().back().m_path_length_cumulative_meters);
   auto dtg_result = IsDistanceAlongPathValid(guidance_calculator, dtg_guess);
   bool dtg_is_valid = std::get<0>(dtg_result);
   int loop_counter = 0;
   while (!dtg_is_valid && loop_counter < 100) {
      dtg_guess = dtg_guess - along_path_distance_step;
      dtg_result = IsDistanceAlongPathValid(guidance_calculator, dtg_guess);
      dtg_is_valid = std::get<0>(dtg_result);
      ++loop_counter;
   }
   if (!dtg_is_valid) {
      std::string msg =
            "Unable to find a start position on the path that passes validation checks; the HFP file may be malformed";
      throw std::runtime_error(msg);
   }

   return std::get<1>(dtg_result);
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

std::shared_ptr<mitre::oss::simcore::ADSBReceiver> FrameworkAircraftLoader::BuildAdsbReceiver(
      std::string traffic_csv_file) {
   if (!traffic_csv_file.empty()) {
      const Units::NauticalMilesLength adsb_reception_distance(90);
      std::shared_ptr<mitre::oss::simcore::ADSBReceiver> adsb_receiver =
            std::make_shared<PreloadedAdsbReceiver>(traffic_csv_file, m_guidance_loader.GetTangentPlaneSequence());
      adsb_receiver->Initialize(adsb_reception_distance);
      return adsb_receiver;
   }
   return std::make_shared<mitre::oss::simcore::NullADSBReceiver>();
}

mitre::oss::simcore::AircraftState FrameworkAircraftLoader::BuildInitialState(
      std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> &dynamics, Units::Time start_time,
      Units::Length initial_altitude, const EarthModel::GeodeticPosition &wgs84_position) {
   std::pair<Units::Speed, Units::Speed> wind_components = dynamics->GetWindComponents();
   return mitre::oss::simcore::AircraftState::Builder(m_aircraft_id, start_time)
         .Position(m_initial_local_position.x, m_initial_local_position.y)
         ->Latitude(wgs84_position.latitude)
         ->Longitude(wgs84_position.longitude)
         ->AltitudeMsl(initial_altitude)
         ->GroundSpeed(dynamics->GetDynamicsState().xd, dynamics->GetDynamicsState().yd)
         ->SensedWindComponents(wind_components.first, wind_components.second)
         ->DynamicsState(dynamics->GetDynamicsState())
         ->Build();
}

}
