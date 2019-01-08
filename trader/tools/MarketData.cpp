// MarketData.cpp

#include "MarketData.hpp"

#include "NetworkingUtilities.hpp"
#include "JSONUtilities.hpp"
#include "Authentication.hpp"
#include "PreprocessorOptions.hpp"

#include <vector>
#include <iostream>
#include <stdexcept>
#include <ctime>

namespace tools
{
	std::string MarketData::marketQueryAlpaca(std::string url,
	 const std::vector<std::string>& symbols, const std::vector<std::string>& otherParams)
	{
		std::string auth = ("APCA-API-KEY-ID: " + Authentication::key);
		std::string secret = ("APCA-API-SECRET-KEY: " + Authentication::secretKey);
		std::vector<std::string> headers = {auth, secret};

		// used for pricing info
		std::string list = "";
		if ( ! symbols.empty())
		{
			for (size_t i = 0; i < symbols.size(); i++)
			{
				list = list+symbols[i];
				if (i != symbols.size()-1) { list = list+","; }
			}
		}
		
		if ( ! otherParams.empty())
		{
			for (const std::string& param : otherParams)
			{
				url = url+"&"+param;
			}
		}
		return (simpleGet(url, "", headers));
	}

	// @param url after https://api.iextrading.com/1.0/
	std::string MarketData::marketQueryIEX(const std::string& endOfUrl)
	{
		return (simpleGet(IEX_DOMAIN+endOfUrl));
	}

	std::vector<double> MarketData::getPrices(const std::vector<std::string>& symbols)
	{
		std::vector<double> results;

		for (const std::string& symbol : symbols)
		{
			std::string iexResponse = marketQueryIEX("stock/"+symbol+"/price");
			
			// response is just a number (no JSON)
			results.push_back(std::stod(iexResponse));
		}

		return results;
	}

	bool MarketData::isOpen()
	{
		// first check using local time (to prevent rate limiting)
        std::time_t now = std::time(NULL);
		std::tm * ptm = std::localtime(&now);
		char buffer[32];

		// Format: Mo, 15.06.2009 20:20:00
		std::strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", ptm);
		if (buffer[0] == 'S')
		{
			// Saturday or Sunday
			return false;
		}
		else if ( (buffer[16] == 0 && buffer[17] < '9') || (buffer[18] == 0 && buffer[19] == '9' && buffer[19] < '3')
		 || (buffer[16] == '1' && buffer[17] >= '6') || buffer[16] =='2')
		{
			// outside 09:30 to 16:00
			return false;
		}

  		// query if necessary
		std::string response = marketQueryAlpaca(PAPER_DOMAIN+"clock");
		rapidjson::Document domTree = getDOMTree(response);
		return domTree["is_open"].GetBool();
	}

	// return back vector of doubles containing key stats from IEX
	// Refer to: https://iextrading.com/developer/docs/#key-stats
	std::vector<double> MarketData::getKeyStats(const std::string& symbol, const std::vector<std::string>& fields)
	{
		std::vector<double> results;
		results.reserve(fields.size());

		std::string iexResponse = marketQueryIEX("stock/"+symbol+"/stats");
		rapidjson::Document doc = getDOMTree(iexResponse);

		for (const std::string& field : fields)
		{
			results.push_back(doc[field.c_str()].GetDouble());
		}

		return results;
	}
}