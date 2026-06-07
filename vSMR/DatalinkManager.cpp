#include "stdafx.h"
#include "DatalinkManager.hpp"
#include <sstream>
#include <algorithm>
#include <ctime>

DatalinkManager::DatalinkManager(HttpHelper * httpHelper, const string& baseUrl)
	: httpHelper(httpHelper), baseUrlDatalink(baseUrl), logonCode(""), logonCallsign(""), messageIdCounter(0)
{
	messageIdCounter = rand() % 10000 + 1789;
}

DatalinkManager::~DatalinkManager()
{
}

void DatalinkManager::setCredentials(const string& code, const string& callsign)
{
	logonCode = code;
	logonCallsign = callsign;
}

void DatalinkManager::setHttpHelper(HttpHelper * helper)
{
	if (helper != NULL) {
		httpHelper = helper;
	}
}

int DatalinkManager::getNextMessageId()
{
	return ++messageIdCounter;
}

bool DatalinkManager::isConnected() const
{
	if (httpHelper == NULL) {
		return false;
	}
	
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=SERVER&type=PING";
	
	string raw = httpHelper->downloadStringFromURL(url);
	
	if (raw.substr(0, 2) == "ok") {
		return true;
	}
	return false;
}

int DatalinkManager::getAndIncrementSequenceNumber(const string& callsign)
{
	lock_guard<mutex> lock(statesMutex);
	
	if (aircraftStates.find(callsign) == aircraftStates.end()) {
		aircraftStates[callsign] = AircraftDatalinkState();
		aircraftStates[callsign].callsign = callsign;
		aircraftStates[callsign].lastSequenceNumber = 0;
		aircraftStates[callsign].isWaitingForResponse = false;
		aircraftStates[callsign].messageRequestCount = 0;
		aircraftStates[callsign].lastPolledTime = 0;
	}
	
	return ++aircraftStates[callsign].lastSequenceNumber;
}

void DatalinkManager::queueAckForAircraft(const string& callsign, const string& message, MessageType type)
{
	lock_guard<mutex> lock(statesMutex);
	
	if (aircraftStates.find(callsign) == aircraftStates.end()) {
		aircraftStates[callsign] = AircraftDatalinkState();
		aircraftStates[callsign].callsign = callsign;
		aircraftStates[callsign].lastSequenceNumber = 0;
		aircraftStates[callsign].isWaitingForResponse = false;
		aircraftStates[callsign].messageRequestCount = 0;
		aircraftStates[callsign].lastPolledTime = 0;
	}
	
	PendingAck ack;
	ack.callsign = callsign;
	ack.message = message;
	ack.type = type;
	ack.sequenceNumber = ++aircraftStates[callsign].lastSequenceNumber;
	
	// Format timestamp
	time_t now = time(NULL);
	tm tmnow;
	gmtime_s(&tmnow, &now);
	char buf[64];
	strftime(buf, sizeof(buf), "%H%MZ", &tmnow);
	ack.timestamp = buf;
	strftime(buf, sizeof(buf), "%d%m%y", &tmnow);
	ack.date = buf;
	
	aircraftStates[callsign].pendingAcks.push(ack);
}

string DatalinkManager::buildDatalinkUrl(const string& destination, const string& messageType, const string& packet)
{
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=";
	url += destination;
	url += "&type=";
	url += messageType;
	url += "&packet=";
	url += packet;

	// URL encode spaces
	size_t start_pos = 0;
	while ((start_pos = url.find(" ", start_pos)) != std::string::npos) {
		url.replace(start_pos, string(" ").length(), "%20");
		start_pos += string("%20").length();
	}
	
	return url;
}

