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

#include "public/AircraftState.h"
#include "public/AircraftIntent.h"
#include "framework/IMSpeedCommandFile.h"
#include "loader/Loadable.h"
#include "public/Guidance.h"
#include "public/SimulationTime.h"
#include "public/ThreeDOFDynamics.h"
#include <string>
#include <vector>
#include <map>

namespace mitre::oss::taft {

class TestFrameworkApplication : public Loadable {
  public:
   TestFrameworkApplication();

   ~TestFrameworkApplication();

   aaesim::open_source::Guidance Update(const SimulationTime &simTime, aaesim::open_source::ThreeDOFDynamics &dynamics,
                                        const aaesim::open_source::AircraftState &state_in,
                                        const aaesim::open_source::Guidance &guidance_in);

   bool load(DecodedStream *input);

   // Conditionals
   bool IsLoaded();

  private:
   // Interval Management classes
   IMSpeedCommandFile m_im_speed_command_file;

   // Input Data
   std::string m_application_type;

   bool m_loaded;
};

}  // namespace mitre::oss::taft
