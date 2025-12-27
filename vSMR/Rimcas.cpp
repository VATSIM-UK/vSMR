#include "stdafx.h"
#include "Rimcas.hpp"
#include "Logger.h"

CRimcas::CRimcas() {}

CRimcas::~CRimcas() { Reset(); }

void CRimcas::Reset() {

  RunwayAreas.clear();
  AcColor.clear();
  AcOnRunway.clear();
  TimeTable.clear();
  MonitoredRunwayArr.clear();
  MonitoredRunwayDep.clear();
  ApproachingAircrafts.clear();
  DepartedAircraft.clear();
  AircraftWasOnGround.clear();
}

void CRimcas::OnRefreshBegin(bool isLVP) {

  AcColor.clear();
  AcOnRunway.clear();
  TimeTable.clear();
  ApproachingAircrafts.clear();
  this->IsLVP = isLVP;
}

void CRimcas::OnRefresh(CRadarTarget Rt, CRadarScreen *instance,
                        bool isCorrelated, string acType, string acStand) {

  GetAcInRunwayArea(Rt, instance);
  GetAcInRunwayAreaSoon(Rt, instance, isCorrelated, acType, acStand);
}

void CRimcas::AddRunwayArea(CRadarScreen *instance, string runway_name1,
                            string runway_name2, vector<CPosition> Definition) {

  string Name = runway_name1 + " / " + runway_name2;

  RunwayAreaType Runway;
  Runway.Name = Name;
  Runway.Definition = Definition;

  RunwayAreas[Name] = Runway;
}

string CRimcas::GetAcInRunwayArea(CRadarTarget Ac, CRadarScreen *instance) {

  int AltitudeDif = Ac.GetPosition().GetFlightLevel() -
                    Ac.GetPreviousPosition(Ac.GetPosition()).GetFlightLevel();
  if (!Ac.GetPosition().GetTransponderC())
    AltitudeDif = 0;

  if (Ac.GetGS() > 160 || AltitudeDif > 200)
    return string_false;

  POINT AcPosPix =
      instance->ConvertCoordFromPositionToPixel(Ac.GetPosition().GetPosition());

  for (std::map<string, RunwayAreaType>::iterator it = RunwayAreas.begin();
       it != RunwayAreas.end(); ++it) {
    if (!MonitoredRunwayDep[string(it->first)])
      continue;

    vector<POINT> RunwayOnScreen;

    for (auto &Point : it->second.Definition) {
      RunwayOnScreen.push_back(
          instance->ConvertCoordFromPositionToPixel(Point));
    }

    if (Is_Inside(AcPosPix, RunwayOnScreen)) {
      AcOnRunway.insert(std::pair<string, string>(it->first, Ac.GetCallsign()));
      return string(it->first);
    }
  }

  return string_false;
}

