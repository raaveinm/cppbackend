#include "logging.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/json.hpp>

namespace logging {

    namespace logging = boost::log;
    namespace keywords = boost::log::keywords;
    namespace expr = boost::log::expressions;
    namespace json = boost::json;

    void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
        json::object log_record;

        // Add timestamp
        auto ts = logging::extract<boost::posix_time::ptime>("TimeStamp", rec);
        log_record["timestamp"] = to_iso_extended_string(*ts);

        // Add message
        log_record["message"] = *logging::extract<std::string>("Message", rec);

        // Add additional data
        auto data = logging::extract<json::value>("AdditionalData", rec);
        if (data) {
            log_record["data"] = *data;
        }

        strm << json::serialize(log_record);
    }

    void Init() {
        logging::add_common_attributes();

        auto sink = logging::add_console_log(std::cout, keywords::format = &MyFormatter);
        sink->locked_backend()->auto_flush(true);
    }

} // namespace logging