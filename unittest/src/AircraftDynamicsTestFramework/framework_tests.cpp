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

#include "gtest/gtest.h"

#include <vector>

#include "framework/GuidanceFromStaticData.h"

namespace mitre::oss::taft::test {

class StaticGuidanceTest : public ::testing::Test {
  protected:
   GuidanceFromStaticData CreateGuidance(double cruise_mach = 0.78) {
      std::vector<mitre::oss::simcore::HorizontalPath> horizontal_path;
      for (double distance : {0.0, 1000.0, 2000.0}) {
         mitre::oss::simcore::HorizontalPath segment;
         segment.SetXYPositionMeters(distance, 0);
         segment.m_path_length_cumulative_meters = distance;
         segment.m_path_course = 0;
         segment.m_segment_type = mitre::oss::simcore::HorizontalPath::STRAIGHT;
         horizontal_path.push_back(segment);
      }

      GuidanceFromStaticData::VerticalData vertical_path;
      vertical_path.m_time_to_go_sec = {0, 10, 20, 30};
      vertical_path.m_distance_to_go_meters = {0, 1000, 2000, 3000};
      vertical_path.m_altitude_meters = {1000, 2000, 3000, 4000};
      vertical_path.m_ias_mps = {100, 120, 140, 160};
      vertical_path.m_vertical_speed_mps = {-3, -4, -5, -6};
      vertical_path.m_ground_speed_mps = {110, 130, 150, 170};

      GuidanceFromStaticData::PlannedDescentParameters parameters;
      parameters.planned_transition_altitude = Units::MetersLength(2000);
      parameters.planned_transition_ias = Units::MetersPerSecondSpeed(120);
      parameters.planned_cruise_mach = cruise_mach;
      return GuidanceFromStaticData(horizontal_path, vertical_path, parameters);
   }

   mitre::oss::simcore::AircraftState CreateState(double distance, double altitude) {
      return mitre::oss::simcore::AircraftState::Builder(1, Units::SecondsTime(0))
            .Position(Units::MetersLength(distance), Units::MetersLength(0))
            ->AltitudeMsl(Units::MetersLength(altitude))
            ->GroundSpeed(Units::MetersPerSecondSpeed(140), Units::MetersPerSecondSpeed(0))
            ->Build();
   }
};

TEST_F(StaticGuidanceTest, InterpolatesIasBelowTransitionAltitude) {
   auto calculator = CreateGuidance();
   const auto guidance = calculator.Update(CreateState(1500, 1999));

   EXPECT_TRUE(guidance.IsValid());
   EXPECT_EQ(guidance.GetSelectedSpeedType(), SpeedValueType::INDICATED_AIR_SPEED);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(guidance.m_ias_command).value(), 130, 1e-9);
   EXPECT_NEAR(Units::MetersLength(guidance.m_reference_altitude).value(), 2500, 1e-9);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(guidance.m_vertical_speed).value(), -4.5, 1e-9);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(guidance.m_ground_speed).value(), 140, 1e-9);
}

TEST_F(StaticGuidanceTest, SelectsPlannedMachAtAndAboveTransitionAltitude) {
   for (double altitude : {2000.0, 2001.0}) {
      auto calculator = CreateGuidance();
      const auto guidance = calculator.Update(CreateState(1500, altitude));

      EXPECT_TRUE(guidance.IsValid());
      EXPECT_EQ(guidance.GetSelectedSpeedType(), SpeedValueType::MACH_SPEED);
      EXPECT_DOUBLE_EQ(guidance.GetMachCommand(), 0.78);
      EXPECT_NEAR(Units::MetersPerSecondSpeed(guidance.m_ias_command).value(), 130, 1e-9);
   }
}

TEST_F(StaticGuidanceTest, PreservesProfileValuesAtSamples) {
   for (double distance : {0.0, 1000.0, 2000.0}) {
      auto calculator = CreateGuidance();
      const auto guidance = calculator.Update(CreateState(distance, 1999));

      EXPECT_TRUE(guidance.IsValid());
      EXPECT_NEAR(Units::MetersPerSecondSpeed(guidance.m_ias_command).value(), 100 + distance / 50, 1e-9);
      EXPECT_NEAR(Units::MetersLength(guidance.m_reference_altitude).value(), 1000 + distance, 1e-9);
   }
}

TEST_F(StaticGuidanceTest, RejectsMachOutsideExistingBounds) {
   for (double mach : {-0.01, 1.01}) {
      auto calculator = CreateGuidance(mach);
      EXPECT_THROW(calculator.Update(CreateState(1500, 2000)), BoundedValueException<double>);
   }
}

TEST_F(StaticGuidanceTest, AcceptsMachAtExistingBounds) {
   for (double mach : {0.0, 1.0}) {
      auto calculator = CreateGuidance(mach);
      const auto guidance = calculator.Update(CreateState(1500, 2000));

      EXPECT_EQ(guidance.GetSelectedSpeedType(), SpeedValueType::MACH_SPEED);
      EXPECT_DOUBLE_EQ(guidance.GetMachCommand(), mach);
   }
}

TEST_F(StaticGuidanceTest, ReusesGuidanceUntilDistanceDecreases) {
   auto calculator = CreateGuidance();
   const auto previous = calculator.Update(CreateState(1500, 1999));
   for (double distance : {1500.0, 1600.0}) {
      const auto guidance = calculator.Update(CreateState(distance, 2001));

      EXPECT_EQ(guidance.IsValid(), previous.IsValid());
      EXPECT_EQ(guidance.GetSelectedSpeedType(), previous.GetSelectedSpeedType());
      EXPECT_EQ(guidance.m_ias_command, previous.m_ias_command);
      EXPECT_EQ(guidance.GetMachCommand(), previous.GetMachCommand());
      EXPECT_EQ(guidance.m_reference_altitude, previous.m_reference_altitude);
      EXPECT_NEAR(calculator.GetEstimatedDistanceAlongPath().value(), 1500, 1e-9);
   }

   const auto advanced = calculator.Update(CreateState(1400, 2001));
   EXPECT_EQ(advanced.GetSelectedSpeedType(), SpeedValueType::MACH_SPEED);
   EXPECT_DOUBLE_EQ(advanced.GetMachCommand(), 0.78);
   EXPECT_NEAR(Units::MetersPerSecondSpeed(advanced.m_ias_command).value(), 128, 1e-9);
}

}
