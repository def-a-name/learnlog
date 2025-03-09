file(GLOB LEARNLOG_BASE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB LEARNLOG_CONCURRENTQUEUE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/concurrentqueue/*.h")
file(GLOB LEARNLOG_BUNDLED_FMT_HEADERS "${CMAKE_CURRENT_LIST_DIR}/fmt/*.h")
file(GLOB LEARNLOG_BUNDLED_FMT_SRCS "${CMAKE_CURRENT_LIST_DIR}/fmt/*.cc")
file(GLOB LEARNLOG_BASE_SRCS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

if (LEARNLOG_USE_BUNDLED_FMT)
    add_library(learnlog_base ${LEARNLOG_BUNDLED_FMT_SRCS} 
                              ${LEARNLOG_BASE_SRCS} 
                              ${LEARNLOG_BASE_HEADERS}
                              ${LEARNLOG_CONCURRENTQUEUE_HEADERS}
                              ${LEARNLOG_BUNDLED_FMT_HEADERS})
else()
    add_library(learnlog_base ${LEARNLOG_BASE_SRCS} 
                              ${LEARNLOG_BASE_HEADERS}
                              ${LEARNLOG_CONCURRENTQUEUE_HEADERS})
endif()

foreach(LEARNLOG_OPTION
        LEARNLOG_USE_TLS
        LEARNLOG_USE_BUNDLED_FMT)
    if(${LEARNLOG_OPTION})
        target_compile_definitions(learnlog_base PUBLIC ${LEARNLOG_OPTION})
    endif()
endforeach()

target_link_libraries(learnlog_base PUBLIC Threads::Threads)
if (NOT LEARNLOG_USE_BUNDLED_FMT)
    target_link_libraries(learnlog_base PUBLIC fmt::fmt)
endif()

target_include_directories(learnlog_base PUBLIC 
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
                           $<BUILD_INTERFACE:${LEARNLOG_HEADERS_DIR}>)

spdlog_enable_warnings(learnlog_base)