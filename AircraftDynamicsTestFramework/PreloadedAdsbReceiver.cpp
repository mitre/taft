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

#include "framework/PreloadedAdsbReceiver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "public/EarthModel.h"
#include "framework/NumericCsv.h"
#include "public/TvReader.h"

namespace mitre::oss::taft {

PreloadedAdsbReceiver::PreloadedAdsbReceiver(
      const std::string &traffic_csv_file,
      std::shared_ptr<mitre::oss::simcore::TangentPlaneSequence> tangent_plane_sequence)
   : m_traffic_filename(traffic_csv_file), m_tanget_plane_sequence(tangent_plane_sequence) {}

mitre::oss::simcore::ADSBSVReport PreloadedAdsbReceiver::GetCurrentADSBReport(int id) const {
   if (m_most_recent_adsb_reports.empty()) return mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT;
   const auto found = m_most_recent_adsb_reports.find(id);
   return found == m_most_recent_adsb_reports.end() ? mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT : found->second;
}

mitre::oss::simcore::ADSBSVReport PreloadedAdsbReceiver::GetADSBReportBefore(int id, Units::Time time) const {
   const auto found = m_all_reports_received.find(id);
   if (found == m_all_reports_received.end()) return mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT;
   const auto report =
         std::lower_bound(found->second.begin(), found->second.end(), time,
                          [](const auto &value, const Units::Time &timestamp) { return value.GetTime() < timestamp; });
   return report == found->second.begin() ? mitre::oss::simcore::ADSBSVReport::EMPTY_REPORT : *(report - 1);
}

const std::vector<mitre::oss::simcore::ADSBSVReport> &PreloadedAdsbReceiver::GetReportsReceivedByTime(
      const mitre::oss::simcore::SimulationTime &time) const {
   if (m_reports_received_by_time.empty() ||
       m_reports_received_by_time.find(time.GetCycle()) == m_reports_received_by_time.end()) {
      static std::vector<mitre::oss::simcore::ADSBSVReport> empty;
      return empty;
   }
   return m_reports_received_by_time.at(time.GetCycle());
}

const std::map<int, std::vector<mitre::oss::simcore::ADSBSVReport> > &PreloadedAdsbReceiver::GetAllReportsReceived()
      const {
   return m_all_reports_received;
}

std::map<int, mitre::oss::simcore::ADSBSVReport> const PreloadedAdsbReceiver::GetCurrentADSBReport() const {
   return m_most_recent_adsb_reports;
}

void PreloadedAdsbReceiver::Initialize(Units::Length adsb_reception_range_threshold) {
   m_all_reports_received.clear();
   m_reports_received_by_time.clear();
   m_most_recent_adsb_reports.clear();
   if (!m_tanget_plane_sequence) throw std::runtime_error("Traffic replay requires a tangent plane sequence");
   mitre::oss::simcore::TvReader data_reader(m_traffic_filename, 1);
   std::vector<std::size_t> numeric_columns{0};
   for (const auto &name : {"ACID", "TOAp[sec]", "Lat[degrees]", "Lon[degrees]", "Alt[feet]", "EWVel[knots]",
                            "NSVel[knots]", "TOAv[sec]", "NACp", "NIC", "NACv", "VertRate[fpm]"}) {
      bool found = false;
      for (std::size_t column = 0; column < data_reader.GetColumnCount(); ++column) {
         auto header = data_reader.GetString(column);
         if (!header.empty() && header.back() == '\r') header.pop_back();
         if (header == name) {
            numeric_columns.push_back(column);
            found = true;
            break;
         }
      }
      if (!found) throw std::runtime_error(m_traffic_filename + ": missing traffic column: " + name);
   }
   while (data_reader.Advance()) {
      for (const auto column : numeric_columns) {
         const double value = loader::ReadFiniteNumber(data_reader.GetString(column),
                                                       m_traffic_filename + ": column " + std::to_string(column + 1));
         if (column == numeric_columns[1] || column == numeric_columns[9] || column == numeric_columns[10] ||
             column == numeric_columns[11]) {
            if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max() ||
                std::trunc(value) != value)
               throw std::runtime_error(m_traffic_filename + ": invalid integer field");
         }
      }
      mitre::oss::simcore::EarthModel::GeodeticPosition geo_position;
      geo_position.latitude = data_reader.GetLat();
      geo_position.longitude = data_reader.GetLon();
      geo_position.altitude = data_reader.GetAlt();
      mitre::oss::simcore::EarthModel::LocalPositionEnu local_position;
      m_tanget_plane_sequence->ConvertGeodeticToLocal(geo_position, local_position);

      const auto report =
            mitre::oss::simcore::ADSBSVReport::Builder(data_reader.GetAcid(), data_reader.GetTimeOfReceipt())
                  .Position(local_position.x, local_position.y)
                  ->GeodeticPosition(geo_position.latitude, geo_position.longitude)
                  ->AltitudeMsl(geo_position.altitude)
                  ->GroundSpeed(data_reader.GetEwvel(), data_reader.GetNsvel())
                  ->AltitudeRate(data_reader.GetVertRate())
                  ->NACp(data_reader.GetNacp())
                  ->NACv(data_reader.GetNacv())
                  ->NICp(data_reader.GetNic())
                  ->NICv(data_reader.GetNic())
                  ->Build();

      m_all_reports_received[report.GetId()].push_back(report);
      const double cycle =
            std::round(data_reader.GetTimeOfReceipt() / mitre::oss::simcore::SimulationTime::GetSimulationTimeStep());
      if (!std::isfinite(cycle) || cycle < std::numeric_limits<int>::min() || cycle > std::numeric_limits<int>::max())
         throw std::runtime_error(m_traffic_filename + ": invalid report time");
      m_reports_received_by_time[static_cast<int>(cycle)].push_back(report);
   }
   for (auto &entry : m_all_reports_received)
      std::stable_sort(entry.second.begin(), entry.second.end(),
                       [](const auto &a, const auto &b) { return a.GetTime() < b.GetTime(); });
}

std::map<int, mitre::oss::simcore::ADSBSVReport> PreloadedAdsbReceiver::Receive(
      const mitre::oss::simcore::SimulationTime &time, const mitre::oss::simcore::AircraftState &state) {
   auto surveillance_reports = GetReportsReceivedByTime(time);
   if (surveillance_reports.empty()) return std::map<int, mitre::oss::simcore::ADSBSVReport>{};
   std::map<int, mitre::oss::simcore::ADSBSVReport> received;
   for (const auto &report : surveillance_reports) {
      const auto found = received.find(report.GetId());
      if (found == received.end() || found->second.GetTime() <= report.GetTime()) received[report.GetId()] = report;
   }
   for (const auto &entry : received) m_most_recent_adsb_reports[entry.first] = entry.second;
   return received;
}

}  // namespace mitre::oss::taft
