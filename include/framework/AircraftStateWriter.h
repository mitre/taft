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

#include <string>
#include <vector>

#include "public/AircraftState.h"
#include "public/OutputHandler.h"

namespace mitre::oss::taft {
class AircraftStateWriter final : public OutputHandler {
  public:
   AircraftStateWriter() : OutputHandler("", "_AcStates.csv"), m_data_to_write() {}
   void Finish() override;
   void Gather(std::vector<aaesim::open_source::AircraftState> aircraft_states);

  private:
   static std::vector<std::string> COLUMN_NAMES;
   struct DataToWrite {
      DataToWrite() = default;
      ~DataToWrite() = default;

      Units::Time simulation_time{Units::NegInfinity()};
      Units::Speed dynamics_ias{Units::NegInfinity()};
      Units::Speed dynamics_tas{Units::NegInfinity()};
      Units::Speed dynamics_altitude_rate{Units::NegInfinity()};
      Units::Length euclidean_x{Units::NegInfinity()};
      Units::Length euclidean_y{Units::NegInfinity()};
      Units::Length altitude_msl{Units::NegInfinity()};
      Units::Speed dynamics_ground_speed{Units::NegInfinity()};
      Units::DegreesAngle latitude{0}, longitude{0};
   };

   std::vector<DataToWrite> m_data_to_write;
};
}  // namespace mitre::oss::taft
