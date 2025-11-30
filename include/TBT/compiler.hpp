#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/strtonum/StrToNum.hpp>
#include <glaze/glaze.hpp>

/*
  Grammar:
    Hierarchy: Task($n, ...)[Task($n, ...), ...]
      special node: root if no custom root node
                    root[Task($n, ...)[Task($n, ...), ...], Task($n, ...)]

    Behaviour Tree: sequence[Task($n, ...), Task($n, ...)]

    State Machine: $[$n: Task($n, ...), ..., $n0->$n1:Task($n, ...), ...]
      $n:     State
      $n->$n: transition

    $[State Machine]
    sequence[BT]
*/

namespace TBT::Compiler {

  // constexpr int16_t vidx_root           = -1;
  // constexpr int16_t vidx_sequence       = -2;
  // constexpr int16_t vidx_fallback       = -3;
  // constexpr int16_t vidx_nloop          = -4;
  // constexpr int16_t vidx_cooldown       = -5;
  // constexpr int16_t vidx_always_fail    = -6;
  // constexpr int16_t vidx_always_succeed = -7;

  constexpr uint8_t pt_bool  = 0b00000001;  // bool type
  constexpr uint8_t pt_int   = 0b00000010;  // int type
  constexpr uint8_t pt_float = 0b00000100;  // float type
  constexpr uint8_t pt_dyn   = 0b00001000;  // dynamic type

  template <typename Variant>
  consteval auto variant_type_index_name_pairs() {
    using V            = std::remove_cvref_t<Variant>;
    constexpr size_t N = std::variant_size_v<V>;
    return []<size_t... I>(std::index_sequence<I...>) consteval {
      using tuple_t = std::pair<size_t, std::string_view>;
      return std::array<tuple_t, N>{{tuple_t{I, Detail::TypeName<std::variant_alternative_t<I, V>>::Get()}...}};
    }(std::make_index_sequence<N>{});
  }  // variant_type_index_name_pairs

  template <class T>
  consteval size_t real_size() {
    using TT = std::decay_t<T>;
    static_assert(!std::is_pointer_v<T>, "Pointer not supported.");
    static_assert(!(std::is_arithmetic_v<TT> && sizeof(TT) > 4), "Only 4 byte types supported.");
    if constexpr (std::same_as<TT, Parameter>) {
      return sizeof(uint8_t) + sizeof(size_t);
    } else if constexpr (std::same_as<TT, bool>) {
      return sizeof(int32_t);
    } else if constexpr (std::is_arithmetic_v<T>) {
      return sizeof(TT);
    } else {
      T dummy;
      auto tie         = glz::to_tie(dummy);
      constexpr auto N = glz::reflect<TT>::size;
      size_t out       = 0;
      for (size_t i = 0; i < N; ++i) {
        using Tuple                   = std::decay_t<decltype(tie)>;
        constexpr auto indices        = std::make_index_sequence<glz::tuple_size_v<Tuple>>{};
        constexpr auto runtime_getter = glz::detail::tuple_runtime_getter<Tuple>(indices);
        const auto field              = runtime_getter[i](tie);
        std::visit(
            [&](const auto& val) {
              using Field_t = std::remove_pointer_t<std::decay_t<decltype(val)>>;
              out += sizeof(Field_t);
            },
            field);
      }
      return out;
    }
  }  // real_size

