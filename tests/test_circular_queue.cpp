#include <catch2/catch_all.hpp>
#include "base/circular_queue.h"

using q_type = learnlog::base::circular_queue<int>;
TEST_CASE("test_construct", "[circular_queue]")
{
    q_type q1(9);
    q_type q2(q1);
    q_type q3(std::move(q1));
    q_type q4(q3);

    REQUIRE(q4.vec_size() == 10);
    REQUIRE(q3.vec_size() == 10);
    REQUIRE(q2.vec_size() == 10);
    REQUIRE(q1.vec_size() == 0);
    // CHECK(q1.front() == 0);
}

TEST_CASE("test_size", "[circular_queue]") {
    q_type q1(0);
    q1.push_back(1);
    REQUIRE(q1.empty());

    q_type q2(6);
    for(int i = 0; i<6; i++) {
        q2.push_back(std::move(i));
    }
    REQUIRE(q2.full());
    
    q2.push_back(6);
    REQUIRE(q2.full());
    REQUIRE(q2.size() == 6);
}

TEST_CASE("test_rolling", "[circular_queue]") {
    q_type q(6);

    for(int i = 0; i < 6+6; i++) {
        q.push_back(std::move(i));
    }

    REQUIRE(q.full());

    for(int i=6; i<12; i++){
        REQUIRE(q.front() == i);
        q.pop_front();
    }

    REQUIRE(q.empty());
    // CHECK(q.front() == 0);
}