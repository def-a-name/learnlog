#include <catch2/catch_all.hpp>
#include "file_utils.h"
#include "definitions.h"
#include "logger.h"
#include "sinks/formatters/pattern_formatter.h"
#include "sinks/ostream_sink.h"
#include "sinks/std_sinks.h"
#include "sinks/std_color_sinks.h"
#include "sinks/basic_file_sink.h"
#include "sinks/rolling_file_sink.h"
#include "base/static_mutex.h"

#include <functional>

#ifdef _WIN32
    #define B_FNAME L"test_tmp/basic_file_sink_test.txt"
    #define R_FNAME L"test_tmp/rolling_file_sink_test.txt"
#else
    #define B_FNAME "test_tmp/basic_file_sink_test.txt"
    #define R_FNAME "test_tmp/rolling_file_sink_test.txt"
#endif

using learnlog::filename_t;

std::string logger_ostream_sink_str(const std::string& text, std::string pattern) {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<learnlog::sinks::ostream_sink_st>(oss);
    learnlog::logger oss_logger("ostream sink test logger", oss_sink);
    
    oss_logger.set_log_level(learnlog::level::info);
    oss_logger.set_pattern(pattern);

    oss_logger.info(text);
    return oss.str();
}

void logger_std_sink(FILE* file, const std::string& text, std::string pattern) {
    using std_sink_st = learnlog::sinks::std_sink<learnlog::base::static_nullmutex>;

    auto std_sink = std::make_shared<std_sink_st>(file);
    learnlog::logger std_logger("stdout/stderr sink test logger", std_sink);
    
    std_logger.set_log_level(learnlog::level::trace);
    std_logger.set_pattern(pattern);
    std_logger.info(text);
}

void logger_stdout_color_sink(const std::string& text, std::string pattern,
                              learnlog::level::level_enum msg_lvl) {
    auto color_sink = std::make_shared<learnlog::sinks::stdout_color_sink_st>();
    learnlog::logger color_logger("stdout color sink test logger", color_sink);
    
    color_logger.set_log_level(learnlog::level::trace);
    color_logger.set_pattern(pattern);
    color_logger.log(msg_lvl, text);
}

void logger_basic_file_sink(const std::string& text, std::string pattern,
                            const filename_t& filename, bool trunc) {
    auto file_sink = std::make_shared<learnlog::sinks::basic_file_sink_st>(filename, trunc);
    learnlog::logger file_logger("basic file sink test logger", file_sink);
    
    file_logger.set_log_level(learnlog::level::trace);
    file_logger.set_pattern(pattern);
    file_logger.info(text);
    file_logger.flush();
}

void rolling_file_sink_info(const std::string& text, std::string pattern,
                            learnlog::logger_shr_ptr logger) {
    logger->set_pattern(pattern);
    logger->info(text);
}

// ======================================================================================

TEST_CASE("ostream_sink", "[sinks]") {
    std::string txt = "hello world!";
    REQUIRE(logger_ostream_sink_str(txt, "") == 
            fmt::format("{}", DEFAULT_EOL));
    REQUIRE(logger_ostream_sink_str(txt, "[%l] %v") == 
            fmt::format("[info] hello world!{}", DEFAULT_EOL));
    REQUIRE(logger_ostream_sink_str(txt, "%5!v") == 
            fmt::format("hello{}", DEFAULT_EOL));
}

TEST_CASE("std_sink", "[sinks]") {
    std::string txt = "hello world!";
    logger_std_sink(stdout, txt, "");
    logger_std_sink(stdout, txt, "stdout: [%n] %v");
    logger_std_sink(stdout, txt, "stdout: [%n] %5!v");
    logger_std_sink(stderr, txt, "stderr: [%n] %5!v");
    logger_std_sink(stderr, txt, "stderr: [%n] %v");
    logger_std_sink(stdout, txt, "");
}

TEST_CASE("stdout_color_sink", "[sinks]") {
    std::string txt = "hello world!";
    logger_stdout_color_sink(txt, "", learnlog::level::trace);
    logger_stdout_color_sink(txt, "[%n] %^%c%$ [%l] msg: %v", learnlog::level::trace);
    logger_stdout_color_sink(txt, "[%n] %c %^[%l]%$ msg: %v", learnlog::level::debug);
    logger_stdout_color_sink(txt, "[%n] %c [%l] %^msg:%$ %v", learnlog::level::info);
    logger_stdout_color_sink(txt, "[%n] %c [%l] msg: %^%v%$", learnlog::level::warn);
    logger_stdout_color_sink(txt, "%^[%n]%$ %c [%l] msg: %v", learnlog::level::error);
    logger_stdout_color_sink(txt, "[%n] %c [%l] %^msg: %v%$", learnlog::level::critical);
}

