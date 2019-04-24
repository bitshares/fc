#include <boost/test/unit_test.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include <fc/thread/thread.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>
#include <fc/io/json.hpp>
#include <fc/io/fstream.hpp>

#include <thread>
#include <iostream>
#include <fstream>

BOOST_AUTO_TEST_SUITE(logging_tests)

BOOST_AUTO_TEST_CASE(log_reboot)
{
    BOOST_TEST_MESSAGE("Setting up logger");
    fc::file_appender::config conf;
    conf.filename = "/tmp/my.log";
    conf.format = "${timestamp} ${thread_name} ${context} ${file}:${line} ${method} ${level}]  ${message}";
    conf.flush = true;
    conf.rotate = true;
    conf.rotation_interval = fc::seconds(5); // rotate every 5 seconds
    conf.rotation_limit = fc::seconds(20); // Don't keep files older than 20 seconds
    conf.max_object_depth = 200;

    fc::appender::ptr fa = fc::appender::create("file", "file", fc::variant(conf, 200));

    fc::path prev_log_filename = "";

    BOOST_TEST_MESSAGE("Starting Loop");
    for(int i = 0; i < conf.rotation_limit.to_seconds(); i++)
    {
        fc::log_context ctx(fc::log_level::all, "my_file.cpp", i, "my_method()");
        fc::log_message my_log_message( ctx, "${message}", {"message","This is a test"} );
        fa->log(my_log_message);

        fc::time_point now = fc::time_point::now();
        int64_t interval_seconds = conf.rotation_interval.to_seconds();
        int64_t file_number = now.sec_since_epoch() / interval_seconds;
        fc::time_point_sec start_time = fc::time_point_sec( (uint32_t)(file_number * interval_seconds) );
        std::string timestamp_string = start_time.to_non_delimited_iso_string();
        fc::path link_filename = conf.filename;
        fc::path log_filename = link_filename.parent_path() / (link_filename.filename().string() + "." + timestamp_string);

        if (prev_log_filename != log_filename) {
            if (i > conf.rotation_interval.to_seconds()) {
                std::string rez;
                fc::read_file_contents(prev_log_filename, rez);
                std::size_t found = rez.find("my_file.cpp:" + std::to_string(i - 1));
                BOOST_CHECK(found != std::string::npos);

                fc::read_file_contents(log_filename, rez);
                found = rez.find("my_file.cpp:" + std::to_string(i));
                BOOST_CHECK(found != std::string::npos);
            }
            prev_log_filename = log_filename;
        }

        fc::usleep(fc::seconds(1));
    }
    BOOST_TEST_MESSAGE("Loop complete");
}

BOOST_AUTO_TEST_SUITE_END()