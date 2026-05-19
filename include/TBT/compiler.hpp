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

    enum class ParameterTypeId : int16_t {
      bool_    = 0,
      uint8_   = 1,
      int8_    = 2,
      uint16_  = 3,
      int16_   = 4,
      uint32_  = 5,
      int32_   = 6,
      uint64_  = 7,
      int64_   = 8,
      float_   = 9,
      double_  = 10,
      dynamic_ = 11
    };  // TypeId

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

    template <class TaskVariant>
    constexpr size_t get_idx_for_name(const std::string_view& _name) {
      constexpr auto args = std::meta::template_arguments_of(^^std::decay_t<TaskVariant>);
      for (std::size_t i = 0; i < args.size(); ++i)
        if (std::meta::display_string_of(args[i]) == _name) return i;

      static_assert(false, "Type T is not an alternative of Variant");
      return std::variant_npos;
    }  // get_meta_for_idx

    template <class TaskVariant>
    consteval size_t get_meta_for_idx(const size_t _idx) {
      constexpr auto args = ;

      for (std::size_t i = 0; i < args.size(); ++i)
        if (args[i] == ^^std::decay_t<T>) return i;

      static_assert(false, "Type T is not an alternative of Variant");
      return std::variant_npos;
    }  // get_idx_for_type

    consteval ParameterTypeId get_type_id_for_meta(const std::meta::info& _meta) {
      const auto canonical = std::meta::dealias(_meta);
      if (canonical == std::meta::dealias(^^bool)) return ParameterTypeId::bool_;
      if (canonical == std::meta::dealias(^^uint8_t)) return ParameterTypeId::uint8_;
      if (canonical == std::meta::dealias(^^int8_t)) return ParameterTypeId::int8_;
      if (canonical == std::meta::dealias(^^uint16_t)) return ParameterTypeId::uint16_;
      if (canonical == std::meta::dealias(^^int16_t)) return ParameterTypeId::int16_;
      if (canonical == std::meta::dealias(^^uint32_t)) return ParameterTypeId::uint32_;
      if (canonical == std::meta::dealias(^^int32_t)) return ParameterTypeId::int32_;
      if (canonical == std::meta::dealias(^^uint64_t)) return ParameterTypeId::uint64_;
      if (canonical == std::meta::dealias(^^int64_t)) return ParameterTypeId::int64_;
      if (canonical == std::meta::dealias(^^float)) return ParameterTypeId::float_;
      if (canonical == std::meta::dealias(^^double)) return ParameterTypeId::double_;
      return ParameterTypeId::dynamic_;
    }  // get_type_id_for_meta

    // header | data | padding
    // size | size | size | size | type (Default_Node, FSM_Node) | node_idx | node_idx | node_idx | node_idx |
    // default:
    // type_idx | type_idx | type_idx | type_idx | Options ... | Parameters ... | Children ... | Padding |
    // Options:
    //  value: option type | option_idx | option_idx
    //  key-value: option type | option_idx | option_idx | value type | value type | value | ...
    // Parameters:
    //  value: param type | value length | value data...
    //  key-value: param type | key length | key value ... | value length | value data...
    // Children:
    //  children count | child_ptr | child_ptr | child_ptr | child_ptr | ...
    template <typename TaskVariant>
    constexpr auto serialize_default_node = [](const std::vector<std::pair<std::string_view, int32_t>>& _option_to_idx,
                                               const Parser::Default_Node& _node) -> std::vector<uint8_t> {
      std::vector<uint8_t> out;

      // ------------------------------------------------
      // pre allocate size
      out.push_back(0);
      out.push_back(0);
      out.push_back(0);
      out.push_back(0);

      // ------------------------------------------------
      // node type (default node)
      out.push_back(0);

      // ------------------------------------------------
      // type id
      const size_t tid = get_meta_for_idx(_node.name_);
      for (const uint8_t v : std::bit_cast<std::array<uint8_t, sizeof(int32_t)>>(tid)) out.push_back(v);

      // ------------------------------------------------
      // options

      {
        // check if initializer list or key/value
        size_t il = 0, kv = 0;
        for (const auto& ov : _node.options_) {
          std::visit(overloaded{[&](const Param_Value& o) { il++; }, [&](const Param_KeyValue& o) { kv++; }}, ov);
        }

        if (il > 0 && kv > 0) {
          // TODO error
        }

        // initializer list
        if (il > 0) {
          // param counts
          const int32_t count = (int32_t)std::min(members.size(), il);
          for (const uint8_t v : std::bit_cast<std::array<uint8_t, sizeof(int32_t)>>(count)) out.push_back(v);

          const auto task_meta = std::meta::template_arguments_of(^^std::decay_t<TaskVariant>)[tid];
          const auto members   = std::meta::nonstatic_data_members_of(task_meta);
          for (int32_t i = 0; i < count; ++i) {
            const Parser::Param_Value& v = std::get<Parser::Param_Value>(_node.options_[i]);

            const int16_t pid            = get_type_id_for_meta(members[i]);
            for (const uint8_t v : std::bit_cast<std::array<uint8_t, sizeof(int16_t)>>(pid)) out.push_back(v);
          }
        }

        // key value pairs
        else {
          //
        }
      }

      // ------------------------------------------------
      // parameters

      // ------------------------------------------------
      // children

      // ------------------------------------------------
      // padding

      return out;
    };

    // header | data | padding
    // size | size | size | size | type (Default_Node, FSM_Node) | node_idx | node_idx | node_idx | node_idx |
    // FSM:
    // node_idx | node_idx | node_idx | node_idx | Options ... | Transitions ... | Padding |
    // Options:
    //  value: option type | option_idx | option_idx
    //  key-value: option type | option_idx | option_idx | value type | value type | value | ...
    // Transitions:
    //  count | from id [4] | to id [4]
    constexpr auto serialize_fsm_node = [](const Parser::FSM_Node& _node) -> std::vector<uint8_t> {
      std::vector<uint8_t> out;
      //
      return out;
    };

    template <typename Variant, size_t Size>
    [[nodiscard]] constexpr std::array<uint8_t, Size> compile(const std::string_view& _tree) {
      const auto root = Parser::root_extract(_tree);

      std::vector<std::vector<uint8_t>> data;

      // fake queue for bfs
      size_t id = 0;
      std::vector<std::pair<size_t, const Parser::Node*>> q;
      q.emplace_back(id++, &root);

      while (!q.empty()) {
        const auto [id, node] = q.back();
        q.pop_back();

        std::visit(overloaded{[&](const Parser::Default_Node& n) {
                                for (const auto& cn : n.children_) { q.emplace_back(id++, cn.get()); }
                                std::sort(q, [](const auto& [id1, n1], const auto& [id2, n2]) { return id1 > id2; });
                              },
                              [&](const Parser::Default_Node& n) {
                                //
                              }},
                   *node);
      }

      std::array<uint8_t, Size> out = {0};

      return out;
    }  // compile

    // [[nodiscard]] constexpr size_t compute_tree_size(const std::string_view& _tree) {
    //   //
    //   return 0;
    // }  // compute_tree_size

  }  // namespace Compiler

}  // namespace TBT