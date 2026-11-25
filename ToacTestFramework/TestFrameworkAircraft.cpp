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

#include "framework/TestFrameworkAircraft.h"

#include <memory>
#include <stdexcept>
#include <string>

#include "public/Guidance.h"

namespace mitre::oss::taft {

using namespace std;
using namespace mitre::oss::simcore;

log4cplus::Logger TestFrameworkAircraft::m_logger =
      log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("TestFrameworkAircraft"));

bool TestFrameworkAircraft::Update(const SimulationTime &time) {
   if (IsFinished()) return true;
   if (time.GetCurrentSimulationTime() <= m_states.cbegin()->GetTime()) {
      return false;
   }

   const AircraftState &previous_state = m_states.back();

   m_weather_truth->Update(time, m_fms_test_bench->GetDistanceToGo(), previous_state.GetAltitudeMsl());

   const Guidance current_guidance = m_fms_test_bench->Update(time, previous_state);
   if (!current_guidance.IsValid()) {
      throw std::runtime_error("FMS test bench returned invalid guidance for aircraft " +
                               std::to_string(previous_state.GetUniqueId()));
   }
   AircraftState state_result =
         m_dynamics->Update(previous_state.GetUniqueId(), time, current_guidance, m_aircraft_control);
   SaveState(state_result, time);

   return IsFinished();
}

void TestFrameworkAircraft::SaveState(AircraftState &state, const SimulationTime &time) { m_states.push_back(state); }

std::shared_ptr<TestFrameworkAircraft> TestFrameworkAircraft::Builder::Build() const {
   return std::make_shared<TestFrameworkAircraft>(*this);
}

TestFrameworkAircraft::TestFrameworkAircraft(const Builder &builder)
   : m_weather_truth(builder.GetWeatherTruth()),
     m_dynamics(builder.GetDynamicsModel()),
     m_fms_test_bench(builder.GetFmsTestBench()),
     m_aircraft_control(builder.GetAircraftControl()),
     m_bada_calculator(builder.GetAircraftPerformance()),
     m_states{builder.GetInitialState()} {
   if (!m_fms_test_bench) throw std::invalid_argument("An FMS test bench is required");
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithAircraftPerformance(
      std::shared_ptr<mitre::oss::simcore::FixedMassAircraftPerformance> &performance) {
   performance_ = performance;
   return this;
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithAircraftDynamics(
      std::shared_ptr<mitre::oss::simcore::ThreeDOFDynamics> &dynamics) {
   aircraft_dynamics_model_ = dynamics;
   return this;
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithAircraftControl(
      std::shared_ptr<mitre::oss::simcore::AircraftControl> &control) {
   aircraft_control_ = control;
   return this;
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithTrueWeather(
      std::shared_ptr<WeatherTruthFromStaticData> &true_weather) {
   true_weather_ = true_weather;
   return this;
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithFmsTestBench(
      const std::shared_ptr<FmsTestBench> &bench) {
   fms_test_bench_ = bench;
   return this;
}

TestFrameworkAircraft::Builder *TestFrameworkAircraft::Builder::WithInitialState(
      const mitre::oss::simcore::AircraftState &initial_state) {
   initial_state_ = initial_state;
   return this;
}

}  // namespace mitre::oss::taft
