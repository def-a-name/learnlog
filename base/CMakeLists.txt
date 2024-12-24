file(GLOB MYLOG_BASE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB MYLOG_BASE_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(mylog_base ${MYLOG_BASE_SRCS} ${MYLOG_BASE_HEADERS})

target_include_directories(mylog_base PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
                            $<BUILD_INTERFACE:${MYLOG_HEADERS_DIR}>)

target_link_libraries(mylog_base PUBLIC Threads::Threads)
target_link_libraries(mylog_base PUBLIC fmt::fmt)

spdlog_enable_warnings(mylog_base)