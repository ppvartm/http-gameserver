#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/log/utility/setup/common_attributes.hpp> 
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include "../json_tools/json_loader.h"

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)
namespace logging = boost::log;



void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void ServerStartLog(unsigned port, boost::asio::ip::address ip);

void ServerStopLog(unsigned returns_code, std::string_view ex = "");

void ServerErrorLog(unsigned code, std::string_view message, std::string_view place);