string CRimcas::GetAcInRunwayAreaSoon(CRadarTarget Ac, CRadarScreen *instance,
                                      bool isCorrelated, string acType,
                                      string acStand) {

  int AltitudeDif = Ac.GetPosition().GetFlightLevel() -
                    Ac.GetPreviousPosition(Ac.GetPosition()).GetFlightLevel();
  if (!Ac.GetPosition().GetTransponderC())
    AltitudeDif = 0;

  // Making sure the AC is airborne and not climbing, but below transition
  if (Ac.GetGS() < 50 || AltitudeDif > 50 ||
      Ac.GetPosition().GetPressureAltitude() >
          instance->GetPlugIn()->GetTransitionAltitude())
    return string_false;

  // If the AC is already on the runway, then there is no point in this step
  if (isAcOnRunway(Ac.GetCallsign()))
    return string_false;

  POINT AcPosPix =
      instance->ConvertCoordFromPositionToPixel(Ac.GetPosition().GetPosition());

  for (std::map<string, RunwayAreaType>::iterator it = RunwayAreas.begin();
       it != RunwayAreas.end(); ++it) {
    if (!MonitoredRunwayArr[it->first])
      continue;

    // We need to know when and if the AC is going to enter the runway within 5
    // minutes (by steps of 10 seconds

    vector<POINT> RunwayOnScreen;

    for (auto &Point : it->second.Definition) {
      RunwayOnScreen.push_back(
          instance->ConvertCoordFromPositionToPixel(Point));
    }

    double AcSpeed = Ac.GetPosition().GetReportedGS();
    if (!Ac.GetPosition().GetTransponderC())
      AcSpeed = Ac.GetGS();

    for (int t = 5; t <= 300; t += 5) {
      double distance = Ac.GetPosition().GetReportedGS() * 0.514444 * t;

      // We tolerate up 2 degree variations to the runway at long range (> 120
      // s) And 3 degrees after (<= 120 t)

      bool isGoingToLand = false;
      int AngleMin = -2;
      int AngleMax = 2;
      if (t <= 120) {
        AngleMin = -3;
        AngleMax = 3;
      }

      for (int a = AngleMin; a <= AngleMax; a++) {
        POINT PredictedPosition = instance->ConvertCoordFromPositionToPixel(
            BetterHarversine(Ac.GetPosition().GetPosition(),
                             fmod(Ac.GetTrackHeading() + a, 360), distance));
        isGoingToLand = Is_Inside(PredictedPosition, RunwayOnScreen);

        if (isGoingToLand)
          break;
      }

      if (isGoingToLand) {
        // The aircraft is going to be on the runway, we need to decide where it
        // needs to be shown on the AIW
        bool first = true;
        vector<int> Definiton = CountdownDefinition;
        if (IsLVP)
          Definiton = CountdownDefinitionLVP;
        for (size_t k = 0; k < Definiton.size(); k++) {
          int Time = Definiton.at(k);

          int PreviousTime = 0;
          if (first) {
            PreviousTime = Time + 15;
            first = false;
          } else {
            PreviousTime = Definiton.at(k - 1);
          }
          if (t < PreviousTime && t >= Time) {
            AircraftInfo info;
            info.callsign = Ac.GetCallsign();
            info.type = acType;
            info.stand = acStand;
            TimeTable[it->first][Time] = info;
            break;
          }
        }

        // If the AC is xx seconds away from the runway, we consider him on it

        int StageTwoTrigger = 20;
        if (IsLVP)
          StageTwoTrigger = 30;

        if (t <= StageTwoTrigger)
          AcOnRunway.insert(
              std::pair<string, string>(it->first, Ac.GetCallsign()));

        // If the AC is 45 seconds away from the runway, we consider him
        // approaching

        if (t > StageTwoTrigger && t <= 45)
          ApproachingAircrafts.insert(
              std::pair<string, string>(it->first, Ac.GetCallsign()));

        return Ac.GetCallsign();
      }
    }
  }

  return CRimcas::string_false;
}

vector<CPosition> CRimcas::GetRunwayArea(CPosition Left, CPosition Right,
                                         float hwidth) {

  vector<CPosition> out;

  double RunwayBearing = RadToDeg(TrueBearing(Left, Right));

  out.push_back(BetterHarversine(Left, fmod(RunwayBearing + 90, 360),
                                 hwidth)); // Bottom Left
  out.push_back(BetterHarversine(Right, fmod(RunwayBearing + 90, 360),
                                 hwidth)); // Bottom Right
  out.push_back(BetterHarversine(Right, fmod(RunwayBearing - 90, 360),
                                 hwidth)); // Top Right
  out.push_back(BetterHarversine(Left, fmod(RunwayBearing - 90, 360),
                                 hwidth)); // Top Left

  return out;
}

