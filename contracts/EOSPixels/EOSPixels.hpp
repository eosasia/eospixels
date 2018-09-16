#include <string>

#include <eosiolib/currency.hpp>
#include <eosiolib/eosio.hpp>

#define EOS_SYMBOL S(4, EOS)  // MainNet and TestNet use EOS
#define DEFAULT_PRICE 0.05
#define PRICE_MULTIPLIER 1.35
#define CANVAS_DURATION 60 * 60 * 24 * 7
#define MAX_COORDINATE_X_PLUS_ONE 1000
#define MAX_COORDINATE_Y_PLUS_ONE 1000
#define PIXELS_PER_ROW 256
#define COMMISSION_PERCENTAGE_POINTS 0.1
#define REFERRER_COMMISSION_PERCENTAGE_POINTS 0.02
#define TEAM_ACCOUNT N(ceoimondev11)

// https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html
typedef unsigned __int128 uint128_t;

using namespace eosio;

class eospixels : public contract {
 public:
  eospixels(account_name self)
      : contract(self), canvases(self, self), accounts(self, self) {}

  // @abi table canvases i64
  struct canvas {
    uint64_t id;
    uint64_t lastPaintedAt;
    uint32_t duration;

    uint64_t primary_key() const { return id; }

    bool isEnded() { return now() > lastPaintedAt + duration; }

    EOSLIB_SERIALIZE(canvas, (id)(lastPaintedAt)(duration))
  };

  struct pixel {
    uint32_t color;
    uint16_t priceCounter;
    account_name owner;
  };

  //@abi table pixels i64
  struct pixel_row {
    uint64_t row;
    vector<pixel> pixels;

    uint64_t primary_key() const { return row; }

    pixel_row() {
      vector<pixel> v(PIXELS_PER_ROW);
      pixels = v;
    }

    EOSLIB_SERIALIZE(pixel_row, (row)(pixels))
  };

  //@abi table account i64
  struct account {
    account(const account_name o = account_name(),
            const asset& quantity = asset(0LL, EOS_SYMBOL))
        : owner(o), balance(quantity) {}

    account_name owner;
    asset balance;

    bool is_empty() const { return !balance.amount; }

    uint64_t primary_key() const { return owner; }

    EOSLIB_SERIALIZE(account, (owner)(balance))
  };

  // the first argument of multi_index must be the name of the table
  // in the ABI!
  typedef multi_index<N(canvases), canvas> canvas_store;
  typedef multi_index<N(pixels), pixel_row> pixel_store;
  typedef multi_index<N(account), account> account_store;

  void onTransfer(const currency::transfer& transfer);
  /// @abi action
  void init();
  /// @abi action
  void refresh();
  /// @abi action
  void changedur(time duration);
  /// @abi action
  void end();
  /// @abi action
  void createacct(const account_name account);
  /// @abi action
  void withdraw(const account_name to, const asset& quantity);
  /// @abi action
  void clearpixels(uint16_t count, uint16_t nonce);
  /// @abi action
  void clearaccts(uint16_t count, uint16_t nonce);
  /// @abi action
  void clearcanvs(uint16_t count, uint16_t nonce);

  void apply(account_name contract, action_name act);

 private:
  canvas_store canvases;
  account_store accounts;

  void deposit(const account_name user, const asset& quantity);
  void draw(const account_name user, string pixelOrder, int64_t& amount,
            pixel_store* pixels, int64_t& commission);
  void refreshLastPaintedAt();
};
