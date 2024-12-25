file(GLOB SINKS_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB FORMATTERS_HEADERS "${CMAKE_CURRENT_LIST_DIR}/formatters/*.h")
set(LEARNLOG_SINKS_HEADERS ${SINKS_HEADERS} ${FORMATTERS_HEADERS})

file(GLOB SINKS_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB FORMATTERS_SRCS "${CMAKE_CURRENT_LIST_DIR}/formatters/*.cpp")
set(LEARNLOG_SINKS_SRCS ${SINKS_SRCS} ${FORMATTERS_SRCS})

add_library(learnlog_sinks ${LEARNLOG_SINKS_SRCS} ${LEARNLOG_SINKS_HEADERS})

target_include_directories(learnlog_sinks PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
                            $<BUILD_INTERFACE:${LEARNLOG_HEADERS_DIR}>)

target_link_libraries(learnlog_sinks PUBLIC fmt::fmt)

spdlog_enable_warnings(learnlog_sinks)