string DatalinkManager::buildClearanceUrl(const DatalinkClearance& clearance)
{
	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=";
	url += clearance.callsign;
	url += "&type=CPDLC&packet=/data2/";
	url += std::to_string(getNextMessageId());
	url += "//R/";

	if (clearance.sid == "CHK" && clearance.rwy == "09R") // CPT 09R
	{
		url += "@";
		url += clearance.callsign;
		url += "@ CLRD TO @";
		url += clearance.destination;
		url += "@ OFF RWY @";
		url += clearance.rwy;
		url += "@ VIA CPT AFTER DEP CLIMB STRAIGHT AHEAD - AT LON DME 2.0 TURN RIGHT HDG @220@ - CLIMB @6000FT";
	}
	else // normal PDC message (UK Format)
	{
		url += "@";
		url += clearance.callsign;
		url += "@ CLRD TO @";
		url += clearance.destination;
		url += "@ OFF @";
		url += clearance.rwy;
		url += "@ VIA @";
		url += clearance.sid;
		url += "@ INIT CLB @";
		url += clearance.climb;
	}
	url += "@ SQUAWK @";
	url += clearance.squawk;
	url += "@ NEXT FREQ @";
	
	if (clearance.freq != "no" && clearance.freq.size() > 5) {
		url += clearance.freq;
	}

	url += "@ ";

	if (clearance.message != "no" && clearance.message.size() > 1) {
		url += clearance.message;
	}

	// URL encode spaces
	size_t start_pos = 0;
	while ((start_pos = url.find(" ", start_pos)) != std::string::npos) {
		url.replace(start_pos, string(" ").length(), "%20");
		start_pos += string("%20").length();
	}

	return url;
}

bool DatalinkManager::sendRawDatalinkMessage(const string& url)
{
	if (httpHelper == NULL) {
		return false;
	}
	
	string raw = httpHelper->downloadStringFromURL(url);
	
	if (raw.substr(0, 2) == "ok") {
		return true;
	}
	return false;
}

bool DatalinkManager::login(const string& code, const string& callsign)
{
	setCredentials(code, callsign);
	return isConnected();
}

void DatalinkManager::sendAcknowledgement(const string& callsign, const string& message)
{
	queueAckForAircraft(callsign, message, MessageType::CPDLC);
	
	lock_guard<mutex> lock(statesMutex);
	if (aircraftStates.find(callsign) != aircraftStates.end() && 
		!aircraftStates[callsign].pendingAcks.empty()) {
		
		const PendingAck& ack = aircraftStates[callsign].pendingAcks.front();
		
		string fullMessage = message;
		fullMessage += " ";
		fullMessage += ack.timestamp;
		fullMessage += " ";
		fullMessage += ack.date;
		
		string packet = "/data2/";
		packet += std::to_string(getNextMessageId());
		packet += "//";
		packet += "NE"; // No expected response
		packet += "/";
		packet += fullMessage;
		
		string url = buildDatalinkUrl(callsign, "CPDLC", packet);
		
		if (sendRawDatalinkMessage(url)) {
			aircraftStates[callsign].pendingAcks.pop();
		}
	}
}

void DatalinkManager::sendSimpleCpdlcMessage(const string& callsign, const string& message, const string& responses)
{
	string packet = "/data2/";
	packet += std::to_string(getNextMessageId());
	packet += "//";
	packet += responses;
	packet += "/";
	packet += message;

	string url = buildDatalinkUrl(callsign, "CPDLC", packet);
	sendRawDatalinkMessage(url);
}

void DatalinkManager::sendDatalinkClearance(const DatalinkClearance& clearance)
{
	string url = buildClearanceUrl(clearance);

	if (sendRawDatalinkMessage(url)) {
		{
			lock_guard<mutex> lock(listsMutex);
			
			// Remove from demanding clearance list
			auto it = std::find(aircraftDemandingClearance.begin(), aircraftDemandingClearance.end(), clearance.callsign);
			if (it != aircraftDemandingClearance.end()) {
				aircraftDemandingClearance.erase(it);
			}

			// Remove from standby list
			it = std::find(aircraftStandby.begin(), aircraftStandby.end(), clearance.callsign);
			if (it != aircraftStandby.end()) {
				aircraftStandby.erase(it);
			}

			// Add to message sent list
			aircraftMessageSent.push_back(clearance.callsign);
		}
		
		// Remove pending message
		removePendingMessage(clearance.callsign);
		
		// Queue a status ACK
		queueAckForAircraft(clearance.callsign, "CLEARANCE SENT", MessageType::CPDLC);
	}
}