  template <class T, size_t M = real_size<T>>
  constexpr std::array<uint8_t, M> serialize(const T& _in) {
    using TT = std::decay_t<T>;
    static_assert(!std::is_pointer_v<T>, "Pointer not supported.");
    static_assert(!(std::is_arithmetic_v<TT> && sizeof(TT) > 4), "Only 4 byte types supported.");
    if constexpr (std::same_as<TT, bool>) {
      return std::bit_cast<std::array<uint8_t, M>>((_in ? int32_t(1) : int32_t(0)));
    } else if constexpr (std::is_arithmetic_v<TT>) {
      return std::bit_cast<std::array<uint8_t, M>>(_in);
    } else {
      constexpr auto N = glz::reflect<TT>::size;
      auto tie         = glz::to_tie(_in);
      std::array<uint8_t, M> out;
      for (size_t i = 0; i < M; ++i) out[i] = (uint8_t)0x0;
      size_t j = 0;
      for (size_t i = 0; i < N; ++i) {
        using Tuple                   = std::decay_t<std::decay_t<decltype(tie)>>;
        constexpr auto indices        = std::make_index_sequence<glz::tuple_size_v<Tuple>>{};
        constexpr auto runtime_getter = glz::detail::tuple_runtime_getter<Tuple>(indices);
        const auto field              = runtime_getter[i](tie);
        std::visit(
            [&](const auto& val) {
              using Field_t = std::remove_pointer_t<std::decay_t<decltype(val)>>;
              static_assert(std::is_trivially_copyable_v<Field_t>);
              const auto bytes = std::bit_cast<std::array<uint8_t, sizeof(Field_t)>>(*val);
              for (size_t k = 0; k < sizeof(Field_t); ++k, ++j) out[j] = bytes[k];
            },
            field);
      }
      return out;
    }
  };  // serialize

  template <class T, size_t N = real_size<T>>
  constexpr T deserialize(const std::array<uint8_t, N>& _in) {
    using TT = std::decay_t<T>;
    static_assert(!std::is_pointer_v<T>, "Pointer not supported.");
    static_assert(!(std::is_arithmetic_v<TT> && sizeof(TT) > 4), "Only 4 byte types supported.");
    if constexpr (std::same_as<TT, bool>) {
      return (std::bit_cast<int32_t>(_in) > 0);
    } else if constexpr (std::is_arithmetic_v<TT>) {
      return std::bit_cast<TT>(_in);
    } else {
      constexpr auto M = glz::reflect<TT>::size;
      T out;
      auto tie = glz::to_tie(out);
      size_t j = 0;
      for (size_t i = 0; i < M; ++i) {
        using Tuple                   = std::decay_t<decltype(tie)>;
        constexpr auto indices        = std::make_index_sequence<glz::tuple_size_v<Tuple>>{};
        constexpr auto runtime_getter = glz::detail::tuple_runtime_getter<Tuple>(indices);
        auto field                    = runtime_getter[i](tie);
        std::visit(
            [&](auto& val) {
              using Field_t = std::remove_pointer_t<std::decay_t<decltype(val)>>;
              static_assert(std::is_trivially_copyable_v<Field_t>);
              std::array<uint8_t, sizeof(Field_t)> tmp;
              for (size_t ii = 0; ii < sizeof(Field_t); ++ii) tmp[ii] = _in[j + ii];
              *val = std::bit_cast<Field_t>(tmp);
              j += sizeof(Field_t);
            },
            field);
      }
      return out;
    }
  };  // deserialize

  struct Composite {
    uintptr_t ptr_   = 0;
    int16_t cur_idx_ = 0;
  };  // Composite

  struct Result {
    // constexpr static size_t real_size_ = real_size<Result>();
    State state_   = State::SUCCESS;
    Direction dir_ = Direction::DOWN;
  };  // Result

  struct Header {
    // constexpr static size_t real_size_ = real_size<Header>();
    uint32_t node_count_        = 0;
    uint32_t ptr_               = 0;

    uint16_t children_count_    = 0;
    uint32_t first_node_offset_ = 0;

    Result last_result_;

    uint16_t child_idx_ = 0;
    // Result last_result_;
  };  // Header

  struct NodeHeader {
    // constexpr static size_t real_size_ = real_size<NodeHeader>();
    int16_t type_idx_         = 0;
    uint32_t parent_          = 0;

    uint32_t children_offset_ = 0;
    uint16_t children_count_  = 0;

    uint32_t params_offset_   = 0;
    uint16_t params_count_    = 0;

    uint32_t comp_offset_     = 0;

    uint32_t node_size_       = 0;
  };  // NodeHeader

  namespace RealSize {
    constexpr size_t composite   = real_size<Composite>();
    constexpr size_t result      = real_size<Result>();
    constexpr size_t header      = real_size<Header>();
    constexpr size_t node_header = real_size<NodeHeader>();
  }  // namespace RealSize

  inline constexpr std::array<uint8_t, RealSize::composite> serialize_composite(const Composite& _in) {
    return serialize<Composite, RealSize::composite>(_in);
  }  // serialize_composite

