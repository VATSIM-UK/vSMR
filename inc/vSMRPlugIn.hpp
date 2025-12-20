#pragma once

#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include <string>

class vSMRPlugIn : public EuroScopePlugIn::CPlugIn {
public:
  vSMRPlugIn();
  ~vSMRPlugIn();

  void DisplayMessage(const std::string &message,
                      const std::string &sender = "vSMRPlugIn");

  virtual bool OnCompileCommand(const char *sComandLine);
  virtual EuroScopePlugIn::CRadarScreen *
  OnRadarScreenCreated(const char *displayName, bool needRadarContent,
                       bool geoReferenced, bool canBeSaved, bool canBeCreated);
  virtual void OnFunctionCall(int functionId, const char *itemString, POINT pt,
                              RECT area);
  virtual void OnGetTagItem(EuroScopePlugIn::CFlightPlan flightPlan,
                            EuroScopePlugIn::CRadarTarget radarTarget,
                            int itemCode, int tagData, char itemString[16],
                            int *colourCode, COLORREF *pRGB, double *fontSize);
  virtual void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan flightPlan);
  virtual void OnTimer(int counter);
};