TEST_CASE("basic_file_sink", "[sinks]") {
    std::string txt = "hello world!";
    logger_basic_file_sink(txt, "", B_FNAME, true);
    logger_basic_file_sink(txt, "[%l] %v", B_FNAME, false);
    logger_basic_file_sink(txt, "%5!v", B_FNAME, false);
    
    REQUIRE(file_content(B_FNAME) == 
            fmt::format("{}[info] hello world!{}hello{}", 
                        DEFAULT_EOL, DEFAULT_EOL, DEFAULT_EOL));
}

TEST_CASE("rolling_file_sink", "[sinks]") {
    std::string txt = "hello world!";
    size_t file_size = 32;
    size_t file_num = 3;

    clean_test_tmp();
    FILE* fp = nullptr;
#ifdef _WIN32
    learnlog::filename_t fname = L"test_tmp/rolling_file_sink_test_1.txt";
#else
    learnlog::filename_t fname = "test_tmp/rolling_file_sink_test_1.txt";
#endif
    learnlog::base::file_base::open(&fp, fname);
    learnlog::base::file_base::close(&fp, fname);

    auto roll_sink = std::make_shared<learnlog::sinks::rolling_file_sink_st>(R_FNAME,
                                                                          file_size,
                                                                          file_num);
    auto roll_logger = std::make_shared<learnlog::logger>("rolling file sink test logger",
                                                       roll_sink);
    rolling_file_sink_info(txt, "", roll_logger);
    rolling_file_sink_info(txt, "%=16v", roll_logger);
    rolling_file_sink_info(txt, "%32v", roll_logger);
    roll_logger->flush();

    std::string expected_2 = fmt::format("{}  hello world!  {}", 
                                         DEFAULT_EOL , DEFAULT_EOL);
    REQUIRE(file_content(roll_sink->get_rolling_filename(2)) == expected_2);
    std::string expected_3 = fmt::format("                    hello world!{}", 
                                         DEFAULT_EOL);
    REQUIRE(file_content(roll_sink->get_rolling_filename(3)) == expected_3);
}

TEST_CASE("multithread", "[sinks]") {
    clean_test_tmp();
    size_t roll_size = 1024;
    size_t roll_num = 1024;

    std::ostringstream oss;
    auto oss_sink = std::make_shared<learnlog::sinks::ostream_sink_mt>(oss);
    auto stdout_sink = std::make_shared<learnlog::sinks::stdout_sink_mt>();
    auto stdout_color_sink = std::make_shared<learnlog::sinks::stdout_color_sink_mt>();
    auto basic_file_sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>(B_FNAME);
    auto rolling_file_sink = 
        std::make_shared<learnlog::sinks::rolling_file_sink_mt>(R_FNAME, roll_size, roll_num);

    learnlog::logger mt_logger_1("multithread test logger 1", {oss_sink,
                                                               stdout_sink,
                                                               stdout_color_sink,
                                                               basic_file_sink,
                                                               rolling_file_sink});
    mt_logger_1.set_log_level(learnlog::level::trace);
    mt_logger_1.set_pattern("");
    mt_logger_1.debug("");
    mt_logger_1.set_pattern("<%n> from thread %t");
    learnlog::logger mt_logger_2("multithread test logger 2", {oss_sink,
                                                               stdout_sink,
                                                               stdout_color_sink,
                                                               basic_file_sink,
                                                               rolling_file_sink});
    mt_logger_2.set_log_level(learnlog::level::trace);
    auto func = [&mt_logger_1, &mt_logger_2](int i) { 
        if (i % 2 == 0) mt_logger_1.debug(""); 
        else mt_logger_2.debug("");
    };

    std::vector<std::thread> threads;
    size_t thread_num = 100;
    size_t i = 0;
    for (; i < thread_num; i++) {
        threads.emplace_back(func, static_cast<int>(i));
    }

    i = 0;
    for (auto& t : threads) {
        t.join();
        i++;
    }
    REQUIRE(i == thread_num);
    mt_logger_1.flush();
    mt_logger_2.flush();

    std::istringstream iss(oss.str());
    REQUIRE(count_lines(iss) == thread_num + 1);
    
    filename_t filename(B_FNAME);
    std::string fname(filename.begin(), filename.end());
    std::ifstream ifs(fname, std::ios_base::binary);
    if (!ifs) throw std::runtime_error("Failed to open file: " + fname);
    REQUIRE(count_lines(ifs) == thread_num + 1);

    filename_t basename(R_FNAME);
    size_t total_lines = 0;
    for (size_t idx = 1; idx < roll_num; idx++) {
        filename_t rollname = rolling_file_sink->get_rolling_filename(idx);
        if ( !learnlog::base::os::dir_exist(rollname) ) break;

        std::string fname(rollname.begin(), rollname.end());
        std::ifstream ifs(fname, std::ios_base::binary);
        if (!ifs) throw std::runtime_error("Failed to open file: " + fname);
        total_lines += count_lines(ifs);
    }
    REQUIRE(total_lines == thread_num + 1);
}