  inline constexpr Composite deserialize_composite(const std::array<uint8_t, RealSize::composite>& _in) {
    return deserialize<Composite, RealSize::composite>(_in);
  }  // deserialize_composite

  inline constexpr std::array<uint8_t, RealSize::result> serialize_result(const Result& _in) {
    return serialize<Result, RealSize::result>(_in);
  }  // serialize_result

  inline constexpr Result deserialize_result(const std::array<uint8_t, RealSize::result>& _in) {
    return deserialize<Result, RealSize::result>(_in);
  }  // deserialize_composite

  inline constexpr std::array<uint8_t, RealSize::header> serialize_header(const Header& _in) {
    return serialize<Header, RealSize::header>(_in);
  }  // serialize_header

  inline constexpr Header deserialize_header(const std::array<uint8_t, RealSize::header>& _in) {
    return deserialize<Header, RealSize::header>(_in);
  }  // deserialize_composite

  inline constexpr std::array<uint8_t, RealSize::node_header> serialize_node_header(const NodeHeader& _in) {
    return serialize<NodeHeader, RealSize::node_header>(_in);
  }  // serialize_node_header

  inline constexpr NodeHeader deserialize_node_header(const std::array<uint8_t, RealSize::node_header>& _in) {
    return deserialize<NodeHeader, RealSize::node_header>(_in);
  }  // deserialize_composite

  //--------------------------------------------------

  inline constexpr Header read_global_node_header(std::span<const uint8_t> _tree) {
    std::array<uint8_t, RealSize::header> tmp;
    for (size_t i = 0; i < RealSize::header; ++i) tmp[i] = _tree[i];
    return deserialize_header(tmp);
  }  // read_global_node_header

  inline constexpr void write_global_node_header(const Header& _val, std::span<uint8_t> _tree) {
    const auto res = serialize_header(_val);
    for (size_t i = 0; i < res.size(); ++i) _tree[i] = res[i];
  }  // write_global_node_header

  //----------------------------------

  inline constexpr NodeHeader read_node_header(std::span<const uint8_t> _node) {
    std::array<uint8_t, RealSize::node_header> tmp;
    for (size_t i = 0; i < RealSize::node_header; ++i) tmp[i] = _node[i];
    return deserialize_node_header(tmp);
  }  // read_node_header

  inline constexpr void write_node_header(const NodeHeader& _val, std::span<uint8_t> _node) {
    const auto res = serialize_node_header(_val);
    for (size_t i = 0; i < res.size(); ++i) _node[i] = res[i];
  }  // write_node_header

  //----------------------------------

