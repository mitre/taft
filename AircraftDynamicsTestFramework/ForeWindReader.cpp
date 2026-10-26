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

#include "framework/ForeWindReader.h"

#include <string>

namespace mitre::oss::taft::testvector {

ForeWindReader::ForeWindReader(const std::string &file_name, int header_lines)
   : DataReader(file_name, header_lines, 0) {}

ForeWindReader::ForeWindReader(std::shared_ptr<std::istream> input_stream, int header_lines)
   : DataReader(input_stream, header_lines, 0) {}

ForeWindReader::~ForeWindReader() {}

bool ForeWindReader::ReadWind(aaesim::open_source::WeatherPrediction &weather_prediction) {
   const int ALTITUDE_COUNT(5);

   weather_prediction.east_west().SetBounds(1, ALTITUDE_COUNT);
   weather_prediction.north_south().SetBounds(1, ALTITUDE_COUNT);
   for (int i = 1; i <= ALTITUDE_COUNT; i++) {
      Advance();
      if (GetColumnCount() == 0) {
         // end of stream
         return false;
      }
      Units::FeetLength altitude(GetDouble(0));
      Units::KnotsSpeed u(GetDouble(1));
      Units::KnotsSpeed v(GetDouble(2));
      weather_prediction.east_west().Insert(i, altitude, u);
      weather_prediction.north_south().Insert(i, altitude, v);
   }

   return true;
}

}  // namespace mitre::oss::taft::testvector
