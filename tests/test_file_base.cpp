#include <catch2/catch_all.hpp>
#include "base/file_base.h"
#include "file_utils.h"

#ifdef _WIN32
    #define FILENAME L"test_tmp/file_base_test.txt"
#else
    #define FILENAME "test_tmp/file_base_test.txt"
#endif

TEST_CASE("test_open", "[file_base]") {
    clean_test_tmp();
    
    mylog::filename_t fname(FILENAME);
    FILE* fp = nullptr;
    mylog::base::file_base::open(&fp, fname);
    mylog::base::file_base::close(&fp, fname);

#ifdef _WIN32
    fname += L"/invalid";
#else
    fname += "/invalid";
#endif
    REQUIRE_THROWS_AS(mylog::base::file_base::open(&fp, fname), mylog::mylog_excpt);
}

TEST_CASE("test_size", "[file_base]") {
    clean_test_tmp();
    
    mylog::filename_t fname(FILENAME);
    FILE* fp = nullptr;
    mylog::base::file_base::open(&fp, fname);
    
    write_a_ntimes(fp, fname, 202411);
    REQUIRE(mylog::base::file_base::size(fp, fname) == 202411);
    mylog::base::file_base::close(&fp, fname);

    REQUIRE(get_filesize(FILENAME) == 202411);
}

TEST_CASE("test_reopen", "[file_base]") {
    clean_test_tmp();
    
    mylog::filename_t fname(FILENAME);
    FILE* fp = nullptr;
    mylog::base::file_base::open(&fp, fname);

    write_a_ntimes(fp, fname, 11);
    mylog::base::file_base::open(&fp, fname);
    REQUIRE(mylog::base::file_base::size(fp, fname) == 11);

    mylog::base::file_base::open(&fp, fname, true);
    REQUIRE(mylog::base::file_base::size(fp, fname) == 0);
}