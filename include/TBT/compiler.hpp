#pragma once

#include <TBT/parser.hpp>
#include <cstring>

namespace TBT {

  namespace detail {

    template <typename Variant, std::size_t... Is>
    void print_variant_types_impl(std::index_sequence<Is...>) {
      std::cout << "std::variant<";
      bool first = true;
      (([&] {
         if (!first) { std::cout << ", "; }
         first     = false;
         using Alt = std::variant_alternative_t<Is, Variant>;
         std::cout << typeid(Alt).name();
       }()),
       ...);
      std::cout << ">\n";
    }
  }  // namespace detail

  // Primary overload: print by type (compile-time only)
  template <typename Variant>
  void print_variant_types() {
    using V = std::remove_cvref_t<Variant>;
    static_assert(std::variant_size_v<V> > 0, "Type must be a non-empty std::variant specialization.");

    detail::print_variant_types_impl<V>(std::make_index_sequence<std::variant_size_v<V>>{});
  }

  namespace Compiler {

    template <class T>
    consteval size_t real_size() {
      using TT = std::decay_t<T>;
      if constexpr (std::same_as<TT, bool>)
        return 1;
      else if constexpr (std::is_arithmetic_v<TT>)
        return sizeof(TT);
      else {
        size_t out = 0;
        template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^std::decay_t<T>, std::meta::access_context::unchecked())))  //
            out += std::meta::size_of(member);
        return out;
      }
    }  // real_size

    template <class T, size_t M = real_size<T>()>
    constexpr std::array<uint8_t, M> serialize(const T& _in) {
      using TT = std::decay_t<T>;
      static_assert(std::is_default_constructible_v<TT>, "Type must be default constructible.");
      if constexpr (std::same_as<TT, bool>) {
        return std::bit_cast<std::array<uint8_t, M>>((_in ? int32_t(1) : int32_t(0)));
      } else if constexpr (std::is_arithmetic_v<TT>) {
        return std::bit_cast<std::array<uint8_t, M>>(_in);
      } else {
        std::array<uint8_t, M> out = {0};
        size_t ptr                 = 0;
        template for (constexpr auto member : std::define_static_array(
                          std::meta::nonstatic_data_members_of(^^TT, std::meta::access_context::unchecked()))) {
          constexpr size_t size = std::meta::size_of(member);
          const auto bytes      = std::bit_cast<std::array<uint8_t, size>>(_in.[:member:]);
          for (size_t k = 0; k < size; ++k, ++ptr) out[ptr] = bytes[k];
        }
        return out;
      }
    }  // serialize

    template <class T, size_t N = real_size<T>()>
    constexpr T deserialize(const std::array<uint8_t, N>& _in) {
      using TT = std::decay_t<T>;
      static_assert(std::is_default_constructible_v<TT>, "Type must be default constructible.");
      if constexpr (std::same_as<TT, bool>) {
        return (std::bit_cast<int32_t>(_in) > 0);
      } else if constexpr (std::is_arithmetic_v<TT>) {
        return std::bit_cast<TT>(_in);
      } else {
        TT out;
        size_t ptr = 0;
        template for (constexpr auto member : std::define_static_array(
                          std::meta::nonstatic_data_members_of(^^TT, std::meta::access_context::unchecked()))) {
          constexpr size_t size = std::meta::size_of(member);
          const auto& bytes     = *reinterpret_cast<const std::array<uint8_t, size>*>(&_in[ptr]);
          out.[:member:]        = std::bit_cast<decltype(out.[:member:])>(bytes);
          ptr += size;
        }
        return out;
      }
    }  // deserialize

    template <class T, class TaskVariant>
    consteval size_t get_idx_for_type() {
      constexpr auto args = std::meta::template_arguments_of(^^std::decay_t<TaskVariant>);

      for (std::size_t i = 0; i < args.size(); ++i)
        if (args[i] == ^^std::decay_t<T>) return i;

      static_assert(false, "Type T is not an alternative of Variant");
      return std::variant_npos;
    }  // get_idx_for_type

    template <typename Variant, size_t Size>
    [[nodiscard]] constexpr std::array<uint8_t, Size> compile(const std::string_view& _tree) {
      // const auto root               = Parser::root_extract(_tree);

      std::array<uint8_t, Size> out = {0};

      return out;
    }  // compile

    // [[nodiscard]] constexpr size_t compute_tree_size(const std::string_view& _tree) {
    //   //
    //   return 0;
    // }  // compute_tree_size

  }  // namespace Compiler

}  // namespace TBT