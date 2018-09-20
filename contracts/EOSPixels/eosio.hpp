// poorman's eoslib. minimal definitions to allow some files to compile both as
// smart contract and as normal cpp program

#ifndef _PIXEL_EOSIO
#define _PIXEL_EOSIO

#ifdef GCC
#include <cassert>
#define eosio_assert(A, B) assert((A) && B)

/**
 * @brief Name of an account
 * @details Name of an account
 */
typedef uint64_t account_name;

/**
 *  Converts a base32 symbol into its binary representation, used by
 * string_to_name()
 *
 *  @brief Converts a base32 symbol into its binary representation, used by
 * string_to_name()
 *  @param c - Character to be converted
 *  @return constexpr char - Converted character
 *  @ingroup types
 */
static constexpr char char_to_symbol(char c) {
  if (c >= 'a' && c <= 'z') return (c - 'a') + 6;
  if (c >= '1' && c <= '5') return (c - '1') + 1;
  return 0;
}

/**
 *  Converts a base32 string to a uint64_t. This is a constexpr so that
 *  this method can be used in template arguments as well.
 *
 *  @brief Converts a base32 string to a uint64_t.
 *  @param str - String representation of the name
 *  @return constexpr uint64_t - 64-bit unsigned integer representation of the
 * name
 *  @ingroup types
 */
static constexpr uint64_t string_to_name(const char* str) {
  uint32_t len = 0;
  while (str[len]) ++len;

  uint64_t value = 0;

  for (uint32_t i = 0; i <= 12; ++i) {
    uint64_t c = 0;
    if (i < len && i <= 12) c = uint64_t(char_to_symbol(str[i]));

    if (i < 12) {
      c &= 0x1f;
      c <<= 64 - 5 * (i + 1);
    } else {
      c &= 0x0f;
    }

    value |= c;
  }

  return value;
}

// This EOS definition conflcits with std::time
// typedef uint32_t time;
uint32_t now() { return 0; }

#define N(X) string_to_name(#X)
#define EOSLIB_SERIALIZE(A, B)
#else
#include <eosiolib/eosio.hpp>
#endif

#endif
