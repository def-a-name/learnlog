file(GLOB MYLOG_HELLOWORLD_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB MYLOG_HELLOWORLD_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(mylog_helloworld ${MYLOG_HELLOWORLD_SRCS})

target_include_directories(mylog_helloworld PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
                            $<BUILD_INTERFACE:${MYLOG_HEADERS_DIR}>)

spdlog_enable_warnings(mylog_helloworld)