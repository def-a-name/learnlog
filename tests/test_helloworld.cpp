#include <catch2/catch_all.hpp>
#include "helloworld/helloworld.h"

TEST_CASE("hello world", "[helloworld]")
{
    // int a[2] = {1, 2};
    // CHECK(a[2] == 2);

    // int* a = new int[2];
    // delete[] a;
    // CHECK(a[0]);

    hello_world();
}