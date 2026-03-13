#include "stdafx.h"
#include <cstring>

#include "QnhTracker.hpp"

void CQnhTracker::processMetar(const char *station, const char *metar)
{
	if ((metar = std::strstr(metar, " Q")) && std::strlen(metar) >= 6)
	{
		metar += 4;

		auto &entry = aerodrome_qnh[station];
		if (std::strncmp(entry.qnh, metar, 2))
		{
			std::memcpy(entry.qnh, metar, 2);
			++entry.serial;
		}
	}
	else
	{
		aerodrome_qnh.erase(station);
	}
}

void CQnhTracker::updateReceivedQnh(const char *callsign, const char *aerodrome)
{
	if (auto it = aerodrome_qnh.find(aerodrome); it != aerodrome_qnh.cend())
		aircraft_qnh[callsign] = it->second;
}

void CQnhTracker::resetReceivedQnh(const char *callsign)
{
	aircraft_qnh.erase(callsign);
}

CQnhTracker::ReceivedQnh CQnhTracker::getReceivedQnh(const char *callsign, const char *aerodrome, char out[3])
{
	if (auto ac = aircraft_qnh.find(callsign); ac != aircraft_qnh.cend())
	{
		std::memcpy(out, ac->second.qnh, 2);
		out[2] = 0;

		if (auto ad = aerodrome_qnh.find(aerodrome); ad != aerodrome_qnh.cend() && ad->second == ac->second)
			return CQnhTracker::ReceivedQnh::Latest;

		return CQnhTracker::ReceivedQnh::Stale;
	}
	else
	{
		return CQnhTracker::ReceivedQnh::None;
	}
}
