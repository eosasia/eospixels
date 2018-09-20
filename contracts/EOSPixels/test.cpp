#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helper.hpp"

#include "memo.test.cpp"

SCENARIO("game economics") {
  WHEN("a player buys 1000 blank pixels") {
    auto tt = TestContext{1};

    auto& cnv = tt.cnv;
    auto& player1 = tt.accounts[0];

    tt.buyPixels(tt, 1000, 0, player1);

    REQUIRE(tt.cnv.pixelsDrawn == 1000);
    REQUIRE(tt.totalPaid == 500000);

    REQUIRE(cnv.potScaled / PRECISION_BASE == 165000);
    REQUIRE(cnv.teamScaled / PRECISION_BASE == 135000);
    REQUIRE(tt.totalReferralPaidScaled == 0);
    tt.validatePayouts();
  }

  WHEN("a player buys pixels with referral") {
    auto tt = TestContext{1};

    auto& cnv = tt.cnv;
    auto& player1 = tt.accounts[0];

    tt.buyPixels(tt, 1000, 0, player1, N(aReferrer));

    REQUIRE((uint64_t)(tt.totalReferralPaidScaled / PRECISION_BASE) == 40000);
    REQUIRE(cnv.potScaled / PRECISION_BASE == 125000);
    REQUIRE(cnv.teamScaled / PRECISION_BASE == 135000);
  }

  WHEN("many players buy 150k blank pixels") {
    auto tt = TestContext{250};
    auto& cnv = tt.cnv;

    auto& accounts = tt.accounts;

    for (auto& account : accounts) {
      size_t npixels = 600;
      for (size_t i = 0; i < npixels; i += 50) tt.buyPixels(tt, 50, 0, account);
    }

    REQUIRE(tt.cnv.pixelsDrawn == 150e3);
    REQUIRE(tt.totalPaid == DEFAULT_PRICE * 150e3);
    tt.validatePayouts();

    THEN("early players should earn more patronBonuses") {
      for (size_t i = 1; i < accounts.size(); i++) {
        auto& accountA = accounts[i - 1];
        auto& accountB = accounts[i];

        REQUIRE(cnv.patronBonusScaled(accountA) != 0);
        REQUIRE(cnv.patronBonusScaled(accountA) >
                cnv.patronBonusScaled(accountB));
      }
    }
  }

  WHEN("players buy 1 pixel at a time") {
    auto tt = TestContext{2};

    auto& player1 = tt.accounts[0];
    auto& player2 = tt.accounts[1];

    for (size_t i = 0; i < 500e3; i++) {
      tt.buyPixels(tt, 1, 0, player1);
    }

    for (size_t i = 0; i < 500e3; i++) {
      tt.buyPixels(tt, 1, 0, player2);
    }

    REQUIRE(tt.totalPaid == 1e6 * DEFAULT_PRICE);
    tt.validatePayouts();
  }

  WHEN("player withdraw patronBonuses and balance") {
    auto tt = TestContext{2};
    auto grd = guard{};

    uint64_t quotaStart = 1000 * 1e4;
    grd.quota = quotaStart;

    uint128_t initialBalanceScaled = 1000 * PRECISION_BASE;

    auto& player1 = tt.accounts[0];
    player1.balanceScaled = initialBalanceScaled;

    tt.buyPixels(tt, 10e3, 0, player1);

    auto patronBonusesScaled = tt.cnv.patronBonusScaled(player1);

    uint64_t withdrawAmount =
        calculateWithdrawalAndUpdate(tt.cnv, player1, grd);

    REQUIRE(
        (uint64_t)(withdrawAmount - (patronBonusesScaled + initialBalanceScaled) /
                                        PRECISION_BASE) == 0);

    REQUIRE(tt.cnv.patronBonusScaled(player1) == 0);
    REQUIRE(player1.balanceScaled == 0);
    REQUIRE(quotaStart - withdrawAmount == grd.quota);
  }

  WHEN("player bids up pixel price to very high") {
    auto tt = TestContext{2};

    auto& player1 = tt.accounts[0];

    for (size_t i = 0; i < 25; i++) {
      tt.buyPixels(tt, 1, i, player1);
    }

    // printf("total paid: %llu\n", tt.totalPaid / (uint64_t)1e4);
    // printf("total paid owner: %llu\n",
    //        (uint64_t)(tt.totalPaidToOwnerScaled / PRECISION_BASE / 1e4));
    REQUIRE(tt.totalPaidToOwnerScaled > 0);
    tt.validatePayouts();
  }
}
