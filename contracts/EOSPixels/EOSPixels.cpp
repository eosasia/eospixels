#include "./EOSPixels.hpp"

#include <cmath>
#include <eosiolib/action.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

inline void unpackMemo(string memo, uint16_t &priceCounter,
                       uint32_t &coordinate, uint32_t &color) {
  uint128_t memoInt = stoull(memo, 0, 36);
  priceCounter = memoInt >> 52;
  coordinate = memoInt >> 32 & 0xFFFFF;
  color = memoInt & 0xFFFFFFFF;
}

inline uint64_t priceCounterToPrice(uint16_t priceCounter) {
  return DEFAULT_PRICE * pow(PRICE_MULTIPLIER, priceCounter) * 1E4;
}

inline void splitMemo(vector<string> &results, string memo, char separator) {
  auto start = memo.cbegin();
  auto end = memo.cend();

  for (auto it = start; it != end; ++it) {
    if (*it == separator) {
      results.emplace_back(start, it);
      start = it + 1;
    }
  }
  if (start != end) results.emplace_back(start, end);
}

template <uint64_t A, typename B, typename... C>
void clear_table(multi_index<A, B, C...> *table, uint16_t limit) {
  auto it = table->begin();
  uint16_t count = 0;
  while (it != table->end() && count < limit) {
    it = table->erase(it);
    count++;
  }
}

// template <uint64_t A, typename B, typename... C>
// void clear_table_r(multi_index<A, B, C...> *table, uint16_t limit) {
//   auto it = table->end();
//   uint16_t count = 0;
//   while (it != table->begin() && count < limit) {
//     it--;
//     it = table->erase(it);
//     count++;
//   }
// }

void eospixels::clearpixels(uint16_t count, uint16_t nonce) {
  require_auth(TEAM_ACCOUNT);

  auto itr = canvases.begin();
  eosio_assert(itr != canvases.end(), "no canvas exists");

  pixel_store pixels(_self, itr->id);
  clear_table(&pixels, count);
}

// void eospixels::clearpixelsr(uint16_t count, uint16_t nonce) {
//   require_auth(TEAM_ACCOUNT);

//   auto itr = canvases.begin();
//   eosio_assert(itr != canvases.end(), "no canvas exists");

//   pixel_store pixels(_self, itr->id);
//   clear_table_r(&pixels, count);
// }

void eospixels::clearaccts(uint16_t count, uint16_t nonce) {
  require_auth(TEAM_ACCOUNT);

  clear_table(&accounts, count);
}

void eospixels::clearcanvs(uint16_t count, uint16_t nonce) {
  require_auth(TEAM_ACCOUNT);

  clear_table(&canvases, count);
}

void eospixels::onTransfer(const currency::transfer &transfer) {
  if (transfer.to != _self) return;

  auto accountTtr = accounts.find(transfer.from);
  eosio_assert(accountTtr != accounts.end(), "account doesn't exist");

  auto canvasItr = canvases.begin();
  eosio_assert(canvasItr != canvases.end(), "game is not started yet");
  auto canvas = *canvasItr;
  eosio_assert(!canvas.isEnded(), "canvas had ended");

  std::vector<std::string> memo;
  splitMemo(memo, transfer.memo, ';');

  string orders = memo[0];

  std::vector<std::string> pixelOrders;
  splitMemo(pixelOrders, orders, ',');

  int64_t leftAmount = transfer.quantity.amount;
  int64_t commission = 0;
  pixel_store pixels(_self, canvas.id);
  pixel_store *pixelsPtr = &pixels;
  for (auto pixelOrder : pixelOrders) {
    draw(transfer.from, pixelOrder, leftAmount, pixelsPtr, commission);
  }

  if (leftAmount > 0) {
    deposit(transfer.from, asset(leftAmount, EOS_SYMBOL));
  }

  int64_t cost = transfer.quantity.amount - leftAmount;
  if (cost == 0) {
    return;
  }

  refreshLastPaintedAt();

  if (commission == 0) {
    return;
  }

  if (memo.size() == 2) {
    string referrer = memo[1];
    int64_t referrerCommission =
        REFERRER_COMMISSION_PERCENTAGE_POINTS * commission;
    if (referrerCommission > 0) {
      deposit(string_to_name(referrer.c_str()),
              asset(referrerCommission, EOS_SYMBOL));

      commission -= referrerCommission;
      if (commission == 0) {
        return;
      }
    }
  }

  deposit(TEAM_ACCOUNT, asset(commission, EOS_SYMBOL));
}

