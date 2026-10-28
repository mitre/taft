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

#include "framework/AircraftStateWriter.h"

#include <string>
#include <vector>

namespace mitre::oss::taft {

std::vector<std::string> AircraftStateWriter::COLUMN_NAMES = {
      "Time[sec]", "V(ias)[m/s]", "V(tas)[m/s]", "vRate[m/s]",    "x[m]",
      "y[m]",      "h[m]",        "gs[mps]",     "latitude[deg]", "longitude[deg]"};

void AircraftStateWriter::Finish() {
   if (m_data_to_write.empty()) {
      return;
   }

   mini::csv::ofstream os(filename.c_str());

   if (!os.is_open()) {
      std::string emsg = "Cannot open " + filename;
      return;
   }

   os.set_delimiter(',', ",");

   auto column_inserter = [&os](std::string &column_name) { os << column_name; };
   std::for_each(COLUMN_NAMES.begin(), COLUMN_NAMES.end(), column_inserter);
   os << NEWLINE;

   os.set_precision(6);
   auto data_inserter = [&os](const DataToWrite &data_row) {
      os << Units::SecondsTime(data_row.simulation_time).value();
      os << Units::MetersPerSecondSpeed(data_row.dynamics_ias).value();
      os << Units::MetersPerSecondSpeed(data_row.dynamics_tas).value();
      os << Units::MetersPerSecondSpeed(data_row.dynamics_altitude_rate).value();
      os << Units::MetersLength(data_row.euclidean_x).value();
      os << Units::MetersLength(data_row.euclidean_y).value();
      os << Units::MetersLength(data_row.altitude_msl).value();
      os << Units::MetersPerSecondSpeed(data_row.dynamics_ground_speed).value();
      os << data_row.latitude.value();
      os << data_row.longitude.value();
      os << NEWLINE;
   };
   std::for_each(m_data_to_write.cbegin(), m_data_to_write.cend(), data_inserter);

   os.close();
   m_data_to_write.clear();
}

void AircraftStateWriter::Gather(std::vector<aaesim::open_source::AircraftState> aircraft_states) {
   auto data_gatherer = [this](const aaesim::open_source::AircraftState &state) {
      DataToWrite data;
      data.altitude_msl = state.GetAltitudeMsl();
      data.dynamics_altitude_rate = state.GetVerticalSpeed();
      data.dynamics_ground_speed = state.GetGroundSpeed();
      data.dynamics_ias = state.GetDynamicsState().v_indicated_airspeed;
      data.dynamics_tas = state.GetDynamicsState().v_true_airspeed;
      data.euclidean_x = state.GetPositionEnuX();
      data.euclidean_y = state.GetPositionEnuY();
      data.simulation_time = state.GetTime();
      data.latitude = state.GetLatitude();
      data.longitude = state.GetLongitude();
      this->m_data_to_write.push_back(data);
   };
   std::for_each(aircraft_states.cbegin(), aircraft_states.cend(), data_gatherer);
}

}  // namespace mitre::oss::taft
