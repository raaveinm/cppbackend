//
// Created by Kirill "Raaveinm" on 7/18/26.
//

#include <boost/log/trivial.hpp>
#include <string_view>

int main(int argc, char *argv[]) {
    using namespace std::literals::string_view_literals;
    BOOST_LOG_TRIVIAL(trace) << "Сообщение уровня trace"sv;
    BOOST_LOG_TRIVIAL(debug) << "Сообщение уровня debug"sv;
    BOOST_LOG_TRIVIAL(info) << "Сообщение уровня info"sv;
    BOOST_LOG_TRIVIAL(warning) << "Сообщение уровня warning"sv;
    BOOST_LOG_TRIVIAL(error) << "Сообщение уровня error"sv;
    BOOST_LOG_TRIVIAL(fatal) << "Сообщение уровня fatal"sv;
    return 0;
}
