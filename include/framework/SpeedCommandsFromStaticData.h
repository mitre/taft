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

#include <scalar/Speed.h>
#include <scalar/Time.h>

#include <vector>

#include "public/FlightDeckApplication.h"
#include "public/Guidance.h"

namespace mitre::oss::taft {

class SpeedCommandsFromStaticData final : public mitre::oss::simcore::FlightDeckApplication {
  public:
   struct SpeedRecord {
      SpeedRecord() : simtime(Units::infinity()), speed_command(Units::infinity()) {}
      Units::SecondsTime simtime;
      Units::MetersPerSecondSpeed speed_command;
   };
   SpeedCommandsFromStaticData();
   SpeedCommandsFromStaticData(const std::vector<SpeedCommandsFromStaticData::SpeedRecord> &speed_data,
                               Units::Time pilot_delay_duration);
   ~SpeedCommandsFromStaticData() = default;

   void Initialize(mitre::oss::simcore::FlightDeckApplicationInitializer &initializer_visitor) override;

   mitre::oss::simcore::Guidance Update(const mitre::oss::simcore::SimulationTime &simtime,
                                        const mitre::oss::simcore::Guidance &current_guidance,
                                        const mitre::oss::simcore::DynamicsState &dynamics_state,
                                        const mitre::oss::simcore::AircraftState &own_state) override;

   bool IsActive() const override;

  private:
   mitre::oss::simcore::Guidance Update(Units::Time time);

   std::vector<SpeedRecord> m_speed_data;
   Units::SecondsTime m_pilot_delay_seconds;
};

}  // namespace mitre::oss::taft