void CRimcas::OnRefreshEnd(CRadarScreen *instance, int threshold) {

  for (map<string, RunwayAreaType>::iterator it = RunwayAreas.begin();
       it != RunwayAreas.end(); ++it) {

    if (!MonitoredRunwayArr[string(it->first)] &&
        !MonitoredRunwayDep[string(it->first)])
      continue;

    bool isOnClosedRunway = false;
    if (ClosedRunway.find(it->first) != ClosedRunway.end()) {
      if (ClosedRunway[it->first])
        isOnClosedRunway = true;
    }

    bool isAnotherAcApproaching = ApproachingAircrafts.count(it->first) > 0;

    if (AcOnRunway.count(it->first) > 1 || isOnClosedRunway ||
        isAnotherAcApproaching) {

      auto AcOnRunwayRange = AcOnRunway.equal_range(it->first);

      for (map<string, string>::iterator it2 = AcOnRunwayRange.first;
           it2 != AcOnRunwayRange.second; ++it2) {
        if (isOnClosedRunway) {
          AcColor[it2->second] = StageTwo;
        } else {
          if (instance->GetPlugIn()
                  ->RadarTargetSelect(it2->second.c_str())
                  .GetGS() > threshold) {
            // If the aircraft is on the runway and stage two, we check if
            // the aircraft is going towards any aircraft thats on the runway
            // if not, we don't display the warning
            bool triggerStageTwo = false;
            CRadarTarget rd1 =
                instance->GetPlugIn()->RadarTargetSelect(it2->second.c_str());
            CRadarTargetPositionData currentRd1 = rd1.GetPosition();
            for (map<string, string>::iterator it3 = AcOnRunwayRange.first;
                 it3 != AcOnRunwayRange.second; ++it3) {
              CRadarTarget rd2 =
                  instance->GetPlugIn()->RadarTargetSelect(it3->second.c_str());

              double currentDist = currentRd1.GetPosition().DistanceTo(
                  rd2.GetPosition().GetPosition());
              double oldDist =
                  rd1.GetPreviousPosition(currentRd1)
                      .GetPosition()
                      .DistanceTo(rd2.GetPreviousPosition(rd2.GetPosition())
                                      .GetPosition());

              if (currentDist < oldDist) {
                triggerStageTwo = true;
                break;
              }
            }

            if (triggerStageTwo)
              AcColor[it2->second] = StageTwo;
          } else {
            AcColor[it2->second] = StageOne;
          }
        }
      }

      for (auto &ac : ApproachingAircrafts) {
        if (ac.first == it->first && AcOnRunway.count(it->first) > 1)
          AcColor[ac.second] = StageOne;

        if (ac.first == it->first && isOnClosedRunway)
          AcColor[ac.second] = StageTwo;
      }
    }
  }
}

bool CRimcas::isAcOnRunway(string callsign) {

  for (std::map<string, string>::iterator it = AcOnRunway.begin();
       it != AcOnRunway.end(); ++it) {
    if (it->second == callsign)
      return true;
  }

  return false;
}

CRimcas::RimcasAlertTypes CRimcas::getAlert(string callsign) {

  if (AcColor.find(callsign) == AcColor.end())
    return NoAlert;

  return AcColor[callsign];
}

Color CRimcas::GetAircraftColor(string AcCallsign, Color StandardColor,
                                Color OnRunwayColor, Color RimcasStageOne,
                                Color RimcasStageTwo) {

  if (AcColor.find(AcCallsign) == AcColor.end()) {
    if (isAcOnRunway(AcCallsign)) {
      return OnRunwayColor;
    } else {
      return StandardColor;
    }
  } else {
    if (AcColor[AcCallsign] == StageOne) {
      return RimcasStageOne;
    } else {
      return RimcasStageTwo;
    }
  }
}

Color CRimcas::GetAircraftColor(string AcCallsign, Color StandardColor,
                                Color OnRunwayColor) {

  if (isAcOnRunway(AcCallsign)) {
    return OnRunwayColor;
  } else {
    return StandardColor;
  }
}

