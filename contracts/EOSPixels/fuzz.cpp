#include <iostream>

#include "config.hpp"
#include "types.hpp"

#include "printu128.hpp"

uint64_t totalPaid = 0;
uint128_t totalPaidToOwnerScaled = 0;

void buyNonBlankPixels(size_t npixels, canvas& cnv, account& player) {
  auto ctx = st_transferContext{};

  // a blank pixel
  auto pxl = pixel{};
  pxl.owner = N(somebody);
  pxl.color = 0xff0000ff;
  pxl.priceCounter = 0;

  uint64_t purchasePrice = pxl.nextPrice();
  uint64_t purchaseAmount = npixels * purchasePrice;
  totalPaid += purchaseAmount;
  ctx.amountLeft = purchaseAmount;
  ctx.purchaser = player.owner;

  // buying a non-blank pixel (first take over)
  auto pxlOrder = st_pixelOrder{};
  pxlOrder.color = 0x00ff00ff;
  pxlOrder.priceCounter = 1;

  // buy n blank pixels
  for (size_t i = 0; i < npixels; i++) {
    auto result = ctx.purchase(pxl, pxlOrder);
    totalPaidToOwnerScaled += result.ownerEarningScaled;
  }

  ctx.updateFeesDistribution();
  ctx.updateCanvas(cnv);
  ctx.updatePurchaserAccount(player);
}

void buyBlankPixels(size_t npixels, canvas& cnv, account& player) {
  auto ctx = st_transferContext{};

  uint64_t purchaseAmount = npixels * DEFAULT_PRICE;
  totalPaid += purchaseAmount;
  ctx.amountLeft = purchaseAmount;
  ctx.purchaser = player.owner;

  // a blank pixel
  auto pxl = pixel{};

  // buying a blank pixel
  auto pxlOrder = st_pixelOrder{};
  pxlOrder.color = 0xff0000ff;

  // buy n blank pixels
  for (size_t i = 0; i < npixels; i++) {
    ctx.purchase(pxl, pxlOrder);
  }

  ctx.updateFeesDistribution();

  // printf("ctx.paintedPixelCount: %llu\n", ctx.paintedPixelCount);

  // printf("ctx.totalFeesScaled: ");
  // print_u128(ctx.totalFeesScaled);
  // printf("\n");

  // printf("ctx patronBonuses scaled: ");
  // print_u128(ctx.patronBonusesScaled);
  // printf("\n");

  ctx.updateCanvas(cnv);
  ctx.updatePurchaserAccount(player);
}

double eos(uint64_t value) { return (double)value / 1e4; }

int main() {
  auto cnv = canvas{};

  auto player1 = account{};
  player1.owner = N(player1);

  auto player2 = account{};
  player2.owner = N(player2);

  auto player3 = account{};
  player3.owner = N(player3);

  auto player4 = account{};
  player4.owner = N(player4);

  auto player5 = account{};
  player5.owner = N(player5);

  // buyBlankPixels(100, cnv, player1);
  // buyBlankPixels(100, cnv, player2);
  // buyBlankPixels(1000, cnv, player2);

  // initial layer
  // for (size_t i = 0; i < 10; i++) {
  //   buyBlankPixels(100, cnv, player1);
  // }

  // if i buy 1000 blank pixels (50 EOS), how many more pixels bought till i
  // break even?

  // first guy to buy 1000 pixels, make it back from patronBonuses when 2000 more
  // pixels sold
  // for (size_t i = 0; i < 10; i++) {
  //   buyBlankPixels(100, cnv, player1);
  //   // breaks even: ~3000 shares
  // }

  // for (size_t i = 0; i < 10; i++) {
  //   buyBlankPixels(100, cnv, player2);
  //   // breaks even: ~6000 shares
  // }

  // for (size_t i = 0; i < 10; i++) {
  //   buyBlankPixels(100, cnv, player3);
  //   // breaks even: ~12000 shares
  // }

  // for (size_t i = 0; i < 10; i++) {
  //   buyBlankPixels(100, cnv, player4);
  //   // breaks even: ~12000 shares
  // }

  for (size_t i = 0; i < 1e6; i += 100) {
    buyBlankPixels(100, cnv, player5);
  }

  // buyBlankPixels(100, cnv, player1);
  // buyBlankPixels(1000, cnv, player2);
  // buyBlankPixels(1000, cnv, player3);

  // buyBlankPixels(1000, cnv, player2);
  // buyBlankPixels(1000, cnv, player3);
  // buyBlankPixels(1000, cnv, player3);

  // second layer
  // buyNonBlankPixels(1000, cnv, player2);

  // first paint over
  // for (size_t i = 0; i < 100; i++) {
  //   buyNonBlankPixels(100, cnv, player2);
  // }

  printf("canvas shares: %llu\n", cnv.shares);

  printf("player1 patronBonuses: ");
  print_u128(eos(cnv.playerpatronBonusesScaled(player1) / PRECISION_BASE));
  printf("\n");

  printf("player2 patronBonuses: ");
  print_u128(eos(cnv.playerpatronBonusesScaled(player2) / PRECISION_BASE));
  printf("\n");

  printf("player3 patronBonuses: ");
  print_u128(eos(cnv.playerpatronBonusesScaled(player3) / PRECISION_BASE));
  printf("\n");

  printf("player4 patronBonuses: ");
  print_u128(eos(cnv.playerpatronBonusesScaled(player4) / PRECISION_BASE));
  printf("\n");

  // everybody buys 20 EOS (400 pixels)
  // 250 players
  // how much does everyone make?

  // printf("player5 patronBonuses: ");
  // print_u128(eos(cnv.playerpatronBonusesScaled(player5) / PRECISION_BASE));
  // printf("\n");

  // printf("total patronBonuses : ");
  // print_u128(eos(cnv.maskScaled * cnv.shares / PRECISION_BASE));
  // printf("\n");

  printf("team earning: ");
  print_u128(eos(cnv.teamScaled / PRECISION_BASE));
  printf("\n");

  printf("pot: ");
  print_u128(eos(cnv.potScaled / PRECISION_BASE));
  printf("\n");

  printf("paid to owners (EOS): %.4f\n",
         eos(totalPaidToOwnerScaled / PRECISION_BASE));

  uint128_t allPayouts =
      cnv.playerpatronBonusesScaled(player1) + cnv.playerpatronBonusesScaled(player2) +
      cnv.playerpatronBonusesScaled(player3) + cnv.playerpatronBonusesScaled(player4) +
      cnv.playerpatronBonusesScaled(player5) + totalPaidToOwnerScaled +
      cnv.teamScaled + cnv.potScaled;

  printf("all payouts (EOS): %.4f\n", eos(allPayouts / PRECISION_BASE));

  printf("total paid (EOS): %.4f\n", eos(totalPaid));

  // printf("painted pixels: ");
  // print_u128(ctx.paintedPixelCount);
  // printf("\n");

  // printf("amount left: ");
  // printf("%llu", ctx.amountLeft);
  // printf("\n");

  // printf("total fees: ");
  // print_u128(ctx.totalFeesScaled / PRECISION_BASE);
  // printf("\n");

  // std::cout << "hello" << std::endl;
  // uint128_t a = 2;

  // for (size_t i = 0; i < 130; i++) {
  //   a *= 2;
  //   print_u128(a);
  //   printf("\n");
  // }
  // // std::cout << a << std::endl;
  // print_u128(a);
}
