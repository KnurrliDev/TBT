#pragma once

#include <algorithm>
#include <any>
#include <array>
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
#include <expected>
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

  using Parameter = std::variant<bool, int32_t, float, uint32_t>;
  template <size_t S>
  using StaticTree  = std::array<uint8_t, S>;
  using DynamicTree = std::vector<uint8_t>;

  namespace Detail {
    template <typename T>
    struct TypeName {
      static constexpr const char* Get() {
        static_assert(false);
        return nullptr;
      }
    };

  }  // namespace Detail

  // helper type for the visitor #4
  template <class... Ts>
  struct overloaded : Ts... {
    using Ts::operator()...;
  };
  // explicit deduction guide (not needed as of C++20)
  template <class... Ts>
  overloaded(Ts...) -> overloaded<Ts...>;

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

  template <typename Variant>
  consteval auto variant_type_index_name_pairs() {
    using V            = std::remove_cvref_t<Variant>;
    constexpr size_t N = std::variant_size_v<V>;
    return []<size_t... I>(std::index_sequence<I...>) consteval {
      using tuple_t = std::pair<size_t, std::string_view>;
      return std::array<tuple_t, N>{{tuple_t{I, Detail::TypeName<std::variant_alternative_t<I, V>>::Get()}...}};
    }(std::make_index_sequence<N>{});
  }  // variant_type_index_name_pairs
}  // namespace TBT