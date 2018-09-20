#ifndef _PIXELS_TYPES_HPP
#define _PIXELS_TYPES_HPP

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "config.hpp"
#include "eosio.hpp"

typedef uint32_t eostime;
// https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html
typedef unsigned __int128 uint128_t;

struct st_pixelOrder;
struct st_transferContext;
struct st_buyPixel_result;
struct st_pixelLocation;

struct pixel {
  uint32_t color;
  uint8_t priceCounter;
  account_name owner;

  inline bool isBlank() const { return owner == 0; }

  uint64_t currentPrice() const {
    if (isBlank()) {
      return 0;
    }

    return DEFAULT_PRICE * pow(PRICE_MULTIPLIER, priceCounter);
  }

  uint64_t nextPrice() const {
    if (isBlank()) {
      return DEFAULT_PRICE;
    }

    // Put a ceiling price on pixel. Prevents weird overflow.
    // FIXME: Unconfirmed bug. From the frontend integration test, it's possible
    // to bid up a pixel, and the total payout of the whole game is slightly
    // more.
    //
    // At 100+ EOS per pixel, the excess payout is ~0.006 EOS. But the team gets
    // paid ~14 EOS, so we can cover that excess. Attacker cannot make money
    // this way tho.
    //
    // Just to maintain sanity, we artificially limit the ceiling of a pixel
    // price to below 90 EOS.
    eosio_assert(priceCounter <= 25, "price for pixel is too high");

    return currentPrice() * PRICE_MULTIPLIER;
  }

  uint8_t nextPriceCounter() const {
    if (isBlank()) {
      return 0;
    }

    auto c = priceCounter + 1;
    eosio_assert(c > 0, "price counter overflow");

    return c;
  }
};

//@abi table account i64
struct account {
  account_name owner;

  // Including refunds, earnings, and referral fees (scaled)
  uint128_t balanceScaled;

  uint64_t pixelsDrawn;
  uint128_t maskScaled;

  uint64_t primary_key() const { return owner; }
  // asset eos_balance() const {
  //   return asset(balance / PRECISION_BASE, EOS_SYMBOL);
  // }

  EOSLIB_SERIALIZE(account, (owner)(balanceScaled)(pixelsDrawn)(maskScaled))
};

// @abi table guard i64
struct guard {
  uint64_t id;
  // Admin needs to refill this quota to enable more withdrwal
  uint64_t quota;

  uint64_t primary_key() const { return id; }

  EOSLIB_SERIALIZE(guard, (id)(quota))
};

// @abi table canvases i64
struct canvas {
  uint64_t id;
  eostime lastPaintedAt;
  eostime duration;
  account_name lastPainter;
  uint64_t pixelsDrawn;

  uint128_t maskScaled;
  uint128_t potScaled;
  uint128_t teamScaled;

  uint64_t primary_key() const { return id; }

  bool isEnded() { return now() > lastPaintedAt + duration; }

  uint128_t patronBonusScaled(const account &player) const {
    return maskScaled * player.pixelsDrawn - player.maskScaled;
  }

  EOSLIB_SERIALIZE(canvas, (id)(lastPaintedAt)(duration)(lastPainter)(
                               pixelsDrawn)(maskScaled)(potScaled)(teamScaled))
};

//@abi table pixels i64
struct pixel_row {
  uint64_t row;
  std::vector<pixel> pixels;

  uint64_t primary_key() const { return row; }

  void initialize_empty_pixels() {
    std::vector<pixel> v(PIXELS_PER_ROW);
    pixels = v;
  }

  EOSLIB_SERIALIZE(pixel_row, (row)(pixels))
};

uint64_t calculateWithdrawalAndUpdate(const canvas &cnv, account &player,
                                      guard &grd) {
  int64_t patronBonus = cnv.patronBonusScaled(player) / PRECISION_BASE;

  int64_t balance = player.balanceScaled / PRECISION_BASE;

  int64_t withdrawAmount = patronBonus + balance;
  eosio_assert(withdrawAmount > 0, "Balance too small for withdrawal");

  eosio_assert(grd.quota >= withdrawAmount,
               "Contract withdrawal quota exceeded");

  grd.quota -= withdrawAmount;

  // Find the actual truncated values, and use those to update the records
  uint128_t withdrawnBonusScaled = patronBonus * PRECISION_BASE;
  uint128_t withdrawnBalanceScaled = balance * PRECISION_BASE;

  player.balanceScaled -= withdrawnBalanceScaled;
  player.maskScaled += withdrawnBonusScaled;

  return withdrawAmount;
};

// The location of the pixel in table.
struct st_pixelLocation {
  // index into the table
  uint16_t row;
  // index into the pixels array of the row
  uint16_t col;

  st_pixelLocation(uint32_t coordinate) {
    row = coordinate / PIXELS_PER_ROW;
    col = coordinate % PIXELS_PER_ROW;
  }
};

struct st_pixelOrder {
  uint32_t coordinate;
  uint32_t color;
  uint8_t priceCounter;

  uint32_t x;
  uint32_t y;

