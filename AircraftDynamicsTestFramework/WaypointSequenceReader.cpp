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

#include "framework/NumericCsv.h"

namespace mitre::oss::taft {

std::vector<WaypointSequenceReader::WaypointSequenceRow> WaypointSequenceReader::ReadFile() {
   std::vector<WaypointSequenceRow> data;
   for (const auto &row : loader::ReadNumericCsv(m_filename, 4, 2)) {
      WaypointSequenceRow point;
      point.latitude = Units::DegreesAngle(row[0]);
      point.longitude = Units::DegreesAngle(row[1]);
      point.turn_radius = Units::MetersLength(row[2]);
      point.bank_angle = Units::DegreesAngle(row[3]);
      data.push_back(point);
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
