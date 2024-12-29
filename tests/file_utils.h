#pragma once

#include "definitions.h"
#include "base/file_base.h"
#ifdef _WIN32
    #include "win.h"
    #include "base/wchar_support.h"
#endif

#include <fstream>

void clean_test_tmp() {
#ifdef _WIN32
    system("rmdir /S /Q test_tmp");
#else
    if (system("rm -rf test_tmp") != 0) {
        throw std::runtime_error("Failed to rm -rf test_tmp");
    }
#endif
}

learnlog::u_long_long get_filesize(const learnlog::filename_t& filename) {
#ifdef _WIN32
    std::ifstream ifs(learnlog::base::wstring_to_string(filename),
                      std::ifstream::ate | std::ifstream::binary);
#else
    std::ifstream ifs(filename, 
                      std::ifstream::ate | std::ifstream::binary);
#endif
    if (!ifs) {
        throw std::runtime_error("Failed to create ifstream");
    }

    return static_cast<learnlog::u_long_long>(ifs.tellg());
}

size_t count_lines(std::istream& istream_in) {
    size_t cnt = 0;
    std::string line;
    while (std::getline(istream_in, line)) {
        cnt++;
    }
    return cnt;
}

std::string file_content(const learnlog::filename_t& filename) {
    std::string fname(filename.begin(), filename.end());
    std::ifstream ifs(fname, std::ios_base::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open file: " + fname);
    }
    return std::string((std::istreambuf_iterator<char>(ifs)), 
                       (std::istreambuf_iterator<char>()));
}

static void write_a_ntimes(FILE* fp, const learnlog::filename_t& filename, size_t ntimes) {
    learnlog::fmt_memory_buf buf;
    fmt::format_to(std::back_inserter(buf), FMT_STRING("{}"), std::string(ntimes, 'a'));
    learnlog::base::file_base::write(fp, filename, buf);
    learnlog::base::file_base::flush(fp, filename);
}