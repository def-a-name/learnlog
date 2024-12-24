cmake_minimum_required(VERSION 3.11)
project(mylog_utests LANGUAGES CXX)

if(NOT TARGET mylog)
    # Stand-alone build
    find_package(mylog REQUIRED)
endif()

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(systemd libsystemd)
endif()

find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
    message(STATUS "Packaged version of Catch will be used.")
else()
    message(STATUS "Bundled version of Catch will be downloaded and used.")
    include(FetchContent)
    FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 53d0d913a422d356b23dd927547febdf69ee9081 # v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

set(MYLOG_UTEST_HELLOWORLD_SOURCES
    test_helloworld.cpp
)

set(MYLOG_UTEST_BASE_SOURCES
    test_circular_queue.cpp
    test_mpmc_queue.cpp
    test_handle_exception.cpp
    test_fmt_base.cpp
    test_file_base.cpp
    test_periodic_function.cpp
)

set(MYLOG_UTEST_SINKS_SOURCES
    test_filler.cpp
    test_pattern_formatter.cpp
    test_sinks.cpp
    test_backtracer.cpp
)

set(MYLOG_UTEST_INTERFACE_SOURCES
    test_interface.cpp
    test_async_logger.cpp
)

enable_testing()

function(mylog_prepare_test test_target test_srcs mylog_lib)
    add_executable(${test_target} ${test_srcs})
    target_link_libraries(${test_target} PRIVATE ${mylog_lib})
    if(systemd_FOUND)
        target_link_libraries(${test_target} PRIVATE ${systemd_LIBRARIES})
    endif()
    target_link_libraries(${test_target} PRIVATE Catch2::Catch2WithMain)
    if(MYLOG_SANITIZE_ADDRESS)
        spdlog_enable_sanitizer(${test_target})
    endif()
    add_test(NAME ${test_target} COMMAND ${test_target})
    set_tests_properties(${test_target} PROPERTIES RUN_SERIAL ON)
    spdlog_enable_warnings(${test_target})
endfunction()

if(MYLOG_BUILD_TESTS)
    mylog_prepare_test(mylog-utests-helloworld "${MYLOG_UTEST_HELLOWORLD_SOURCES}" mylog_helloworld)
    
    mylog_prepare_test(mylog-utests-base "${MYLOG_UTEST_BASE_SOURCES}" mylog_base)

    mylog_prepare_test(mylog-utests-sinks "${MYLOG_UTEST_SINKS_SOURCES}" mylog)

    mylog_prepare_test(mylog-utests-interface "${MYLOG_UTEST_INTERFACE_SOURCES}" mylog)
endif()
