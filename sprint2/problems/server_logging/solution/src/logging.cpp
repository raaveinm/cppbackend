#include "logging.h"

#include <iostream>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    auto ts = logging::extract<boost::posix_time::ptime>("TimeStamp", rec);
    json::object log_record;
    log_record["timestamp"] = to_iso_extended_string(*ts);
    log_record["message"] = *rec[expr::smessage];
    if (rec.attribute_values().count("AdditionalData")) {
        if (const auto val = boost::log::extract<json::value>("AdditionalData", rec)) {
            log_record["data"] = val.get();
        }
    }

    strm << json::serialize(log_record);
}

void InitBoostLog() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}
