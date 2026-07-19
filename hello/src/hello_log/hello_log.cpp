//
// Created by Kirill "Raaveinm" on 7/18/26.
//

#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <string_view>
#include <boost/beast/core/file.hpp>
#include <thread>

namespace logging = boost::log;

void InitBoostLogFilter() {
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int)

static void LogFormater(logging::record_view const& rec, logging::formatting_ostream& strm) {
    /*
    strm << logging::extract<unsigned int>("LineID", rec) << ": "; // parse LineID
    strm << "<" << rec[logging::trivial::severity] << "> "; // type lineid
    strm << rec[logging::expressions::smessage]; // message
    */
    // or
    strm << rec[line_id] << ": ";
    const auto ts = *rec[timestamp];
    strm << to_iso_extended_string(ts) << ": ";
    strm << "<" << rec[logging::trivial::severity] << "> ";
    strm << rec[logging::expressions::smessage];
    strm << rec[file] << " - " << rec[line] << " ";
}

int main(int argc, char *argv[]) {
    logging::add_common_attributes();
    InitBoostLogFilter(); // filters
    
    namespace keywords = boost::log::keywords;

    logging::add_file_log(
        keywords::file_name = "sample.log", // file
        std::clog, // output stream
        keywords::format = &LogFormater, // output formatting
        keywords::auto_flush = true,
        keywords::open_mode = std::ios_base::app | std::ios_base::out, // append to file
        keywords::rotation_size = 10 * 1024 * 1024, // rotation by size
        keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(12, 0, 0) // rotation by time
    );

    using namespace std::literals::string_view_literals;

    BOOST_LOG_TRIVIAL(trace) << "level trace"sv;
    BOOST_LOG_TRIVIAL(debug) << "log level debug"sv;
    BOOST_LOG_TRIVIAL(info) << "log level info"sv
    << logging::add_value(file, std::string(__FILE__))
    << logging::add_value(line, __LINE__)
    << "Something happend"sv;;
    BOOST_LOG_TRIVIAL(warning) << "log level warning"sv;

    std::thread t([](){
        const std::chrono::time_point<std::chrono::steady_clock> await =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        std::this_thread::sleep_until(await);
        BOOST_LOG_TRIVIAL(warning) << "second thread"sv << logging::add_value(line, __LINE__);
    });


    BOOST_LOG_TRIVIAL(error) << "log level error"sv;
    BOOST_LOG_TRIVIAL(fatal) << "log level fatal"sv;

    t.join();

    return 0;
}