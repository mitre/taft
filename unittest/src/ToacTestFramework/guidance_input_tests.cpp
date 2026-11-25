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
#include <stdexcept>
#include <string>

#include "framework/HfpReader2020.h"
#include "framework/WaypointSequenceReader.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
class GuidanceInputTest : public TemporaryDirectoryTest {};

TEST_F(GuidanceInputTest, WaypointsRequireAtLeastTwoRows) {
   for (const auto &data : {"lat,lon,radius,bank\n", "lat,lon,radius,bank\n33,-112,0,0\n"}) {
      SCOPED_TRACE(data);
      const auto filename = Write("waypoints.csv", data);
      EXPECT_THROW(WaypointSequenceReader(filename.string()).ReadFile(), std::runtime_error);
   }
}

TEST_F(GuidanceInputTest, HfpRejectsMissingHeadersAndMalformedNumbers) {
   const auto missing = Write("missing-hfp.csv", "x[m],y[m]\n0,0\n");
   EXPECT_THROW(HfpReader2020(missing.string(), 1), std::runtime_error);
   std::ifstream sample(std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files/example-horizontal-path.csv");
   std::string header;
   std::getline(sample, header);
   const auto malformed = Write("malformed-hfp.csv", header + "\n0,bad,0,0,straight,0,0,0,0,0,0,0,0,33,-112,0,0\n");
   HfpReader2020 reader(malformed.string(), 1);
   ASSERT_TRUE(reader.Advance());
   EXPECT_THROW(reader.GetX(), std::runtime_error);
}

}
