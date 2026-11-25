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

#include <log4cplus/logger.h>

#include <memory>
#include <vector>

#include "framework/FmsTestBench.h"
#include "framework/WeatherTruthFromStaticData.h"
#include "public/AircraftControl.h"
#include "public/AircraftState.h"
#include "public/FixedMassAircraftPerformance.h"
#include "public/ScenarioEntity.h"
#include "public/SimulationTime.h"
#include "public/ThreeDOFDynamics.h"

namespace mitre::oss::taft {

class TestFrameworkAircraft final : public mitre::oss::simcore::ScenarioEntity {
  public:
   bool Update(const mitre::oss::simcore::SimulationTime &time) override;

   const std::vector<mitre::oss::simcore::AircraftState> &GetAircraftStates() const { return m_states; }

   const int GetStartTime() const override { return m_states[0].GetTime().value(); };

   bool IsFinished() const override { return m_fms_test_bench->IsFinished(); }

   class Builder final {
     public:
      Builder()
         : performance_(),
           aircraft_dynamics_model_(),
           aircraft_control_(),
           true_weather_(
                 std::make_shared<WeatherTruthFromStaticData>(WeatherTruthFromStaticData::CreateZeroTruthWind())),
           fms_test_bench_(),
           initial_state_() {};
      ~Builder() = default;
      std::shared_ptr<TestFrameworkAircraft> Build() const;
      Builder *WithAircraftPerformance(std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance);
      Builder *WithAircraftDynamics(std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> &dynamics);
      Builder *WithAircraftControl(std::shared_ptr<mitre::oss::simcore::AircraftControl> &control);
      Builder *WithTrueWeather(std::shared_ptr<WeatherTruthFromStaticData> &true_weather);
      Builder *WithFmsTestBench(const std::shared_ptr<FmsTestBench> &bench);
      Builder *WithInitialState(const mitre::oss::simcore::AircraftState &initial_state);

      std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> GetAircraftPerformance() const {
         return performance_;
      }
      std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> GetDynamicsModel() const {
         return aircraft_dynamics_model_;
      }
      std::shared_ptr<mitre::oss::simcore::AircraftControl> GetAircraftControl() const { return aircraft_control_; }
      std::shared_ptr<WeatherTruthFromStaticData> GetWeatherTruth() const { return true_weather_; }
      std::shared_ptr<FmsTestBench> GetFmsTestBench() const { return fms_test_bench_; }
      const mitre::oss::simcore::AircraftState &GetInitialState() const { return initial_state_; }

     private:
      std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> performance_;
      std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> aircraft_dynamics_model_;
      std::shared_ptr<mitre::oss::simcore::AircraftControl> aircraft_control_;
      std::shared_ptr<WeatherTruthFromStaticData> true_weather_;
      std::shared_ptr<FmsTestBench> fms_test_bench_;
      mitre::oss::simcore::AircraftState initial_state_;
   };

   TestFrameworkAircraft(const TestFrameworkAircraft::Builder &builder);

  private:
   static log4cplus::Logger m_logger;

   void SaveState(mitre::oss::simcore::AircraftState &state, const mitre::oss::simcore::SimulationTime &time);
   void AddLatitudeLongitude(mitre::oss::simcore::AircraftState &state) const;

   std::shared_ptr<WeatherTruthFromStaticData> m_weather_truth;
   std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> m_dynamics;
   std::shared_ptr<FmsTestBench> m_fms_test_bench;
   std::shared_ptr<mitre::oss::simcore::AircraftControl> m_aircraft_control;
   std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> m_bada_calculator;
   std::vector<mitre::oss::simcore::AircraftState> m_states;
};

}  // namespace mitre::oss::taft
