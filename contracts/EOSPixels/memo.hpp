#include <string>
#include <vector>

#include "eosio.hpp"
#include "types.hpp"

void splitMemo(std::vector<std::string>& results, const std::string& memo,
               char separator) {
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

class TransferMemo {
 public:
  std::vector<st_pixelOrder> pixelOrders;
  account_name referrer;

  void parse(const std::string& memo) {
    std::vector<std::string> memoParts;
    splitMemo(memoParts, memo, ';');

    if (memoParts.size() == 2) {
      referrer = string_to_name(memoParts[1].c_str());
    } else {
      referrer = 0;
    }

    const auto& ordersStr = memoParts[0];

    std::vector<std::string> pixelOrderParts;
    splitMemo(pixelOrderParts, ordersStr, ',');

    pixelOrders = std::vector<st_pixelOrder>(pixelOrderParts.size());

    size_t i = 0;
    for (auto& pixelOrderPart : pixelOrderParts) {
      auto& order = pixelOrders[i];
      order.parse(pixelOrderPart);
      i++;
    }
  }
};