void eospixels::draw(const account_name user, string pixelOrder,
                     int64_t &amount, pixel_store *allPixels,
                     int64_t &commission) {
  uint32_t coordinate;
  uint32_t color;
  uint16_t bidPriceCounter;
  unpackMemo(pixelOrder, bidPriceCounter, coordinate, color);
  uint64_t bidPrice = priceCounterToPrice(bidPriceCounter);

  eosio_assert(bidPrice > 0 && amount >= bidPrice, "wrong price");
  eosio_assert((coordinate >> 10) < MAX_COORDINATE_Y_PLUS_ONE,
               "invalid coordinate y");
  eosio_assert((coordinate & 0x3ff) < MAX_COORDINATE_X_PLUS_ONE,
               "invalid coordinate x");

  uint16_t row = coordinate / PIXELS_PER_ROW;
  uint16_t col = coordinate - PIXELS_PER_ROW * row;
  auto pixelRowItr = allPixels->find(row);
  bool hasRow = pixelRowItr != allPixels->end();
  if (!hasRow) {
    pixelRowItr = allPixels->emplace(
        _self, [&](pixel_row &pixelRow) { pixelRow.row = row; });
  }

  auto pixels = pixelRowItr->pixels;
  auto pixel = pixels[col];
  auto hadPaintedBefore = hasRow && pixel.owner != account_name();

  if (hadPaintedBefore && color == pixel.color) {
    // unnecessary paint
    return;
  }

  uint16_t priceCounter = hadPaintedBefore ? (pixel.priceCounter) + 1 : 0;
  if (bidPriceCounter < priceCounter) {
    // payment not enough
    return;
  }

  uint64_t pixelPrice = priceCounterToPrice(priceCounter);
  // don't use bidPrice here, allow user to bid a higher price.
  amount -= pixelPrice;

  int64_t rewardForLastPaint = (1 - COMMISSION_PERCENTAGE_POINTS) * pixelPrice;
  commission += pixelPrice - rewardForLastPaint;

  allPixels->modify(pixelRowItr, 0, [&](pixel_row &pixelRow) {
    pixelRow.pixels[col] = {color, priceCounter, user};
  });

  if (hadPaintedBefore) {
    deposit(pixel.owner, asset(rewardForLastPaint, EOS_SYMBOL));
  }
}

void eospixels::end() {
  // anyone can create new canvas
  auto itr = canvases.begin();
  eosio_assert(itr != canvases.end(), "no canvas exists");

  auto c = *itr;
  eosio_assert(c.isEnded(), "canvas still has time left");

  // reclaim memory
  canvases.erase(itr);

  // create new canvas
  canvases.emplace(_self, [&](canvas &newCanvas) {
    newCanvas.id = c.id + 1;
    newCanvas.lastPaintedAt = now();
    newCanvas.duration = CANVAS_DURATION;
  });
}

void eospixels::refreshLastPaintedAt() {
  auto itr = canvases.begin();
  eosio_assert(itr != canvases.end(), "no canvas exists");

  canvases.modify(itr, 0,
                  [&](canvas &newCanvas) { newCanvas.lastPaintedAt = now(); });
}

void eospixels::refresh() {
  require_auth(TEAM_ACCOUNT);

  refreshLastPaintedAt();
}

void eospixels::changedur(time duration) {
  require_auth(TEAM_ACCOUNT);

  auto itr = canvases.begin();
  eosio_assert(itr != canvases.end(), "no canvas exists");

  canvases.modify(itr, 0,
                  [&](canvas &newCanvas) { newCanvas.duration = duration; });
}

void eospixels::createacct(const account_name account) {
  require_auth(account);

  auto itr = accounts.find(account);
  eosio_assert(itr == accounts.end(), "account already exist");

  accounts.emplace(account, [&](auto &acnt) { acnt.owner = account; });
}

void eospixels::init() {
  require_auth(_self);
  // make sure table records is empty
  eosio_assert(canvases.begin() == canvases.end(), "already initialized");

  canvases.emplace(_self, [&](canvas &newCanvas) {
    newCanvas.id = 0;
    newCanvas.lastPaintedAt = now();
    newCanvas.duration = CANVAS_DURATION;
  });
}

// void eospixels::createpxrs(uint16_t start, uint16_t end) {
//   require_auth(TEAM_ACCOUNT);

//   auto itr = canvases.begin();
//   eosio_assert(itr != canvases.end(), "no canvas exists");

//   pixel_store pixels(_self, itr->id);
//   for (uint16_t i = start; i < end; i++) {
//     pixels.emplace(
//       _self, [&](pixel_row &pixelRow) { pixelRow.row = i; });
//   }
// }

void eospixels::withdraw(const account_name to, const asset &quantity) {
  require_auth(to);

  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must withdraw positive quantity");

  auto itr = accounts.find(to);
  eosio_assert(itr != accounts.end(), "unknown account");

  accounts.modify(itr, 0, [&](auto &acnt) {
    eosio_assert(acnt.balance >= quantity, "insufficient balance");
    acnt.balance -= quantity;
  });

  action(permission_level{_self, N(active)}, N(eosio.token), N(transfer),
         std::make_tuple(_self, to, quantity,
                         std::string("Withdraw from EOS Pixels")))
      .send();
}

void eospixels::deposit(const account_name user, const asset &quantity) {
  eosio_assert(quantity.is_valid(), "invalid quantity");
  eosio_assert(quantity.amount > 0, "must deposit positive quantity");

  auto itr = accounts.find(user);
  if (itr == accounts.end()) {
    // in the case of invalid referrer, we deposit commission to team account
    itr = accounts.find(TEAM_ACCOUNT);
    eosio_assert(itr != accounts.end(), "team account is not exist");
  }

  accounts.modify(itr, 0, [&](auto &acnt) { acnt.balance += quantity; });
}

void eospixels::apply(account_name contract, action_name act) {
  if (contract == N(eosio.token) && act == N(transfer)) {
    // React to transfer notification.
    // DANGER: All methods MUST check whethe token symbol is acceptable.

    auto transfer = unpack_action_data<currency::transfer>();
    eosio_assert(transfer.quantity.symbol == EOS_SYMBOL,
                 "must pay with EOS token");
    onTransfer(transfer);
    return;
  }

  if (contract != _self) return;

  // needed for EOSIO_API macro
  auto &thiscontract = *this;
  switch (act) {
    // first argument is name of CPP class, not contract
    EOSIO_API(eospixels, (init)(refresh)(changedur)(end)(createacct)(withdraw)(
                             clearpixels)(clearaccts)(clearcanvs))
  };
}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  eospixels pixels(receiver);
  pixels.apply(code, action);
  eosio_exit(0);
}
}
