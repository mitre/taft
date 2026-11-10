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

#include "framework/SpeedCommandsFromStaticData.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace mitre::oss::taft {

SpeedCommandsFromStaticData::SpeedCommandsFromStaticData() : m_speed_data(), m_pilot_delay_seconds(0) {}

SpeedCommandsFromStaticData::SpeedCommandsFromStaticData(
      const std::vector<SpeedCommandsFromStaticData::SpeedRecord> &speed_data, Units::Time pilot_delay_duration)
   : m_speed_data(speed_data), m_pilot_delay_seconds(pilot_delay_duration) {
   if (!std::isfinite(m_pilot_delay_seconds.value()) || m_pilot_delay_seconds < Units::ZERO_TIME)
      throw std::invalid_argument("Pilot delay must be finite and nonnegative");
   if (m_speed_data.size() < 2) throw std::invalid_argument("Speed commands require at least two samples");
   for (std::size_t i = 0; i < m_speed_data.size(); ++i) {
      const auto &record = m_speed_data[i];
      if (!std::isfinite(record.simtime.value()) || !std::isfinite(record.speed_command.value()) ||
          record.speed_command <= Units::ZERO_SPEED)
         throw std::invalid_argument("Speed command samples require finite times and positive finite speeds");
      if (i > 0 && record.simtime <= m_speed_data[i - 1].simtime)
         throw std::invalid_argument("Speed command times must be strictly increasing");
   }
}

mitre::oss::simcore::Guidance SpeedCommandsFromStaticData::Update(Units::Time time) {
   mitre::oss::simcore::Guidance guidance;
   guidance.SetValid(false);

   const Units::Time lookup_time = time - m_pilot_delay_seconds;
   if (!std::isfinite(Units::SecondsTime(lookup_time).value()) || m_speed_data.empty() ||
       lookup_time < m_speed_data.front().simtime || lookup_time > m_speed_data.back().simtime)
      return guidance;

   const auto upper =
         std::lower_bound(m_speed_data.begin(), m_speed_data.end(), lookup_time,
                          [](const SpeedRecord &record, const Units::Time &value) { return record.simtime < value; });
   if (upper->simtime == lookup_time) {
      guidance.m_ias_command = upper->speed_command;
   } else {
      const auto &lower = *(upper - 1);
      const double fraction = Units::SecondsTime(lookup_time - lower.simtime).value() /
                              Units::SecondsTime(upper->simtime - lower.simtime).value();
      guidance.m_ias_command = (1.0 - fraction) * lower.speed_command + fraction * upper->speed_command;
   }
   guidance.SetSelectedSpeedType(SpeedValueType::INDICATED_AIR_SPEED);
   guidance.SetValid(true);

   return guidance;
}

void SpeedCommandsFromStaticData::Initialize(
      mitre::oss::simcore::FlightDeckApplicationInitializer &initializer_visitor) {}

mitre::oss::simcore::Guidance SpeedCommandsFromStaticData::Update(
      const mitre::oss::simcore::SimulationTime &simtime, const mitre::oss::simcore::Guidance &current_guidance,
      const mitre::oss::simcore::DynamicsState &dynamics_state, const mitre::oss::simcore::AircraftState &own_state) {
   return Update(simtime.GetCurrentSimulationTime());
}

bool SpeedCommandsFromStaticData::IsActive() const { return !m_speed_data.empty(); }

}  // namespace mitre::oss::taft