void DatalinkManager::pollMessages()
{
	if (httpHelper == NULL) {
		return;
	}

	string url = baseUrlDatalink;
	url += "?logon=";
	url += logonCode;
	url += "&from=";
	url += logonCallsign;
	url += "&to=SERVER&type=POLL";
	
	string raw = httpHelper->downloadStringFromURL(url);

	if (raw.substr(0, 2) != "ok" || raw.size() <= 3) {
		return;
	}

	raw = raw + " ";
	raw = raw.substr(3, raw.size() - 3);

	string delimiter = "}} ";
	size_t pos = 0;
	while ((pos = raw.find(delimiter)) != std::string::npos) {
		string token = raw.substr(1, pos);

		string parsed;
		stringstream input_stringstream(token);
		AcarsMessage message;
		int i = 1;
		while (getline(input_stringstream, parsed, ' ')) {
			if (i == 1) {
				message.from = parsed;
			}
			if (i == 2) {
				message.type = parsed;
			}
			if (i > 2) {
				message.message.append(" ");
				message.message.append(parsed);
			}

			i++;
		}

		if (message.type.find("telex") != std::string::npos || message.type.find("cpdlc") != std::string::npos) {
			parseAndQueueIncomingMessage(message);
			setPendingMessage(message.from, message);
		}

		raw.erase(0, pos + delimiter.length());
	}
}

void DatalinkManager::parseAndQueueIncomingMessage(const AcarsMessage& message)
{
	lock_guard<mutex> lock(listsMutex);
	
	if (message.message.find("REQ") != std::string::npos || 
		message.message.find("CLR") != std::string::npos || 
		message.message.find("PDC") != std::string::npos || 
		message.message.find("PREDEP") != std::string::npos || 
		message.message.find("REQUEST") != std::string::npos) {
		
		if (message.message.find("LOGON") != std::string::npos) {
			sendSimpleCpdlcMessage(message.from, "UNABLE", "NE");
		} else {
			addAircraftDemandingClearance(message.from);
			
			// Queue acknowledgement for departure message request
			string reqAck = "DEPART MESSAGE REQUEST RECEIVED";
			queueAckForAircraft(message.from, reqAck, MessageType::CPDLC);
		}
	}
	else if (message.message.find("WILCO") != std::string::npos || 
			message.message.find("ROGER") != std::string::npos || 
			message.message.find("RGR") != std::string::npos || 
			message.message.find("ACCEPT") != std::string::npos) {
		
		if (std::find(aircraftMessageSent.begin(), aircraftMessageSent.end(), message.from) != aircraftMessageSent.end()) {
			addAircraftWilco(message.from);
			
			// Queue acknowledgement for clearance acceptance
			string clearanceAck = "DEPART MESSAGE ACK RECEIVED";
			queueAckForAircraft(message.from, clearanceAck, MessageType::CPDLC);
		}
	}
	else if (message.message.length() != 0) {
		addAircraftMessage(message.from);
	}
}

// Aircraft state management methods
void DatalinkManager::addAircraftDemandingClearance(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	if (std::find(aircraftDemandingClearance.begin(), aircraftDemandingClearance.end(), callsign) == aircraftDemandingClearance.end()) {
		aircraftDemandingClearance.push_back(callsign);
	}
}

void DatalinkManager::removeAircraftDemandingClearance(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	auto it = std::find(aircraftDemandingClearance.begin(), aircraftDemandingClearance.end(), callsign);
	if (it != aircraftDemandingClearance.end()) {
		aircraftDemandingClearance.erase(it);
	}
}

