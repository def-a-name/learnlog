#include "learnlog_example.h"

int main(int argc, char* argv[])
{ 
    learnlog::handle_exception("err");

    learnlog::fmt_memory_buf buf;
    
    learnlog::filler_helloworld(buf);
    learnlog::print_fmt_buf(buf);

    buf.clear();
    learnlog::pattern_formatter_helloworld(buf);
    learnlog::print_fmt_buf(buf);

    buf.clear();
    learnlog::custom_formatter_hello(buf);
    learnlog::print_fmt_buf(buf);

    learnlog::logger_helloworld();

    learnlog::async_helloworld();
    

    return 0;
}
