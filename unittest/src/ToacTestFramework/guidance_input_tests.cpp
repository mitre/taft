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

#include "framework/GuidanceDataLoader.h"
#include "framework/HfpReader2020.h"
#include "framework/WaypointSequenceReader.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
class GuidanceInputTest : public TemporaryDirectoryTest {
  protected:
   GuidanceConfiguration Guidance(const std::string &vertical) {
      GuidanceConfiguration config;
      config.hfp_csv_file = std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files/example-horizontal-path.csv";
      config.hfp_compute_xy = false;
      config.vfp_csv_file = Write("vertical.csv", vertical);
      return config;
   }
};

TEST_F(GuidanceInputTest, VerticalProfileRetainsFinalRowWithoutNewline) {
   const auto config = Guidance("t,d,h,ias,hdot,gs\n0,0,1000,100,0,100\n10,1000,2000,120,-5,130");
   const auto guidance = GuidanceDataLoader(config).BuildGuidanceCalculator();
   ASSERT_EQ(guidance->GetVerticalData().m_ias_mps.size(), 2);
   EXPECT_EQ(guidance->GetVerticalData().m_ias_mps.back(), 120);
}

TEST_F(GuidanceInputTest, VerticalProfileRejectsMalformedValues) {
   for (const auto &data : {"t,d,h,ias,hdot,gs\n0,0,bad,100,0,100\n1,1000,2000,120,0,120\n",
                            "t,d,h,ias,hdot,gs\n0,0,1000,100,0,100\n1,0,2000,120,0,120\n",
                            "t,d,h,ias,hdot,gs\n0,0,1000,nan,0,100\n1,1000,2000,120,0,120\n"}) {
      SCOPED_TRACE(data);
      const auto config = Guidance(data);
      EXPECT_THROW(GuidanceDataLoader(config).BuildGuidanceCalculator(), std::runtime_error);
   }
}

TEST_F(GuidanceInputTest, VerticalProfileRejectsMissingSamples) {
   const auto config = Guidance("t,d,h,ias,hdot,gs\n");
   EXPECT_THROW(GuidanceDataLoader(config).BuildGuidanceCalculator(), std::runtime_error);
}

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

TEST_F(GuidanceInputTest, HfpRejectsInsufficientWaypoints) {
   std::ifstream sample(std::filesystem::path(TAFT_SOURCE_DIR) / "Run_Files/example-horizontal-path.csv");
   std::string header;
   std::getline(sample, header);
   auto config = Guidance("t,d,h,ias,hdot,gs\n0,0,1000,100,0,100\n1,1000,2000,120,0,120\n");
   config.hfp_csv_file = Write("empty-hfp.csv", header + "\n");
   EXPECT_THROW(GuidanceDataLoader(config).BuildGuidanceCalculator(), std::runtime_error);
}

}
