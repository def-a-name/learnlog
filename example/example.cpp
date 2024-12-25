#include "mylog_example.h"

int main(int argc, char* argv[])
{ 
    mylog::handle_exception("err");

    mylog::create_thread_pool();

    mylog::fmt_memory_buf buf;
    
    mylog::filler_helloworld(buf);
    mylog::print_fmt_buf(buf);

    buf.clear();
    mylog::pattern_formatter_helloworld(buf);
    mylog::print_fmt_buf(buf);

    buf.clear();
    mylog::custom_formatter_hello(buf);
    mylog::print_fmt_buf(buf);

    mylog::logger_helloworld();

    mylog::async_helloworld();
    

    return 0;
}
