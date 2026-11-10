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

#include <cmath>
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utility/CsvParser.h"

namespace mitre::oss::taft::loader {
inline double ReadFiniteNumber(const std::string &text, const std::string &context) {
   std::istringstream cell(text);
   cell.imbue(std::locale::classic());
   double value;
   if (!(cell >> value) || !std::isfinite(value)) throw std::runtime_error(context + ": expected finite number");
   cell >> std::ws;
   if (!cell.eof()) throw std::runtime_error(context + ": unexpected text after number");
   return value;
}

inline std::vector<std::vector<double>> ReadNumericCsv(const std::string &filename, std::size_t columns,
                                                       std::size_t minimum_rows) {
   std::ifstream input(filename);
   if (!input.is_open()) throw std::runtime_error("Cannot open CSV: " + filename);
   std::string line;
   if (!std::getline(input, line) || line.empty()) throw std::runtime_error(filename + ": missing CSV header");
   std::vector<std::vector<double>> data;
   std::size_t row_number = 1;
   while (std::getline(input, line)) {
      ++row_number;
      std::istringstream stream(line);
      CsvParser::CsvRow row;
      row.ReadNextRow(stream);
      const auto context = filename + ": row " + std::to_string(row_number);
      if (row.Size() != columns) throw std::runtime_error(context + ": unexpected column count");
      std::vector<double> values;
      for (std::size_t column = 0; column < columns; ++column) {
         values.push_back(ReadFiniteNumber(row[column], context + ", column " + std::to_string(column + 1)));
      }
      data.push_back(std::move(values));
   }
   if (input.bad()) throw std::runtime_error(filename + ": failed while reading CSV");
   if (data.size() < minimum_rows)
      throw std::runtime_error(filename + ": expected at least " + std::to_string(minimum_rows) + " data rows");
   return data;
}
}
