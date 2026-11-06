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
#include <tuple>

#include "framework/ApplicationLoader.h"
#include "framework/GuidanceDataLoader.h"
#include "framework/TestFrameworkAircraft.h"
#include "public/FixedMassAircraftPerformance.h"
#include "public/WeatherTruth.h"

namespace mitre::oss::taft {
class FrameworkAircraftLoader final {
  public:
   explicit FrameworkAircraftLoader(const AircraftConfiguration &configuration);
   std::shared_ptr<TestFrameworkAircraft> BuildAircraft();

  private:
   int m_start_time{0};
   double m_mass_fraction{0};
   std::string m_ac_type{};
   std::string m_speed_management_type{};
   std::string m_traffic_csv_file{};
   std::string m_env_csv_file{};
   std::string m_env_csv_data_index{};
   GuidanceDataLoader m_guidance_loader;
   ApplicationLoader m_flightdeck_application_loader;
   mitre::oss::simcore::EarthModel::LocalPositionEnu m_initial_local_position{};

   std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> BuildAircraftPerformance(
         const std::string &bada_aircraft_code);
   std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> BuildAircraftDynamics(
         std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance,
         std::shared_ptr<WeatherTruthFromStaticData> &true_weather, std::shared_ptr<GuidanceFromStaticData> &guidance,
         const mitre::oss::simcore::EarthModel::GeodeticPosition &initial_wgs84_position);
   std::shared_ptr<mitre::oss::simcore::AircraftControl> BuildAircraftControl(
         std::string control_method,
         std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &bada_calculator);
   std::shared_ptr<WeatherTruthFromStaticData> BuildTrueWeather(const std::string &env_csv_file,
                                                                       const std::string &env_csv_data_index,
                                                                       Units::Length initial_altitude);
   std::shared_ptr<mitre::oss::simcore::ADSBReceiver> BuildAdsbReceiver(std::string traffic_csv_file);
   mitre::oss::simcore::AircraftState BuildInitialState(
         std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> &dynamics, Units::Time start_time,
         Units::Length initial_altitude, const mitre::oss::simcore::EarthModel::GeodeticPosition &wgs84_position);
   mitre::oss::simcore::EarthModel::LocalPositionEnu ComputeInitialPositionOnPath(
         std::shared_ptr<GuidanceFromStaticData> &guidance);
   std::tuple<bool, mitre::oss::simcore::EarthModel::LocalPositionEnu> IsDistanceAlongPathValid(
         std::shared_ptr<GuidanceFromStaticData> &guidance, const Units::Length &target_distance_along_path) const;
};
}  // namespace mitre::oss::taft
