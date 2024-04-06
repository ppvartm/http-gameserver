#include "log.h"

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    boost::json::value jv = {
            {"timestamp", to_iso_extended_string(*rec[timestamp])},
            {"data", (*rec[additional_data]).as_object()},
            {"message", *rec[logging::expressions::smessage]}
    };
    strm << boost::json::serialize(jv);
}

void ServerStartLog(unsigned port, boost::asio::ip::address ip) {
    boost::json::value data = {
        {"port", port},
        {"address", ip.to_string()}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server started";
}

void ServerStopLog(unsigned returns_code, std::string_view ex) {
    boost::json::value data;
    if (ex == "")
        data = {
        {"code", returns_code},
    };
    else
        data = {
        {"code", returns_code},
        {"exception", ex}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server exited";
}

void ServerErrorLog(unsigned code, std::string_view message, std::string_view place) {
    boost::json::value data{
        {"code", code},
        {"text", message},
        {"where", place}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "error";
}