bool DatalinkManager::isAircraftDemandingClearance(const string& callsign) const
{
	lock_guard<mutex> lock(listsMutex);
	return std::find(aircraftDemandingClearance.begin(), aircraftDemandingClearance.end(), callsign) != aircraftDemandingClearance.end();
}

void DatalinkManager::addAircraftMessage(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	if (std::find(aircraftMessage.begin(), aircraftMessage.end(), callsign) == aircraftMessage.end()) {
		aircraftMessage.push_back(callsign);
	}
}

void DatalinkManager::removeAircraftMessage(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	auto it = std::find(aircraftMessage.begin(), aircraftMessage.end(), callsign);
	if (it != aircraftMessage.end()) {
		aircraftMessage.erase(it);
	}
}

void DatalinkManager::addAircraftWilco(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	if (std::find(aircraftWilco.begin(), aircraftWilco.end(), callsign) == aircraftWilco.end()) {
		aircraftWilco.push_back(callsign);
	}
}

bool DatalinkManager::hasAircraftWilco(const string& callsign) const
{
	lock_guard<mutex> lock(listsMutex);
	return std::find(aircraftWilco.begin(), aircraftWilco.end(), callsign) != aircraftWilco.end();
}

void DatalinkManager::addAircraftStandby(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	if (std::find(aircraftStandby.begin(), aircraftStandby.end(), callsign) == aircraftStandby.end()) {
		aircraftStandby.push_back(callsign);
	}
}

void DatalinkManager::removeAircraftStandby(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	auto it = std::find(aircraftStandby.begin(), aircraftStandby.end(), callsign);
	if (it != aircraftStandby.end()) {
		aircraftStandby.erase(it);
	}
}

void DatalinkManager::addAircraftMessageSent(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	if (std::find(aircraftMessageSent.begin(), aircraftMessageSent.end(), callsign) == aircraftMessageSent.end()) {
		aircraftMessageSent.push_back(callsign);
	}
}

bool DatalinkManager::isAircraftMessageSent(const string& callsign) const
{
	lock_guard<mutex> lock(listsMutex);
	return std::find(aircraftMessageSent.begin(), aircraftMessageSent.end(), callsign) != aircraftMessageSent.end();
}

void DatalinkManager::setPendingMessage(const string& callsign, const AcarsMessage& message)
{
	lock_guard<mutex> lock(listsMutex);
	pendingMessages[callsign] = message;
}

bool DatalinkManager::hasPendingMessage(const string& callsign) const
{
	lock_guard<mutex> lock(listsMutex);
	return pendingMessages.find(callsign) != pendingMessages.end();
}

AcarsMessage DatalinkManager::getPendingMessage(const string& callsign) const
{
	lock_guard<mutex> lock(listsMutex);
	if (pendingMessages.find(callsign) != pendingMessages.end()) {
		return pendingMessages.at(callsign);
	}
	return AcarsMessage();
}

void DatalinkManager::removePendingMessage(const string& callsign)
{
	lock_guard<mutex> lock(listsMutex);
	auto it = pendingMessages.find(callsign);
	if (it != pendingMessages.end()) {
		pendingMessages.erase(it);
	}
}

vector<string> DatalinkManager::getAircraftDemandingClearance() const
{
	lock_guard<mutex> lock(listsMutex);
	return aircraftDemandingClearance;
}

vector<string> DatalinkManager::getAircraftWilco() const
{
	lock_guard<mutex> lock(listsMutex);
	return aircraftWilco;
}

vector<string> DatalinkManager::getAircraftMessageSent() const
{
	lock_guard<mutex> lock(listsMutex);
	return aircraftMessageSent;
}

vector<string> DatalinkManager::getAircraftMessage() const
{
	lock_guard<mutex> lock(listsMutex);
	return aircraftMessage;
}

vector<string> DatalinkManager::getAircraftStandby() const
{
	lock_guard<mutex> lock(listsMutex);
	return aircraftStandby;
}
