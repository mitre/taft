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

#include "framework/TestFrameworkScenario.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "framework/AircraftStateWriter.h"
#include "framework/FrameworkAircraftLoader.h"

#ifdef MITRE_BADA3_LIBRARY
#include "bada/Bada3Factory.h"
#endif

namespace mitre::oss::taft {

log4cplus::Logger TestFrameworkScenario::m_logger = log4cplus::Logger::getInstance("TestFrameworkScenario");

TestFrameworkScenario::TestFrameworkScenario(const ScenarioConfiguration &configuration) {
   SetScenarioName(configuration.GetScenarioName());
#ifdef MITRE_BADA3_LIBRARY
   if (configuration.bada_data_path.empty())
      throw std::runtime_error(configuration.source_file.string() + ": bada_data_path: required by the BADA provider");
   aaesim::bada::Bada3Factory::SetBadaDataPath(configuration.bada_data_path.string(),
                                               mitre::oss::simcore::Atmosphere::AtmosphereType::BADA37);
#endif
   for (const auto &aircraft : configuration.aircraft)
      m_aircraft_in_scenario.push_back(FrameworkAircraftLoader(aircraft).BuildAircraft());
}

void TestFrameworkScenario::SimulateAllIterations() {

   AircraftStateWriter taft_state_writer;
   taft_state_writer.SetScenarioName(GetScenarioName());

   LOG4CPLUS_INFO(m_logger, "Running TAFT scenario " << GetScenarioName());
   mitre::oss::simcore::SimulationTime time;
   bool scenario_complete = false;
   while (!scenario_complete) {
      scenario_complete = AdvanceAllAircraft(time);
      time.Increment();
   }
   LOG4CPLUS_INFO(m_logger, "TAFT scenario complete; writing data files.");
   std::for_each(m_aircraft_in_scenario.cbegin(), m_aircraft_in_scenario.cend(),
                 [&taft_state_writer](const std::shared_ptr<TestFrameworkAircraft> &aircraft) {
                    taft_state_writer.Gather(aircraft->GetAircraftStates());
                 });

   taft_state_writer.Finish();
}

bool TestFrameworkScenario::AdvanceAllAircraft(mitre::oss::simcore::SimulationTime &time) {
   bool all_aircraft_complete = true;
   auto advance_aircraft = [&all_aircraft_complete, &time](std::shared_ptr<TestFrameworkAircraft> &aircraft) {
      bool aircraft_finished = aircraft->Update(time);

      all_aircraft_complete = all_aircraft_complete && aircraft_finished;
   };
   std::for_each(m_aircraft_in_scenario.begin(), m_aircraft_in_scenario.end(), advance_aircraft);
   return all_aircraft_complete;
}

}  // namespace mitre::oss::taft
