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

#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mitre::oss::taft::loader {
SpeedCommandsLoader::SpeedCommandsLoader(const SpeedCommandsConfiguration &configuration)
   : m_configuration(configuration) {}

SpeedCommandsFromStaticData SpeedCommandsLoader::Build() const {
   const auto filename = m_configuration.csv_file.string();
   std::ifstream file(filename);
   if (!file.is_open()) throw std::runtime_error("Cannot open speed commands CSV: " + filename);
   std::string line;
   if (!std::getline(file, line)) throw std::runtime_error(filename + ": missing CSV header");
   std::vector<SpeedCommandsFromStaticData::SpeedRecord> records;
   std::size_t line_number = 1;
   while (std::getline(file, line)) {
      ++line_number;
      std::istringstream row(line);
      row.imbue(std::locale::classic());
      double time, speed;
      char separator;
      if (!(row >> time >> separator >> speed) || separator != ',')
         throw std::runtime_error(filename + ": row " + std::to_string(line_number) + ": expected time,speed");
      row >> std::ws;
      if (!row.eof())
         throw std::runtime_error(filename + ": row " + std::to_string(line_number) + ": unexpected trailing data");
      SpeedCommandsFromStaticData::SpeedRecord record;
      record.simtime = Units::SecondsTime(time);
      record.speed_command = Units::MetersPerSecondSpeed(speed);
      records.push_back(record);
   }
   try {
      return SpeedCommandsFromStaticData(records, Units::SecondsTime(m_configuration.pilot_delay_seconds));
   } catch (const std::exception &error) {
      throw std::runtime_error(filename + ": " + error.what());
   }
}
}