  void parse(const std::string &memo) {
    uint64_t memoInt = stoull(memo, 0, 36);
    priceCounter = memoInt >> 52;

    coordinate = memoInt >> 32 & 0xFFFFF;
    color = memoInt & 0xFFFFFFFF;

    x = coordinate & 0x3ff;
    y = coordinate >> 10;

    eosio_assert(y < MAX_COORDINATE_Y_PLUS_ONE, "invalid y");
    eosio_assert(x < MAX_COORDINATE_X_PLUS_ONE, "invalid x");
  }

  st_pixelLocation location() const { return st_pixelLocation(coordinate); }
};

struct st_buyPixel_result {
  // Whether the buy was skipped for some reason.
  bool isSkipped;
  // Whether the buy was a blank pixel
  bool isFirstBuyer;

  // Fee (scaled) paid to the contract. Going to community pot, patron bonus,
  // and team.
  uint128_t feeScaled;

  // Value (scaled) that goes to the previous pixel owner. 0, if pixel was
  // blank.
  uint128_t ownerEarningScaled;
};

// Used to bookeep various money calculations
struct st_transferContext {
  account_name purchaser;
  account_name referrer;

  // Fund available for purchasing
  uint64_t amountLeft;  // 1 EOS = 10,000

  // Total fees collected (scaled)
  uint128_t totalFeesScaled;

  // How much is paid to the previous owners
  // uint128_t totalPaidToPreviousOwnersScaled;

  // How many pixels are painted
  uint64_t paintedPixelCount;

  uint128_t amountLeftScaled() { return amountLeft * PRECISION_BASE; }

  st_buyPixel_result purchase(const pixel &pixel,
                              const st_pixelOrder &pixelOrder) {
    auto result = st_buyPixel_result();

    auto isBlank = pixel.isBlank();
    result.isFirstBuyer = isBlank;

    if (!isBlank && pixelOrder.color == pixel.color) {
      // Pixel already the same color. Skip.
      result.isSkipped = true;
      return result;
    }

    if (!isBlank && pixelOrder.priceCounter < pixel.nextPriceCounter()) {
      // Payment too low for this pixel, maybe price had increased (somebody
      // else drew first). Skip.
      result.isSkipped = true;
      return result;
    }

    uint64_t nextPrice = pixel.nextPrice();
    eosio_assert(amountLeft >= nextPrice, "insufficient fund to buy pixel");

    if (isBlank) {
      // buying blank. The fee is the entire buy price.
      result.feeScaled = nextPrice * PRECISION_BASE;
    } else {
      // buying from another player. The fee is a percentage of the gain.
      uint64_t currentPrice = pixel.currentPrice();
      uint128_t priceIncreaseScaled =
          (nextPrice - currentPrice) * PRECISION_BASE;

      result.feeScaled = priceIncreaseScaled * FEE_PERCENTAGE / 100;
      result.ownerEarningScaled = nextPrice * PRECISION_BASE - result.feeScaled;
    }

    // bookkeeping for multiple purchases
    amountLeft -= nextPrice;
    paintedPixelCount++;
    totalFeesScaled += result.feeScaled;

    return result;
  }

  uint128_t patronBonusScaled;
  uint128_t potScaled;
  uint128_t teamScaled;
  uint128_t referralEarningScaled;

  bool hasReferrer() { return referrer != 0; }

  void updateFeesDistribution() {
    patronBonusScaled = totalFeesScaled * PATRON_BONUS_PERCENTAGE_POINTS / 100;

    potScaled = totalFeesScaled * POT_PERCENTAGE_POINTS / 100;

    referralEarningScaled = totalFeesScaled * REFERRER_PERCENTAGE_POINTS / 100;
    if (referrer == 0) {
      // if no referrer, pay the pot.
      potScaled += referralEarningScaled;
      referralEarningScaled = 0;
    }

    teamScaled =
        totalFeesScaled - patronBonusScaled - potScaled - referralEarningScaled;
  }

  uint128_t canvasMaskScaled;
  uint128_t bonusPerPixelScaled;

  void updateCanvas(canvas &cv) {
    cv.potScaled += potScaled;
    cv.teamScaled += teamScaled;
    cv.pixelsDrawn += paintedPixelCount;

    bonusPerPixelScaled = patronBonusScaled / cv.pixelsDrawn;
    eosio_assert(bonusPerPixelScaled > 0, "bonus is 0");

    cv.maskScaled += bonusPerPixelScaled;
    eosio_assert(cv.maskScaled >= bonusPerPixelScaled, "canvas mask overflow");

    canvasMaskScaled = cv.maskScaled;
  }

  void updatePurchaserAccount(account &acct) {
    if (amountLeft) {
      acct.balanceScaled += amountLeft * PRECISION_BASE;
    }

    uint128_t patronBonusScaled = bonusPerPixelScaled * paintedPixelCount;

    // FIXME: give the truncated to the team?

    uint128_t maskUpdate =
        canvasMaskScaled * paintedPixelCount - patronBonusScaled;
    acct.maskScaled += maskUpdate;

    eosio_assert(acct.maskScaled >= maskUpdate, "player mask overflow");
    acct.pixelsDrawn += paintedPixelCount;
  }
};

#endif
