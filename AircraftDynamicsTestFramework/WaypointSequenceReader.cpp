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

#include "framework/WaypointSequenceReader.h"

#include <vector>


namespace mitre::oss::taft {

std::vector<WaypointSequenceReader::WaypointSequenceRow> WaypointSequenceReader::ReadFile() {
   std::vector<WaypointSequenceRow> data;
   while (Advance()) {
      Units::DegreesAngle latitude(GetDouble(0));
      Units::DegreesAngle longitude(GetDouble(1));
      Units::DegreesAngle bank_angle(GetDouble(3));
      Units::MetersLength turn_radius(GetDouble(2));

      WaypointSequenceReader::WaypointSequenceRow data_row;
      data_row.latitude = latitude;
      data_row.longitude = longitude;
      data_row.bank_angle = bank_angle;
      data_row.turn_radius = turn_radius;

      data.push_back(data_row);
   }

   static Units::MetersLength tolerance(1);
   const bool invalid_first_row = data.front().turn_radius > tolerance;
   const bool invalid_last_row = data.back().turn_radius > tolerance;
   if (invalid_first_row || invalid_last_row) {
      throw std::runtime_error(
            "Waypoint sequence is not allowed to start or end with a turn segment. Please remove the non-zero turn "
            "radius from the first and last row.");
   }

   return data;
}

}  // namespace mitre::oss::taft
