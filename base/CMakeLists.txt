file(GLOB LEARNLOG_BASE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB LEARNLOG_BASE_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(learnlog_base ${LEARNLOG_BASE_SRCS} ${LEARNLOG_BASE_HEADERS})

target_include_directories(learnlog_base PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
                            $<BUILD_INTERFACE:${LEARNLOG_HEADERS_DIR}>)

target_link_libraries(learnlog_base PUBLIC Threads::Threads)
target_link_libraries(learnlog_base PUBLIC fmt::fmt)

spdlog_enable_warnings(learnlog_base)