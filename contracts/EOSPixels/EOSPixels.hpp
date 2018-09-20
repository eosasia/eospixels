#include <eosiolib/currency.hpp>
#include <eosiolib/eosio.hpp>
#include <vector>

#include "config.hpp"
#include "types.hpp"

#define EOS_SYMBOL S(4, EOS)  // MainNet and TestNet use EOS

using namespace eosio;

class eospixels : public contract {
 public:
  eospixels(account_name self)
      : contract(self),
        canvases(self, self),
        accounts(self, self),
        guards(self, self) {}

  // the first argument of multi_index must be the name of the table
  // in the ABI!
  typedef multi_index<N(canvases), canvas> canvas_store;
  typedef multi_index<N(pixels), pixel_row> pixel_store;
  typedef multi_index<N(account), account> account_store;
  typedef multi_index<N(guard), guard> guard_store;

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
  void withdraw(const account_name to);
  /// @abi action
  void clearpixels(uint16_t count, uint16_t nonce);
  /// @abi action
  void clearaccts(uint16_t count, uint16_t nonce);
  /// @abi action
  void clearcanvs(uint16_t count, uint16_t nonce);
  /// @abi action
  void resetquota();

  void apply(account_name contract, action_name act);

 private:
  canvas_store canvases;
  account_store accounts;
  guard_store guards;

  bool isValidReferrer(account_name name);

  void deposit(const account_name user, const uint128_t quantityScaled);

  void drawPixel(pixel_store& allPixels, const st_pixelOrder& pixelOrder,
                 st_transferContext& ctx);
  void refreshLastPaintedAt();
};
