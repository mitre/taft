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

#include "framework/SpeedCommandsLoader.h"

#include <string>
#include <vector>

#include "MiniCSV/minicsv.h"


namespace mitre::oss::taft::loader {

bool SpeedCommandsLoader::load(DecodedStream *input) {
   set_stream(input);
   register_var("imspd_csv_file", &m_file_path, true);
   m_loaded = complete();
   return m_loaded;
}

SpeedCommandsFromStaticData SpeedCommandsLoader::Build(Units::Time pilot_delay_duration) const {
   if (m_loaded && !m_file_path.empty()) {
      const auto speed_data = ReadStaticSpeedCommands(m_file_path);
      return SpeedCommandsFromStaticData{speed_data, pilot_delay_duration};
   }
   return SpeedCommandsFromStaticData();
}

const std::vector<SpeedCommandsFromStaticData::SpeedRecord> SpeedCommandsLoader::ReadStaticSpeedCommands(
      const std::string &filename) const {
   std::ifstream file(filename.c_str());
   if (!file.is_open()) {
      std::string error_msg = "Speed file " + filename + " not found.";
      throw std::runtime_error(error_msg);
   }

   std::vector<SpeedCommandsFromStaticData::SpeedRecord> speed_data;
   mini::csv::ifstream input_stream(filename);
   input_stream.set_delimiter(',', ",");
   if (input_stream.is_open()) {
      input_stream.read_line();
      while (input_stream.read_line()) {
         double simtime_seconds, speed_command_mps;
         input_stream >> simtime_seconds >> speed_command_mps;
         SpeedCommandsFromStaticData::SpeedRecord speed_record;
         speed_record.simtime = Units::SecondsTime(simtime_seconds);
         speed_record.speed_command = Units::MetersPerSecondSpeed(speed_command_mps);
         speed_data.push_back(speed_record);
      }
   }
   input_stream.close();

   return speed_data;
}

}  // namespace mitre::oss::taft::loader
