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

#include <scalar/Length.h>

#include <memory>
#include <vector>

#include "framework/GuidanceFromStaticData.h"
#include "framework/WeatherTruthFromStaticData.h"
#include "public/AircraftControl.h"
#include "public/AircraftState.h"
#include "public/FixedMassAircraftPerformance.h"
#include "public/FlightDeckApplication.h"
#include "public/NullADSBReceiver.h"
#include "public/NullFlightDeckApplication.h"
#include "public/ScenarioEntity.h"
#include "public/SimulationTime.h"
#include "public/ThreeDOFDynamics.h"
#include "public/WeatherTruth.h"

namespace mitre::oss::taft {

class TestFrameworkAircraft final : public aaesim::open_source::ScenarioEntity {
  public:
   bool Update(const aaesim::open_source::SimulationTime &time) override;

   const std::vector<aaesim::open_source::AircraftState> &GetAircraftStates() const { return m_states; }

   const int GetStartTime() const override { return m_states[0].GetTime().value(); };

   bool IsFinished() const override {
      return m_guidance_calculator->GetEstimatedDistanceAlongPath() < Units::ZERO_LENGTH;
   }

   std::shared_ptr<aaesim::open_source::FlightDeckApplication> GetFlightDeckApplication() const {
      return m_speed_application;
   }

   class Builder final {
     public:
      Builder()
         : performance_(),
           aircraft_dynamics_model_(),
           aircraft_control_(),
           true_weather_(std::make_shared<WeatherTruthFromStaticData>(
                 WeatherTruthFromStaticData::CreateZeroTruthWind())),
           receiver_(std::make_shared<aaesim::open_source::NullADSBReceiver>()),
           guidance_calculator_(),
           speed_application_(std::make_shared<aaesim::open_source::NullFlightDeckApplication>()),
           initial_state_() {};
      ~Builder() = default;
      std::shared_ptr<TestFrameworkAircraft> Build() const;
      Builder *WithAircraftPerformance(std::shared_ptr<aaesim::open_source::FixedMassAircraftPerformance> &performance);
      Builder *WithAircraftDynamics(std::shared_ptr<aaesim::open_source::ThreeDOFDynamics> &dynamics);
      Builder *WithAircraftControl(std::shared_ptr<aaesim::open_source::AircraftControl> &control);
      Builder *WithTrueWeather(std::shared_ptr<WeatherTruthFromStaticData> &true_weather);
      Builder *WithAdsbReceiver(std::shared_ptr<aaesim::open_source::ADSBReceiver> &receiver);
      Builder *WithGuidanceCalculator(std::shared_ptr<GuidanceFromStaticData> &guidance_calculator);
      Builder *WithFlightDeckApplication(
            std::shared_ptr<aaesim::open_source::FlightDeckApplication> &speed_application);
      Builder *WithInitialState(const aaesim::open_source::AircraftState &initial_state);

      std::shared_ptr<aaesim::open_source::FixedMassAircraftPerformance> GetAircraftPerformance() const {
         return performance_;
      }
      std::shared_ptr<aaesim::open_source::ThreeDOFDynamics> GetDynamicsModel() const {
         return aircraft_dynamics_model_;
      }
      std::shared_ptr<aaesim::open_source::AircraftControl> GetAircraftControl() const { return aircraft_control_; }
      std::shared_ptr<WeatherTruthFromStaticData> GetWeatherTruth() const { return true_weather_; }
      std::shared_ptr<aaesim::open_source::ADSBReceiver> GetAdsbReceiver() const { return receiver_; }
      std::shared_ptr<GuidanceFromStaticData> GetGuidanceCalculator() const { return guidance_calculator_; }
      std::shared_ptr<aaesim::open_source::FlightDeckApplication> GetFligthDeckApplication() const {
         return speed_application_;
      }
      const aaesim::open_source::AircraftState &GetInitialState() const { return initial_state_; }

     private:
      std::shared_ptr<aaesim::open_source::FixedMassAircraftPerformance> performance_;
      std::shared_ptr<aaesim::open_source::ThreeDOFDynamics> aircraft_dynamics_model_;
      std::shared_ptr<aaesim::open_source::AircraftControl> aircraft_control_;
      std::shared_ptr<WeatherTruthFromStaticData> true_weather_;
      std::shared_ptr<aaesim::open_source::ADSBReceiver> receiver_;
      std::shared_ptr<GuidanceFromStaticData> guidance_calculator_;
      std::shared_ptr<aaesim::open_source::FlightDeckApplication> speed_application_;
      aaesim::open_source::AircraftState initial_state_;
   };

   TestFrameworkAircraft(const TestFrameworkAircraft::Builder &builder);

  private:
   static log4cplus::Logger m_logger;

   TestFrameworkAircraft();

   void SaveState(aaesim::open_source::AircraftState &state, const aaesim::open_source::SimulationTime &time);
   void AddLatitudeLongitude(aaesim::open_source::AircraftState &state) const;

   std::shared_ptr<WeatherTruthFromStaticData> m_weather_truth;
   std::shared_ptr<aaesim::open_source::ADSBReceiver> m_adsb_receiver;
   std::shared_ptr<aaesim::open_source::ThreeDOFDynamics> m_dynamics;
   std::shared_ptr<GuidanceFromStaticData> m_guidance_calculator;
   std::shared_ptr<aaesim::open_source::AircraftControl> m_aircraft_control;
   std::shared_ptr<aaesim::open_source::FixedMassAircraftPerformance> m_bada_calculator;
   std::shared_ptr<aaesim::open_source::FlightDeckApplication> m_speed_application;
   std::vector<aaesim::open_source::AircraftState> m_states;
};

}  // namespace mitre::oss::taft
