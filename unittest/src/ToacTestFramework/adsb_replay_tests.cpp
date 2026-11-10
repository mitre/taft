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

#include <list>
#include <memory>
#include <stdexcept>
#include <string>

#include "framework/PreloadedAdsbReceiver.h"
#include "public/SingleTangentPlaneSequence.h"
#include "test_support.h"

namespace mitre::oss::taft::test {
class AdsbReplayTest : public TemporaryDirectoryTest {
  protected:
   std::shared_ptr<mitre::oss::simcore::TangentPlaneSequence> TangentPlane() {
      return std::make_shared<mitre::oss::simcore::SingleTangentPlaneSequence>(
            std::list<mitre::oss::simcore::Waypoint>{{"A", Units::DegreesAngle(33), Units::DegreesAngle(-112)},
                                                     {"B", Units::DegreesAngle(34), Units::DegreesAngle(-111)}});
   }
   std::string TrafficRow(int id, double time) {
      return std::to_string(time) + "," + std::to_string(id) + ",0,33,-112,10000,200,0,0,9,9,2,0\n";
   }
   const std::string traffic_header =
         "tRec[sec],ACID,TOAp[sec],Lat[degrees],Lon[degrees],Alt[feet],EWVel[knots],NSVel[knots],TOAv[sec],NACp,NIC,"
         "NACv,VertRate[fpm]\n";
};

TEST_F(AdsbReplayTest, KeepsEveryAircraftInACycle) {
   const auto filename = Write("traffic.csv", traffic_header + TrafficRow(1, 2) + TrafficRow(2, 2));
   PreloadedAdsbReceiver receiver(filename.string(), TangentPlane());
   receiver.Initialize(Units::NauticalMilesLength(90));
   const auto reports = receiver.Receive(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(2)), {});
   ASSERT_EQ(reports.size(), 2);
   EXPECT_EQ(receiver.GetCurrentADSBReport(2).GetId(), 2);
}

TEST_F(AdsbReplayTest, ReturnsEmptyForUnknownAircraftAndClearsOnReload) {
   const auto filename = Write("traffic.csv", traffic_header + TrafficRow(1, 2));
   PreloadedAdsbReceiver receiver(filename.string(), TangentPlane());
   receiver.Initialize(Units::NauticalMilesLength(90));
   receiver.Receive(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(2)), {});
   EXPECT_EQ(receiver.GetCurrentADSBReport(999).GetId(), mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT.GetId());
   receiver.Initialize(Units::NauticalMilesLength(90));
   EXPECT_EQ(receiver.GetAllReportsReceived().at(1).size(), 1);
   EXPECT_TRUE(receiver.GetCurrentADSBReport().empty());
}

TEST_F(AdsbReplayTest, UsesSimulationTimeStepAndQueriesHistory) {
   const auto previous_step = mitre::oss::simcore::SimulationTime::GetSimulationTimeStep();
   struct RestoreStep {
      Units::Time step;
      ~RestoreStep() { mitre::oss::simcore::SimulationTime::SetSimulationTimeStep(step); }
   } restore{previous_step};
   mitre::oss::simcore::SimulationTime::SetSimulationTimeStep(Units::SecondsTime(0.5));
   const auto filename =
         Write("traffic.csv", traffic_header + TrafficRow(1, 2) + TrafficRow(1, 1.5) + TrafficRow(2, 2));
   PreloadedAdsbReceiver receiver(filename.string(), TangentPlane());
   receiver.Initialize(Units::NauticalMilesLength(90));
   const auto reports = receiver.Receive(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(2)), {});
   EXPECT_EQ(reports.size(), 2);
   EXPECT_DOUBLE_EQ(receiver.GetADSBReportBefore(1, Units::SecondsTime(2)).GetTime().value(), 1.5);
   EXPECT_EQ(receiver.GetADSBReportBefore(1, Units::SecondsTime(1.5)).GetId(),
             mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT.GetId());
   EXPECT_EQ(receiver.GetADSBReportBefore(999, Units::SecondsTime(3)).GetId(),
             mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT.GetId());
   EXPECT_TRUE(receiver.Receive(mitre::oss::simcore::SimulationTime::Of(Units::SecondsTime(3)), {}).empty());
   EXPECT_EQ(receiver.GetCurrentADSBReport(2).GetId(), 2);
}

TEST_F(AdsbReplayTest, RejectsMalformedNumericFields) {
   const auto filename = Write("traffic.csv", traffic_header + "2,bad,0,33,-112,10000,200,0,0,9,9,2,0\n");
   PreloadedAdsbReceiver receiver(filename.string(), TangentPlane());
   EXPECT_THROW(receiver.Initialize(Units::NauticalMilesLength(90)), std::runtime_error);
}

}
