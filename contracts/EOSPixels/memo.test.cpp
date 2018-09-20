#include "catch.hpp"

#include "memo.hpp"

unsigned int Factorial(unsigned int number) {
  return number <= 1 ? number : Factorial(number - 1) * number;
}

TEST_CASE("Can parse memo referrer", "[memo]") {
  auto memo = TransferMemo();
  REQUIRE(memo.referrer == 0);

  memo.parse("0,0;howard");
  REQUIRE(memo.referrer == N(howard));

  memo.parse("0,0");
  REQUIRE(memo.referrer == 0);
}

TEST_CASE("Can parse memo pixel orders", "[memo]") {
  auto memoStr =
      "k5k36s7pxb,k5k55tbrwf,k5k74uftvj,k5k93vjvun,k5kb2wnxtr,k5kd1xrzsv,"
      "k5kf0yw1rz,k5kh0003r3";

  auto memo = TransferMemo();
  memo.parse(memoStr);

  REQUIRE(memo.pixelOrders.size() == 8);

  size_t i = 0;
  for (auto& order : memo.pixelOrders) {
    REQUIRE(order.priceCounter == 0);
    REQUIRE(order.y == 465);
    REQUIRE(order.x == 416 + i);
    REQUIRE(order.color == 0x222222ff);
    i++;
  }
}
