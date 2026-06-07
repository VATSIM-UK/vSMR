#include "stdafx.h"
#include "SMRPlugin.hpp"

bool Logger::ENABLED;
string Logger::DLL_PATH;

bool HoppieConnected = false;
bool ConnectionMessage = false;
bool FailedToConnectMessage = false;

string logonCode = "";
string logonCallsign = "EGKK";

HttpHelper * httpHelper = NULL;

bool BLINK = false;

bool PlaySoundClr = false;

string baseUrlDatalink = "http://www.hoppie.nl/acars/system/connect.html";

clock_t timer;
int pollInterval = 45 + rand() % 31; // Random interval between 45-75 seconds

string myfrequency;

map<string, string> vStrips_Stands;

bool startThreadvStrips = true;

using namespace SMRPluginSharedData;
char recv_buf[1024];

vector<CSMRRadar*> RadarScreensOpened;

static void formatNowTimeDate(string & timeStr, string &dateStr)
{
	time_t now = time(NULL);
	tm tmnow;
	gmtime_s(&tmnow, &now); // use UTC
	char buf[64];
	strftime(buf, sizeof(buf), "%H%MZ", &tmnow);
	timeStr = buf;
	strftime(buf, sizeof(buf), "%d%m%y", &tmnow);
	dateStr = buf;
}


CSMRPlugin::CSMRPlugin(void) :CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT)
{

	Logger::DLL_PATH = "";
	Logger::ENABLED = false;

	//
	// Adding the SMR Display type
	//
	RegisterDisplayType(MY_PLUGIN_VIEW_AVISO, false, true, true, true);

	RegisterTagItemType("Datalink clearance", TAG_ITEM_DATALINK_STS);
	RegisterTagItemFunction("Datalink menu", TAG_FUNC_DATALINK_MENU);

	RegisterTagItemType("Acknowledged QNH", TAG_ITEM_QNH);
	RegisterTagItemFunction("Update acknowledged QNH", TAG_FUNC_QNH_UPDATE);
	RegisterTagItemFunction("Reset acknowledged QNH", TAG_FUNC_QNH_RESET);

	timer = clock();

	if (httpHelper == NULL)
		httpHelper = new HttpHelper();

	// Initialize DatalinkManager
	datalinkManager = new DatalinkManager(httpHelper, baseUrlDatalink);

	const char * p_value;

	if ((p_value = GetDataFromSettings("cpdlc_logon")) != NULL)
		logonCallsign = p_value;
	if ((p_value = GetDataFromSettings("cpdlc_password")) != NULL)
		logonCode = p_value;
	if ((p_value = GetDataFromSettings("cpdlc_sound")) != NULL)
		PlaySoundClr = bool(!!atoi(p_value));

	// Set credentials in DatalinkManager
	datalinkManager->setCredentials(logonCode, logonCallsign);

	char DllPathFile[_MAX_PATH];
	string DllPath;

	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	DllPath = DllPathFile;
	DllPath.resize(DllPath.size() - strlen("vSMR.dll"));
	Logger::DLL_PATH = DllPath;

	// Start UKCP integration socket client
	if (SMRPluginSharedData::ukcpIntegration == nullptr) {
		SMRPluginSharedData::ukcpIntegration = new UKCPIntegration();
	}
	SMRPluginSharedData::ukcpIntegration->Start();
}

