#pragma once

#include "config.hpp"
#include "types.hpp"

class TestContext {
 public:
  uint64_t totalPaid;
  uint128_t totalPaidToOwnerScaled;
  uint128_t totalReferralPaidScaled;

  canvas cnv;
  std::vector<account> accounts;

  TestContext(size_t naccounts)
      : totalPaid(0), totalPaidToOwnerScaled(0), cnv(), accounts(naccounts) {}

  uint128_t payoutsScaled() {
    uint128_t allPatronBonus = 0;

    for (auto& account : accounts) {
      allPatronBonus += cnv.patronBonusScaled(account);
    }

    return cnv.potScaled + cnv.teamScaled + allPatronBonus +
           totalPaidToOwnerScaled;
  }

  void validatePayouts(uint64_t tolerance = 10) {
    uint64_t totalPayouts = payoutsScaled() / PRECISION_BASE;
    REQUIRE(totalPaid >= totalPayouts);
    REQUIRE(totalPaid - totalPayouts <= tolerance);
  }

  void buyPixels(TestContext& tt, size_t npixels, size_t gen, account& player,
                 account_name referrer = 0) {
    auto ctx = st_transferContext{};
    ctx.referrer = referrer;

    // a blank pixel
    auto pxl = pixel{};
    if (gen != 0) {
      pxl.owner = N(somebody);
      pxl.priceCounter = gen - 1;
    } else {
      pxl.owner = 0;
      pxl.priceCounter = 0;
    }
    pxl.color = 0xff0000ff;

    uint64_t purchasePrice = pxl.nextPrice();
    uint64_t purchaseAmount = npixels * purchasePrice;
    tt.totalPaid += purchaseAmount;
    ctx.amountLeft = purchaseAmount;
    ctx.purchaser = player.owner;

    // buying a non-blank pixel (first take over)
    auto pxlOrder = st_pixelOrder{};
    pxlOrder.color = 0x00ff00ff;
    pxlOrder.priceCounter = gen;

    // buy n blank pixels
    for (size_t i = 0; i < npixels; i++) {
      auto result = ctx.purchase(pxl, pxlOrder);

      tt.totalPaidToOwnerScaled += result.ownerEarningScaled;
    }

    ctx.updateFeesDistribution();

    totalReferralPaidScaled = ctx.referralEarningScaled;

    ctx.updateCanvas(cnv);
    ctx.updatePurchaserAccount(player);
  }
};
