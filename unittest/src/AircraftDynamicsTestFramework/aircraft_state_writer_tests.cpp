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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "framework/AircraftStateWriter.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
class AircraftStateWriterTest : public TemporaryDirectoryTest {
  protected:
   std::vector<std::vector<std::string>> ReadCsv(const std::filesystem::path &file) {
      std::ifstream input(file);
      EXPECT_TRUE(input.is_open());
      std::vector<std::vector<std::string>> rows;
      for (std::string line; std::getline(input, line);) {
         std::istringstream stream(line);
         std::vector<std::string> cells;
         for (std::string cell; std::getline(stream, cell, ',');) cells.push_back(cell);
         rows.push_back(cells);
      }
      return rows;
   }
};

TEST_F(AircraftStateWriterTest, ReportsOpenFailure) {
   AircraftStateWriter writer;
   writer.SetScenarioName((directory / "does-not-exist/scenario").string());
   writer.Gather({mitre::oss::simcore::AircraftState::Builder(1, Units::SecondsTime(0)).Build()});
   EXPECT_THROW(writer.Finish(), std::runtime_error);
}

TEST_F(AircraftStateWriterTest, CombinesAircraftWithTheirIdentifiers) {
   AircraftStateWriter writer(true);
   writer.SetScenarioName((directory / "multi").string());
   writer.Gather({mitre::oss::simcore::AircraftState::Builder(1, Units::SecondsTime(0)).Build(),
                  mitre::oss::simcore::AircraftState::Builder(1, Units::SecondsTime(1)).Build()});
   writer.Gather({mitre::oss::simcore::AircraftState::Builder(2, Units::SecondsTime(60)).Build()});
   writer.Finish();
   const auto rows = ReadCsv(directory / "multi_AcStates.csv");
   ASSERT_EQ(rows.size(), 4);
   for (const auto &row : rows) ASSERT_EQ(row.size(), 11);
   EXPECT_EQ(rows[0][0], "acid");
   EXPECT_EQ(rows[0][1], "Time[sec]");
   EXPECT_EQ(rows[1][0], "1");
   EXPECT_DOUBLE_EQ(std::stod(rows[1][1]), 0);
   EXPECT_EQ(rows[2][0], "1");
   EXPECT_DOUBLE_EQ(std::stod(rows[2][1]), 1);
   EXPECT_EQ(rows[3][0], "2");
   EXPECT_DOUBLE_EQ(std::stod(rows[3][1]), 60);
}

TEST_F(AircraftStateWriterTest, PreservesSingleAircraftColumns) {
   AircraftStateWriter writer;
   writer.SetScenarioName((directory / "single").string());
   writer.Gather({mitre::oss::simcore::AircraftState::Builder(7, Units::SecondsTime(5)).Build()});
   writer.Finish();
   const auto rows = ReadCsv(directory / "single_AcStates.csv");
   ASSERT_EQ(rows.size(), 2);
   EXPECT_EQ(rows[0], (std::vector<std::string>{"Time[sec]", "V(ias)[m/s]", "V(tas)[m/s]", "vRate[m/s]", "x[m]", "y[m]",
                                                "h[m]", "gs[mps]", "latitude[deg]", "longitude[deg]"}));
   ASSERT_EQ(rows[1].size(), 10);
   EXPECT_DOUBLE_EQ(std::stod(rows[1][0]), 5);
}

}