CSMRPlugin::~CSMRPlugin()
{
	// NOTE: 'SaveDataToSettings()' doesn't actually write data anywhere in a file, contrary to what the name freaking suggests.
	SaveDataToSettings("cpdlc_logon", "The CPDLC logon callsign", logonCallsign.c_str());
	SaveDataToSettings("cpdlc_password", "The CPDLC logon password", logonCode.c_str());
	int temp = 0;
	if (PlaySoundClr)
		temp = 1;
	SaveDataToSettings("cpdlc_sound", "Play sound on clearance request", std::to_string(temp).c_str());

	// Clean up DatalinkManager
	if (datalinkManager != nullptr) {
		delete datalinkManager;
		datalinkManager = nullptr;
	}

	try
	{
		io_service.stop();
		//vStripsThread.join();

		// Stop UKCP integration
		if (SMRPluginSharedData::ukcpIntegration != nullptr) {
			SMRPluginSharedData::ukcpIntegration->Stop();
			delete SMRPluginSharedData::ukcpIntegration;
			SMRPluginSharedData::ukcpIntegration = nullptr;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

bool CSMRPlugin::OnCompileCommand(const char * sCommandLine) {
	if (startsWith(".smr connect", sCommandLine))
	{
		if (ControllerMyself().IsController()) {
			if (!HoppieConnected) {
				if (datalinkManager != nullptr && datalinkManager->login(logonCode, logonCallsign)) {
					HoppieConnected = true;
					ConnectionMessage = true;
				}
				else {
					FailedToConnectMessage = true;
				}
			}
			else {
				HoppieConnected = false;
				DisplayUserMessage("CPDLC", "Server", "Logged off!", true, true, false, true, false);
			}
		}
		else {
			DisplayUserMessage("CPDLC", "Error", "You are not logged in as a controller!", true, true, false, true, false);
		}

		return true;
	}
	else if (startsWith(".smr poll", sCommandLine))
	{
		if (HoppieConnected && datalinkManager != nullptr) {
			datalinkManager->pollMessages();
		}
		return true;
	}
	else if (strcmp(sCommandLine, ".smr resetvisuals") == 0) {
		for (auto radarScreen : RadarScreensOpened) {
			if (radarScreen != nullptr) {
				radarScreen->ResetMonitorVisuals();
			}
		}
		return true;
	}
	else if (strcmp(sCommandLine, ".smr log") == 0) {
		Logger::ENABLED = !Logger::ENABLED;
		return true;
	}
	else if (startsWith(".smr", sCommandLine))
	{
		CCPDLCSettingsDialog dia;
		dia.m_Logon = logonCallsign.c_str();
		dia.m_Password = logonCode.c_str();
		dia.m_Sound = int(PlaySoundClr);

		if (dia.DoModal() != IDOK)
			return true;

		logonCallsign = dia.m_Logon;
		logonCode = dia.m_Password;
		PlaySoundClr = bool(!!dia.m_Sound);
		SaveDataToSettings("cpdlc_logon", "The CPDLC logon callsign", logonCallsign.c_str());
		SaveDataToSettings("cpdlc_password", "The CPDLC logon password", logonCode.c_str());
		int temp = 0;
		if (PlaySoundClr)
			temp = 1;
		SaveDataToSettings("cpdlc_sound", "Play sound on clearance request", std::to_string(temp).c_str());

		// Update credentials in DatalinkManager
		if (datalinkManager != nullptr) {
			datalinkManager->setCredentials(logonCode, logonCallsign);
		}

		return true;
	}
	return false;
}

void CSMRPlugin::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int * pColorCode, COLORREF * pRGB, double * pFontSize) {
	if (ItemCode == TAG_ITEM_DATALINK_STS) {
		if (FlightPlan.IsValid() && datalinkManager != nullptr) {
			if (datalinkManager->isAircraftDemandingClearance(FlightPlan.GetCallsign())) {
				*pColorCode = TAG_COLOR_RGB_DEFINED;
				if (BLINK)
					*pRGB = RGB(130, 130, 130);
				else
					*pRGB = RGB(255, 255, 0);

				vector<string> standby = datalinkManager->getAircraftStandby();
				if (std::find(standby.begin(), standby.end(), FlightPlan.GetCallsign()) != standby.end())
					strcpy_s(sItemString, 16, "S");
				else
					strcpy_s(sItemString, 16, "R");
			}
			else {
				vector<string> messages = datalinkManager->getAircraftMessage();
				if (std::find(messages.begin(), messages.end(), FlightPlan.GetCallsign()) != messages.end()) {
					*pColorCode = TAG_COLOR_RGB_DEFINED;
					if (BLINK)
						*pRGB = RGB(130, 130, 130);
					else
						*pRGB = RGB(255, 255, 0);
					strcpy_s(sItemString, 16, "T");
				}
				else {
					vector<string> wilco = datalinkManager->getAircraftWilco();
					if (std::find(wilco.begin(), wilco.end(), FlightPlan.GetCallsign()) != wilco.end()) {
						*pColorCode = TAG_COLOR_RGB_DEFINED;
						*pRGB = RGB(0, 176, 0);
						strcpy_s(sItemString, 16, "V");
					}
					else {
						vector<string> sent = datalinkManager->getAircraftMessageSent();
						if (std::find(sent.begin(), sent.end(), FlightPlan.GetCallsign()) != sent.end()) {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = RGB(255, 255, 0);
							strcpy_s(sItemString, 16, "V");
						}
						else {
							*pColorCode = TAG_COLOR_RGB_DEFINED;
							*pRGB = RGB(130, 130, 130);

							strcpy_s(sItemString, 16, "-");
						}
					}
				}
			}
		}
	}

	if (ItemCode == TAG_ITEM_QNH && FlightPlan.IsValid()) {
		switch (qnhTracker.getReceivedQnh(FlightPlan.GetCallsign(), FlightPlan.GetFlightPlanData().GetOrigin(), sItemString)) {
		case CQnhTracker::ReceivedQnh::None:
			*pColorCode = TAG_COLOR_DEFAULT;
			strcpy_s(sItemString, 16, "-");
			break;

		case CQnhTracker::ReceivedQnh::Latest:
			*pColorCode = TAG_COLOR_REDUNDANT;
			break;

		case CQnhTracker::ReceivedQnh::Stale:
			*pColorCode = TAG_COLOR_INFORMATION;
			break;
		}
	}
}

void CSMRPlugin::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area)
{
	CFlightPlan FlightPlan = FlightPlanSelectASEL();

	if (datalinkManager == nullptr)
		return;

	switch (FunctionId) {
	case TAG_FUNC_DATALINK_MENU: {
		bool menu_is_datalink = true;

		if (FlightPlan.IsValid()) {
			if (datalinkManager->isAircraftDemandingClearance(FlightPlan.GetCallsign())) {
				menu_is_datalink = false;
			}
		}

		OpenPopupList(Area, "Datalink menu", 1);
		AddPopupListElement("Confirm", "", TAG_FUNC_DATALINK_CONFIRM, false, 2, menu_is_datalink);
		AddPopupListElement("Message", "", TAG_FUNC_DATALINK_MESSAGE, false, 2, false, true);
		AddPopupListElement("Standby", "", TAG_FUNC_DATALINK_STBY, false, 2, menu_is_datalink);
		AddPopupListElement("Voice", "", TAG_FUNC_DATALINK_VOICE, false, 2, menu_is_datalink);
		AddPopupListElement("Reset", "", TAG_FUNC_DATALINK_RESET, false, 2, false, true);
		AddPopupListElement("Close", "", EuroScopePlugIn::TAG_ITEM_FUNCTION_NO, false, 2, false, true);

		break;
	}

	case TAG_FUNC_DATALINK_RESET:
		if (FlightPlan.IsValid()) {
			datalinkManager->removeAircraftDemandingClearance(FlightPlan.GetCallsign());
			datalinkManager->removeAircraftStandby(FlightPlan.GetCallsign());
			// Remove all other states for this aircraft
			if (datalinkManager->isAircraftMessageSent(FlightPlan.GetCallsign())) {
				vector<string> sent = datalinkManager->getAircraftMessageSent();
				sent.erase(std::remove(sent.begin(), sent.end(), FlightPlan.GetCallsign()), sent.end());
			}
			datalinkManager->removePendingMessage(FlightPlan.GetCallsign());
		}

		break;

	case TAG_FUNC_DATALINK_STBY:
		if (FlightPlan.IsValid()) {
			datalinkManager->addAircraftStandby(FlightPlan.GetCallsign());
			datalinkManager->sendSimpleCpdlcMessage(FlightPlan.GetCallsign(), "STANDBY", "NE");
		}

		break;

	case TAG_FUNC_DATALINK_MESSAGE:
		if (FlightPlan.IsValid()) {
			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			CDataLinkDialog dia;
			dia.m_Callsign = FlightPlan.GetCallsign();
			dia.m_Aircraft = FlightPlan.GetFlightPlanData().GetAircraftFPType();
			dia.m_Dest = FlightPlan.GetFlightPlanData().GetDestination();
			dia.m_From = FlightPlan.GetFlightPlanData().GetOrigin();

			AcarsMessage msg = datalinkManager->getPendingMessage(FlightPlan.GetCallsign());
			dia.m_Req = msg.message.c_str();

			string toReturn = "";

			if (dia.DoModal() != IDOK)
				return;

			datalinkManager->sendSimpleCpdlcMessage(FlightPlan.GetCallsign(), dia.m_Message, "NE");
		}

		break;

	case TAG_FUNC_DATALINK_VOICE:
		if (FlightPlan.IsValid()) {
			datalinkManager->sendSimpleCpdlcMessage(FlightPlan.GetCallsign(), "UNABLE - CALL ON FREQ", "R");

			datalinkManager->removeAircraftDemandingClearance(FlightPlan.GetCallsign());
			datalinkManager->removeAircraftStandby(FlightPlan.GetCallsign());
			datalinkManager->removePendingMessage(FlightPlan.GetCallsign());
			datalinkManager->addAircraftMessageSent(FlightPlan.GetCallsign());
		}

		break;

	case TAG_FUNC_DATALINK_CONFIRM:
		if (FlightPlan.IsValid()) {

			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			CDataLinkDialog dia;
			dia.m_Callsign = FlightPlan.GetCallsign();
			dia.m_Aircraft = FlightPlan.GetFlightPlanData().GetAircraftFPType();
			dia.m_Dest = FlightPlan.GetFlightPlanData().GetDestination();
			dia.m_From = FlightPlan.GetFlightPlanData().GetOrigin();
			dia.m_Departure = FlightPlan.GetFlightPlanData().GetSidName();
			dia.m_Rwy = FlightPlan.GetFlightPlanData().GetDepartureRwy();
			dia.m_SSR = FlightPlan.GetControllerAssignedData().GetSquawk();
			string freq = std::to_string(ControllerMyself().GetPrimaryFrequency());
			if (ControllerSelect(FlightPlan.GetCoordinatedNextController()).GetPrimaryFrequency() != 0)
				string freq = std::to_string(ControllerSelect(FlightPlan.GetCoordinatedNextController()).GetPrimaryFrequency());
			freq = freq.substr(0, 7);
			dia.m_Freq = freq.c_str();
			AcarsMessage msg = datalinkManager->getPendingMessage(FlightPlan.GetCallsign());
			dia.m_Req = msg.message.c_str();

			string toReturn = "";

			int ClearedAltitude = FlightPlan.GetControllerAssignedData().GetClearedAltitude();
			int Ta = GetTransitionAltitude();

			if (ClearedAltitude != 0) {
				if (ClearedAltitude > Ta && ClearedAltitude > 2) {
					string str = std::to_string(ClearedAltitude);
					for (size_t i = 0; i < 5 - str.length(); i++)
						str = "0" + str;
					if (str.size() > 3)
						str.erase(str.begin() + 3, str.end());
					toReturn = "FL";
					toReturn += str;
				}
				else if (ClearedAltitude <= Ta && ClearedAltitude > 2) {
					toReturn = std::to_string(ClearedAltitude);
					toReturn += "ft";
				}
			}
			dia.m_Climb = toReturn.c_str();

			if (dia.DoModal() != IDOK)
				return;

			DatalinkClearance clearance;
			clearance.callsign = FlightPlan.GetCallsign();
			clearance.destination = FlightPlan.GetFlightPlanData().GetDestination();
			clearance.rwy = FlightPlan.GetFlightPlanData().GetDepartureRwy();
			clearance.sid = FlightPlan.GetFlightPlanData().GetSidName();
			clearance.asat = dia.m_TSAT;
			clearance.ctot = dia.m_CTOT;
			clearance.freq = dia.m_Freq;
			clearance.message = dia.m_Message;
			clearance.squawk = FlightPlan.GetControllerAssignedData().GetSquawk();
			clearance.climb = toReturn;

			myfrequency = std::to_string(ControllerMyself().GetPrimaryFrequency()).substr(0, 7);

			datalinkManager->sendDatalinkClearance(clearance);
		}

		break;

	case TAG_FUNC_QNH_UPDATE:
		if (FlightPlan.IsValid())
			qnhTracker.updateReceivedQnh(FlightPlan.GetCallsign(), FlightPlan.GetFlightPlanData().GetOrigin());

		break;

	case TAG_FUNC_QNH_RESET:
		if (FlightPlan.IsValid())
			qnhTracker.resetReceivedQnh(FlightPlan.GetCallsign());

		break;
	}
}

void CSMRPlugin::OnFlightPlanDisconnect(CFlightPlan FlightPlan)
{

	CRadarTarget rt = RadarTargetSelect(FlightPlan.GetCallsign());

	if (std::find(ReleasedTracks.begin(), ReleasedTracks.end(), rt.GetSystemID()) != ReleasedTracks.end())
		ReleasedTracks.erase(std::find(ReleasedTracks.begin(), ReleasedTracks.end(), rt.GetSystemID()));

	if (std::find(ManuallyCorrelated.begin(), ManuallyCorrelated.end(), rt.GetSystemID()) != ManuallyCorrelated.end())
		ManuallyCorrelated.erase(std::find(ManuallyCorrelated.begin(), ManuallyCorrelated.end(), rt.GetSystemID()));

	qnhTracker.resetReceivedQnh(FlightPlan.GetCallsign());
}

void CSMRPlugin::OnTimer(int Counter)
{

	BLINK = !BLINK;

	if (HoppieConnected && ConnectionMessage) {
		DisplayUserMessage("CPDLC", "Server", "Logged in!", true, true, false, true, false);
		ConnectionMessage = false;
	}

	if (FailedToConnectMessage) {
		DisplayUserMessage("CPDLC", "Server", "Could not login! Callsign probably in use.", true, true, false, true, false);
		FailedToConnectMessage = false;
	}

	if (HoppieConnected && GetConnectionType() == CONNECTION_TYPE_NO) {
		DisplayUserMessage("CPDLC", "Server", "Automatically logged off!", true, true, false, true, false);
		HoppieConnected = false;
	}

	if (((clock() - timer) / CLOCKS_PER_SEC) > pollInterval && HoppieConnected && datalinkManager != nullptr) {
		datalinkManager->pollMessages();
		timer = clock();
		pollInterval = 45 + rand() % 31; // Next random interval between 45-75 seconds
	}

	if (datalinkManager != nullptr) {
		vector<string> wilco = datalinkManager->getAircraftWilco();
		for (auto &ac : wilco)
		{
			CRadarTarget RadarTarget = RadarTargetSelect(ac.c_str());

			if (RadarTarget.IsValid()) {
				if (RadarTarget.GetGS() > 160) {
					// Remove aircraft from wilco list when GS > 160
					wilco.erase(std::remove(wilco.begin(), wilco.end(), ac), wilco.end());
				}
			}
		}
	}
};

CRadarScreen * CSMRPlugin::OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{

	if (!strcmp(sDisplayName, MY_PLUGIN_VIEW_AVISO)) {
		CSMRRadar* rd = new CSMRRadar();
		RadarScreensOpened.push_back(rd);
		return rd;
	}

	return NULL;
}

void CSMRPlugin::OnNewMetarReceived(const char *station, const char *metar) {
	qnhTracker.processMetar(station, metar);
}

//---EuroScopePlugInExit-----------------------------------------------

void __declspec (dllexport) EuroScopePlugInExit(void)
{
	for each (auto var in RadarScreensOpened)
	{
		var->EuroScopePlugInExitCustom();
	}
}