  inline constexpr uint32_t read_root_child(const int32_t& _i, std::span<const uint8_t> _tree) {
    std::array<uint8_t, sizeof(uint32_t)> tmp;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) tmp[i] = _tree[RealSize::header + _i * sizeof(uint32_t) + i];
    return deserialize<uint32_t, sizeof(uint32_t)>(tmp);
  }  // read_root_child

  inline constexpr void write_root_child(const int32_t& _i, const uint32_t& _ptr, std::span<uint8_t> _tree) {
    const auto res = serialize<uint32_t, sizeof(uint32_t)>(_ptr);
    for (size_t i = 0; i < res.size(); ++i) _tree[RealSize::header + _i * sizeof(uint32_t) + i] = res[i];
  }  // write_root_child

  //----------------------------------

  inline constexpr uint32_t read_child(const int32_t& _i, std::span<const uint8_t> _node) {
    std::array<uint8_t, sizeof(uint32_t)> tmp;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) tmp[i] = _node[RealSize::node_header + _i * sizeof(uint32_t) + i];
    return deserialize<uint32_t, sizeof(uint32_t)>(tmp);
  }  // read_child

  inline constexpr void write_child(const int32_t& _i, const uint32_t& _ptr, std::span<uint8_t> _node) {
    const auto res = serialize<uint32_t, sizeof(uint32_t)>(_ptr);
    for (size_t i = 0; i < res.size(); ++i) _node[RealSize::node_header + _i * sizeof(uint32_t) + i] = res[i];
  }  // write_child

  //----------------------------------

  inline constexpr Composite read_composite(const NodeHeader& _header, std::span<const uint8_t> _node) {
    std::array<uint8_t, RealSize::composite> tmp;
    for (size_t i = 0; i < RealSize::composite; ++i) tmp[i] = _node[_header.comp_offset_ + i];
    return deserialize_composite(tmp);
  }  // read_composite

  inline constexpr void write_composite(const Composite& _val, const NodeHeader& _header, std::span<uint8_t> _node) {
    const auto res = serialize_composite(_val);
    for (size_t i = 0; i < res.size(); ++i) _node[_header.comp_offset_ + i] = res[i];
  }  // write_composite

  //----------------------------------

  inline constexpr Parameter read_payload(const size_t& _i, const NodeHeader& _header, std::span<const uint8_t> _node) {
    constexpr int32_t size = sizeof(uint8_t) + sizeof(int32_t);

    const uint8_t type     = _node[_header.params_offset_ + _i * size];

    std::array<uint8_t, sizeof(int32_t)> tmp;
    for (size_t i = 0; i < sizeof(int32_t); ++i)
      tmp[i] = _node[_header.params_offset_ + _i * size + sizeof(uint8_t) + i];

    if (type == pt_bool)
      return deserialize<int32_t, sizeof(int32_t)>(tmp) > 0;
    else if (type == pt_int)
      return deserialize<int32_t, sizeof(int32_t)>(tmp);
    else if (type == pt_float)
      return deserialize<float, sizeof(int32_t)>(tmp);
    else if (type == pt_dyn)
      return deserialize<uint32_t, sizeof(int32_t)>(tmp);

    std::unreachable();
  }  // read_payload

  inline constexpr void write_payload(const size_t& _i, const NodeHeader& _header, const Parameter& _param,
                                      std::span<uint8_t> _node) {
    constexpr int32_t size = sizeof(uint8_t) + sizeof(int32_t);
    switch (_param.index()) {
      case 0: {
        _node[_header.params_offset_ + _i * size] = pt_bool;
        const auto res = serialize<int32_t, sizeof(int32_t)>((std::get<bool>(_param) ? 1 : 0));
        for (size_t i = 0; i < sizeof(int32_t); ++i)
          _node[_header.params_offset_ + _i * size + sizeof(uint8_t) + i] = res[i];
        break;
      }
      case 1: {
        _node[_header.params_offset_ + _i * size] = pt_int;
        const auto res                            = serialize<int32_t, sizeof(int32_t)>(std::get<int32_t>(_param));
        for (size_t i = 0; i < sizeof(int32_t); ++i)
          _node[_header.params_offset_ + _i * size + sizeof(uint8_t) + i] = res[i];
        break;
      }
      case 2: {
        _node[_header.params_offset_ + _i * size] = pt_float;
        const auto res                            = serialize<float, sizeof(int32_t)>(std::get<float>(_param));
        for (size_t i = 0; i < sizeof(int32_t); ++i)
          _node[_header.params_offset_ + _i * size + sizeof(uint8_t) + i] = res[i];
        break;
      }
      case 3: {
        _node[_header.params_offset_ + _i * size] = pt_dyn;
        const auto res                            = serialize<uint32_t, sizeof(int32_t)>(std::get<uint32_t>(_param));
        for (size_t i = 0; i < sizeof(int32_t); ++i)
          _node[_header.params_offset_ + _i * size + sizeof(uint8_t) + i] = res[i];
        break;
      }
    }
  }  // write_payload

#define F_SPLIT                                                                                           \
  const auto split = [](const std::string_view& _s, const char _delim) -> std::vector<std::string_view> { \
    std::vector<std::string_view> result;                                                                 \
    size_t start = 0;                                                                                     \
    size_t end   = _s.find(_delim);                                                                       \
                                                                                                          \
    while (end != std::string::npos && end < _s.size()) {                                                 \
      if (start == end)                                                                                   \
        result.emplace_back();                                                                            \
      else                                                                                                \
        result.push_back({&_s[start], end - start});                                                      \
      start = end + 1;                                                                                    \
      end   = _s.find(_delim, start);                                                                     \
    }                                                                                                     \
                                                                                                          \
    if (start == _s.size())                                                                               \
      result.emplace_back();                                                                              \
    else                                                                                                  \
      result.push_back({&_s[start], _s.size() - start});                                                  \
    return result;                                                                                        \
  };

