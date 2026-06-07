#pragma once

#include <string>
#include <map>
#include <queue>
#include <vector>
#include <mutex>
#include "HttpHelper.hpp"

using namespace std;

// Message types
enum class MessageType {
	CPDLC,
	TELEX
};

// Pending ACK structure with sequence tracking
struct PendingAck {
	string callsign;
	string message;
	MessageType type;
	int sequenceNumber;
	string timestamp;
	string date;
};

// Incoming ACARS message structure
struct AcarsMessage {
	string from;
	string type;
	string message;
};

// Aircraft state structure
struct AircraftDatalinkState {
	string callsign;
	int lastSequenceNumber;
	queue<PendingAck> pendingAcks;
	AcarsMessage lastMessage;
	bool isWaitingForResponse;
	int messageRequestCount;
	int lastPolledTime;
};

// Clearance data structure
struct DatalinkClearance {
	string callsign;
	string destination;
	string sid;
	string rwy;
	string freq;
	string ctot;
	string asat;
	string squawk;
	string message;
	string climb;
};

class DatalinkManager
{
private:
	HttpHelper * httpHelper;
	string baseUrlDatalink;
	string logonCode;
	string logonCallsign;
	int messageIdCounter;
	
	// Per-aircraft state tracking
	map<string, AircraftDatalinkState> aircraftStates;
	
	// Lists for tracking aircraft status
	vector<string> aircraftDemandingClearance;
	vector<string> aircraftMessageSent;
	vector<string> aircraftMessage;
	vector<string> aircraftWilco;
	vector<string> aircraftStandby;
	map<string, AcarsMessage> pendingMessages;
	
	// Thread safety
	mutable mutex statesMutex;
	mutable mutex listsMutex;
	
	// Private helper methods
	void parseAndQueueIncomingMessage(const AcarsMessage& message);
	void queueAckForAircraft(const string& callsign, const string& message, MessageType type);
	int getAndIncrementSequenceNumber(const string& callsign);
	string buildDatalinkUrl(const string& destination, const string& messageType, const string& packet);
	string buildClearanceUrl(const DatalinkClearance& clearance);
	bool sendRawDatalinkMessage(const string& url);
	
public:
	DatalinkManager(HttpHelper * httpHelper, const string& baseUrl);
	~DatalinkManager();

	// Initialization and connection
	bool login(const string& logonCode, const string& logonCallsign);
	void setCredentials(const string& code, const string& callsign);
	void setHttpHelper(HttpHelper * helper);
	
	// Message sending
	void sendAcknowledgement(const string& callsign, const string& message);
	void sendSimpleCpdlcMessage(const string& callsign, const string& message, const string& responses);
	void sendDatalinkClearance(const DatalinkClearance& clearance);
	
	// Message polling
	void pollMessages();
	
	// Aircraft state management
	void addAircraftDemandingClearance(const string& callsign);
	void removeAircraftDemandingClearance(const string& callsign);
	bool isAircraftDemandingClearance(const string& callsign) const;
	
	void addAircraftMessage(const string& callsign);
	void removeAircraftMessage(const string& callsign);
	
	void addAircraftWilco(const string& callsign);
	bool hasAircraftWilco(const string& callsign) const;
	
	void addAircraftStandby(const string& callsign);
	void removeAircraftStandby(const string& callsign);
	
	void addAircraftMessageSent(const string& callsign);
	bool isAircraftMessageSent(const string& callsign) const;
	
	// Pending message management
	void setPendingMessage(const string& callsign, const AcarsMessage& message);
	bool hasPendingMessage(const string& callsign) const;
	AcarsMessage getPendingMessage(const string& callsign) const;
	void removePendingMessage(const string& callsign);
	
	// Getters for status information
	vector<string> getAircraftDemandingClearance() const;
	vector<string> getAircraftWilco() const;
	vector<string> getAircraftMessageSent() const;
	vector<string> getAircraftMessage() const;
	vector<string> getAircraftStandby() const;
	
	// Utility methods
	int getNextMessageId();
	bool isConnected() const;
};
