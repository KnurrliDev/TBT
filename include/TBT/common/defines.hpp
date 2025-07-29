#pragma once

#include <algorithm>
#include <any>
#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <stack>
#include <stdfloat>
#include <string>
#include <syncstream>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <variant>
#include <vector>

namespace TBT {

  namespace Detail {
    template <typename T>
    struct TypeName {
      static constexpr const char* Get() {
        static_assert(false);
        return nullptr;
      }
    };

  }  // namespace Detail

#define ENABLE_TBT_TYPENAME(A, B)                    \
  template <>                                        \
  struct Detail::TypeName<A> {                       \
    static constexpr const char* Get() { return B; } \
  };

  // until the macro is fixed
  struct Dummy {};
  ENABLE_TBT_TYPENAME(Dummy, "Dummy")

#define EXPAND(x) x
#define EEXPAND(x) EXPAND(x)

#define SEXPAND(x) #x
#define SEEXPAND(x) SEXPAND(x)

  enum State : uint8_t { BUSY, FAILED, SUCCESS };

  enum Direction : uint8_t { UP, DOWN };

  namespace Detail {

    struct Result {
      State node_state    = SUCCESS;
      Direction direction = DOWN;
    };  // Result

    struct Node {
      int16_t type_ = 0xFFFF;
      std::shared_ptr<Node> parent_;
      std::vector<std::shared_ptr<Node>> d_;
      std::vector<std::shared_ptr<Node>> c_;
      std::vector<std::variant<bool, int32_t, float, uint32_t>> payloads_;
    };  // Node

  }  // namespace Detail
}  // namespace TBT