#define F_CLEAN                                           \
  const auto clean = [](std::string _in) -> std::string { \
    size_t j = 0;                                         \
    for (size_t i = 0; i < _in.size(); ++i) {             \
      switch (_in[i]) {                                   \
        case 0x20: /* space ' ' */                        \
        case 0x0c: /* form feed '\f' */                   \
        case 0x0a: /* line feed '\n' */                   \
        case 0x0d: /* carriage return '\r' */             \
        case 0x09: /* horizontal tab '\t' */              \
        case 0x0b: /* vertical tab '\v' */                \
          continue;                                       \
        default:                                          \
          _in[j++] = _in[i];                              \
      }                                                   \
    }                                                     \
    _in.resize(j);                                        \
    return _in;                                           \
  };

#define F_PARAMS                                                                                     \
  const auto extract_params = [&](const std::string_view& _s) -> std::vector<Parameter> {            \
    std::vector<Parameter> out;                                                                      \
                                                                                                     \
    const auto parts = split(_s, ',');                                                               \
                                                                                                     \
    for (const auto& r : parts) {                                                                    \
      if (r.empty()) continue;                                                                       \
                                                                                                     \
      if (r[0] == '$') {                                                                             \
        const std::optional<uint32_t> val = stn::StrToUInt32(std::string_view(&r[1], r.size() - 1)); \
        if (val) { out.push_back(val.value()); }                                                     \
        continue;                                                                                    \
      } else if (r == "true") {                                                                      \
        out.push_back(true);                                                                         \
        continue;                                                                                    \
      } else if (r == "false") {                                                                     \
        out.push_back(false);                                                                        \
        continue;                                                                                    \
      }                                                                                              \
      if (r.contains('.') || r.contains('f')) {                                                      \
        const std::optional<float> val = stn::StrToFloat(r);                                         \
        if (val) { out.push_back(val.value()); }                                                     \
      } else {                                                                                       \
        const std::optional<int32_t> val = stn::StrToInt32(r);                                       \
        if (val) { out.push_back(val.value()); }                                                     \
      }                                                                                              \
    }                                                                                                \
    return out;                                                                                      \
  };