void CRimcas::TrackDeparture(CRadarTarget Rt, CFlightPlan fp,
                             CRadarScreen *instance, string activeAirport) {
  if (!Rt.IsValid() || !fp.IsValid())
    return;

  string callsign = Rt.GetCallsign();

  string origin = fp.GetFlightPlanData().GetOrigin();
  if (origin != activeAirport) {
    return;
  }

  // Check if aircraft is actually in a runway area (using same logic as GetAcInRunwayArea)
  string runwayArea = GetAcInRunwayArea(Rt, instance);
  bool onRunway = (runwayArea != string_false);

  if (onRunway && DepartedAircraft.find(callsign) == DepartedAircraft.end()) {
    DepartureInfo info;
    info.callsign = callsign;
    if (info.callsign.length() > 8) {
      info.callsign = info.callsign.substr(0, 8);
    }
    
    info.sid = fp.GetFlightPlanData().GetSidName();
    if (info.sid.length() > 7) {
      info.sid = info.sid.substr(0, 7);
    }
    
    info.acType = fp.GetFlightPlanData().GetAircraftFPType();
    if (info.acType.size() > 4) {
      info.acType = info.acType.substr(0, 4);
    }
    info.wakeTurbCat = "";
    info.wakeTurbCat += fp.GetFlightPlanData().GetAircraftWtc();

    info.airborneFreq = fp.GetControllerAssignedData().GetFlightStripAnnotation(8);
    if (info.airborneFreq.length() == 0) {
      info.airborneFreq = "QSY";
    }
    if (info.airborneFreq.length() > 7) {
      info.airborneFreq = info.airborneFreq.substr(0, 7);
    }

    info.groundAltitude = Rt.GetPosition().GetPressureAltitude();
    info.liftoffTime = 0;
    info.timerStarted = false;
    info.dismissed = false;

    DepartedAircraft[callsign] = info;
  }

  // Update and start timer once aircraft is airborne
  if (DepartedAircraft.find(callsign) != DepartedAircraft.end()) {
    DepartureInfo &info = DepartedAircraft[callsign];
    int currentAltitude = Rt.GetPosition().GetPressureAltitude();
    
    // Remove aircraft if above 6000 feet
    if (currentAltitude > 6000) {
      Logger::info("TrackDeparture: Removing " + callsign + " - above 6000ft (alt=" + std::to_string(currentAltitude) + ")");
      DepartedAircraft.erase(callsign);
      return;
    }
    
    if (!info.timerStarted) {
      int verticalSpeed = Rt.GetVerticalSpeed();
      
      // Start timer once airborne: climbing above ground altitude with > 100 fpm climb rate
      if (currentAltitude > info.groundAltitude && verticalSpeed > 100) {
        info.liftoffTime = clock();
        info.timerStarted = true;
        Logger::info("TrackDeparture: Timer started for " + callsign + " - airborne (alt=" + std::to_string(currentAltitude) + ", vs=" + std::to_string(verticalSpeed) + ")");
      }
    }
  }
}

void CRimcas::UpdateDepartureTimer(int departureDisplayDurationSecs, CRadarScreen *instance) {


  clock_t currentTime = clock();
  double maxDurationSecs = (double)departureDisplayDurationSecs;

  vector<string> toRemove;
  for (auto &pair : DepartedAircraft) {
    if (pair.second.dismissed) {
      toRemove.push_back(pair.first);
      continue;
    }

    // Remove if above 6000 feet
    CRadarTarget rt = instance->GetPlugIn()->RadarTargetSelect(pair.first.c_str());
    if (rt.IsValid() && rt.GetPosition().GetPressureAltitude() > 6000) {
      toRemove.push_back(pair.first);
      continue;
    }

    // Only check timer expiry if timer has actually started
    if (pair.second.timerStarted) {
      double elapsedSecs =
          (double)(currentTime - pair.second.liftoffTime) / CLOCKS_PER_SEC;
      if (elapsedSecs >= maxDurationSecs) {
        toRemove.push_back(pair.first);
      }
    }
  }

  for (const string &callsign : toRemove) {
    DepartedAircraft.erase(callsign);
  }
}

void CRimcas::DismissDeparture(string callsign) {
  if (DepartedAircraft.find(callsign) != DepartedAircraft.end()) {
    DepartedAircraft[callsign].dismissed = true;
  }
}

void CRimcas::ClearDismissedDepartures() {


  vector<string> toRemove;
  for (auto &pair : DepartedAircraft) {
    if (pair.second.dismissed) {
      toRemove.push_back(pair.first);
    }
  }

  for (const string &callsign : toRemove) {
    DepartedAircraft.erase(callsign);
  }
}
