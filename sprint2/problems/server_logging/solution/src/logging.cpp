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

        if (const auto ts = logging::extract<boost::posix_time::ptime>("TimeStamp", rec))
            log_record["timestamp"] = to_iso_extended_string(*ts);

        if (const auto msg = logging::extract<std::string>("Message", rec))
            log_record["message"] = *msg;

        if (const auto data_obj = logging::extract<json::object>("AdditionalData", rec)) {
            log_record["data"] = *data_obj;
        } else if (const auto data_val = logging::extract<json::value>("AdditionalData", rec)) {
            log_record["data"] = *data_val;
        }

        strm << json::serialize(log_record);
    }

    void Init() {
        logging::add_common_attributes();

        const auto sink = logging::add_console_log(std::cout, keywords::format = &MyFormatter);
        sink->locked_backend()->auto_flush(true);
    }

} // namespace logging