#define H_EXTRACT                                                                                                 \
  const auto h_extract = [&](std::string_view s) -> std::expected<std::vector<Node>, std::string_view> {          \
    std::vector<Node> nodes;                                                                                      \
    /* {level, parent_id} */                                                                                      \
    std::vector<std::pair<int32_t, int32_t>> stack = {{0, 0}};                                                    \
    int32_t next_id                                = 1;                                                           \
    size_t i                                       = 0;                                                           \
                                                                                                                  \
    const auto parse_node_name                     = [&]() -> std::expected<std::string_view, std::string_view> { \
      size_t start = i;                                                                       \
      while (i < s.size()) {                                                                  \
        char c = s[i];                                                                        \
        if (c == '(' || c == '[' || c == ']' || c == ',') break;                              \
        ++i;                                                                                  \
      }                                                                                       \
      if (i == start) return std::unexpected("empty node name");                              \
      return s.substr(start, i - start);                                                      \
    };                                                                                                            \
                                                                                                                  \
    const auto parse_params = [&]() -> std::vector<Parameter> {                                                   \
      if (i >= s.size() || s[i] != '(') return {};                                                                \
      ++i; /* skip '(' */                                                                                         \
      int32_t depth = 1;                                                                                          \
      size_t start  = i;                                                                                          \
      while (i < s.size() && depth > 0) {                                                                         \
        if (s[i] == '(')                                                                                          \
          ++depth;                                                                                                \
        else if (s[i] == ')')                                                                                     \
          --depth;                                                                                                \
        ++i;                                                                                                      \
      }                                                                                                           \
      return extract_params(s.substr(start, i - start - 1));                                                      \
    };                                                                                                            \
                                                                                                                  \
    const auto get_idx_for_type = [&](const std::string_view& _ss) -> std::optional<size_t> {                     \
      const auto t = _ss.substr(0, _ss.find_first_of('('));                                                       \
      for (size_t i = 0; i < variant.size(); ++i)                                                                 \
        if (variant[i].second == t) return variant[i].first;                                                      \
      return std::nullopt;                                                                                        \
    };                                                                                                            \
                                                                                                                  \
    while (i < s.size()) {                                                                                        \
      /* Parse node name */                                                                                       \
      auto name_res = parse_node_name();                                                                          \
      if (!name_res) return std::unexpected(name_res.error());                                                    \
      std::string_view name = name_res.value();                                                                   \
                                                                                                                  \
      Node n;                                                                                                     \
      n.cl_      = name;                                                                                          \
      n.node_id_ = next_id++;                                                                                     \
      n.level_   = stack.back().first;                                                                            \
      n.parent_  = stack.back().second;                                                                           \
                                                                                                                  \
      auto idx   = get_idx_for_type(name);                                                                        \
      if (!idx) return std::unexpected(name);                                                                     \
      n.type_idx_ = (int32_t)idx.value();                                                                         \
                                                                                                                  \
      /* Parse optional parameters */                                                                             \
      if (i < s.size() && s[i] == '(') { n.p_ = parse_params(); }                                                 \
                                                                                                                  \
      nodes.push_back(std::move(n));                                                                              \
                                                                                                                  \
      if (i >= s.size()) break;                                                                                   \
                                                                                                                  \
      char c = s[i];                                                                                              \
                                                                                                                  \
      if (c == '[') {                                                                                             \
        stack.push_back({stack.back().first + 1, n.node_id_});                                                    \
        ++i;                                                                                                      \
      } else if (c == ']') {                                                                                      \
        stack.pop_back();                                                                                         \
        ++i;                                                                                                      \
        /* Handle possible consecutive ]]... by continuing */                                                     \
        while (i < s.size() && s[i] == ']') {                                                                     \
          stack.pop_back();                                                                                       \
          ++i;                                                                                                    \
        }                                                                                                         \
        /* After closing brackets, expect either ',' or end or another node */                                    \
        if (i < s.size() && s[i] == ',') ++i;                                                                     \
      } else if (c == ',') {                                                                                      \
        ++i;                                                                                                      \
      } else {                                                                                                    \
        return std::unexpected(s.substr(i, 10)); /* invalid char */                                               \
      }                                                                                                           \
    }                                                                                                             \
                                                                                                                  \
    return nodes;                                                                                                 \
  };

  struct Node {
    int32_t node_id_  = 0;
    int32_t type_idx_ = 0;
    int32_t level_    = 0;
    int32_t parent_   = 0;
    std::vector<Parameter> p_;
    std::string_view cl_;
    //---------------------
    uint32_t offset_ = 0;
    uint32_t size_   = 0;
  };  // Node

  template <class Variant>
  constexpr size_t compute_size_static(const std::string_view& _s) {
    F_SPLIT
    F_CLEAN
    F_PARAMS

    constexpr auto variant = variant_type_index_name_pairs<Variant>();

    H_EXTRACT

    const auto gather_children = [](const int32_t _parent, const std::vector<Node>& _nodes) -> std::vector<int32_t> {
      std::vector<int32_t> out;
      for (const auto& n : _nodes)
        if (n.parent_ == _parent) out.push_back(n.node_id_);
      return out;
    };

    // clean input
    const std::string s      = clean(std::string(_s));

    // extract nodes
    const auto hh            = h_extract({s.data(), s.size()});

    const auto nodes         = hh.value();

    //----------------------------------------------------
    // ...|header|node header|children|params|composite|...

    const auto root_children = gather_children(0, nodes);

    size_t out               = RealSize::header + root_children.size() * sizeof(uint32_t);

    for (const Node& n : nodes) {
      const auto children = gather_children(n.node_id_, nodes);
      out += RealSize::node_header + children.size() * sizeof(int32_t) + n.p_.size() * (1 + sizeof(int32_t)) +
             RealSize::composite;
    }

    return out;
  };  // compute_size_static

  template <class Variant, class Array>
  constexpr void compile(const std::string_view& _s, Array& _vals) {
    F_SPLIT
    F_CLEAN
    F_PARAMS

    constexpr auto variant = variant_type_index_name_pairs<Variant>();

    H_EXTRACT

    const auto gather_children = [](const int32_t _parent, const std::vector<Node>& _nodes) -> std::vector<int32_t> {
      std::vector<int32_t> out;
      for (const auto& n : _nodes)
        if (n.parent_ == _parent) out.push_back(n.node_id_);
      return out;
    };

    // clean input
    const std::string s      = clean(std::string(_s));

    // extract nodes
    const auto hh            = h_extract({s.data(), s.size()});

    auto nodes               = hh.value();

    //----------------------------------------------------
    // ...|header|node header|children|params|composite|...

    const auto root_children = gather_children(0, nodes);

    Header header;
    header.node_count_        = static_cast<decltype(header.node_count_)>(nodes.size());
    header.ptr_               = 0;
    header.children_count_    = static_cast<decltype(header.children_count_)>(root_children.size());
    header.first_node_offset_ = RealSize::header + header.children_count_ * sizeof(uint32_t);

    write_global_node_header(header, {_vals.begin(), _vals.end()});

    uint32_t ptr = header.first_node_offset_;
    for (Node& n : nodes) {
      NodeHeader nheader;
      nheader.type_idx_        = static_cast<decltype(nheader.type_idx_)>(n.type_idx_);
      nheader.parent_          = n.parent_;

      const auto children      = gather_children(n.node_id_, nodes);
      nheader.children_count_  = static_cast<decltype(nheader.children_count_)>(children.size());
      nheader.children_offset_ = static_cast<decltype(nheader.children_offset_)>(RealSize::node_header);

      nheader.params_count_    = static_cast<decltype(nheader.params_count_)>(n.p_.size());
      nheader.params_offset_   = nheader.children_offset_ + nheader.children_count_ * sizeof(int32_t);

      nheader.comp_offset_     = nheader.params_offset_ + nheader.params_count_ * (1 + sizeof(int32_t));

      nheader.node_size_       = nheader.comp_offset_ + RealSize::composite;

      n.offset_                = ptr;
      n.size_                  = nheader.node_size_;

      write_node_header(nheader, {_vals.data() + ptr, nheader.node_size_});

      for (size_t i = 0; i < n.p_.size(); ++i)
        write_payload(i, nheader, n.p_[i], {_vals.data() + ptr, nheader.node_size_});

      Composite cmp;
      cmp.cur_idx_ = 0;
      cmp.ptr_     = 0;

      write_composite(cmp, nheader, {_vals.data() + ptr, nheader.node_size_});

      ptr += nheader.node_size_;
    }

    // link root children
    int32_t cc = 0;
    for (const int32_t id : root_children) {
      for (const Node& n : nodes) {
        if (id == n.node_id_) {
          write_root_child(cc, n.offset_, {_vals.begin(), _vals.end()});
          break;
        }
      }
      cc++;
    }

    // link children and parent
    for (const Node& n : nodes) {
      // parent
      if (n.parent_ != 0) {
        NodeHeader tar = read_node_header({_vals.data() + n.offset_, n.size_});
        for (const auto& nc : nodes) {
          if (nc.node_id_ == n.parent_) {
            tar.parent_ = nc.offset_;
            break;
          }
        }
        write_node_header(tar, {_vals.data() + n.offset_, tar.node_size_});
      }

      // children
      const auto children = gather_children(n.node_id_, nodes);
      for (size_t i = 0; i < children.size(); ++i) {
        // find node with id
        for (const auto& nc : nodes) {
          if (nc.node_id_ == children[i]) {
            write_child(int32_t(i), nc.offset_, {_vals.data() + n.offset_, _vals.size() - n.offset_});
            break;
          }
        }
      }
    }
  }  // compile

  template <class Variant>
  [[nodiscard]] DynamicTree compile_dynamic(const std::string_view& _s) {
    std::vector<uint8_t> out;
    out.resize(compute_size_static<Variant>(_s), 0x0);
    compile<Variant>(_s, out);
    return out;
  }  // compile_dynamic

  template <size_t S, class Variant>
  consteval StaticTree<S> compile_static(const std::string_view& _s) {
    std::array<uint8_t, S> out;
    for (size_t i = 0; i < S; ++i) out[i] = 0x0;
    compile<Variant>(_s, out);
    return out;
  }  // compile_static

}  // namespace TBT::Compiler