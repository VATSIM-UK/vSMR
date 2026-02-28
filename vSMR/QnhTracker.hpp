#pragma once

#include <map>
#include <string>

class CQnhTracker
{
private:
	struct QnhEntry
	{
		char qnh[2];
		short serial;

		inline bool operator==(const CQnhTracker::QnhEntry &rhs) const {
			return qnh[0] == rhs.qnh[0] && qnh[1] == rhs.qnh[1] && serial == rhs.serial;
		}
	};

	std::map<std::string, QnhEntry> aerodrome_qnh, aircraft_qnh;

public:
	enum ReceivedQnh
	{
		None,
		Latest,
		Stale,
	};

	CQnhTracker() {}

	void processMetar(const char *station, const char *metar);

	void updateReceivedQnh(const char *callsign, const char *aerodrome);
	void resetReceivedQnh(const char *callsign);
	ReceivedQnh getReceivedQnh(const char *callsign, const char *aerodrome, char out[3]);
};
