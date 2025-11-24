#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/strtonum/StrToNum.hpp>

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

#pragma pack(push, 1)

  struct Composite {
    uintptr_t ptr_   = 0;
    int16_t cur_idx_ = 0;
  };  // Composite

  struct Result {
    State state_   = State::SUCCESS;
    Direction dir_ = Direction::DOWN;
  };  // Result

  struct Header {
    uint32_t node_count_;
    uint32_t ptr_;

    uint16_t children_count_    = 0;
    uint32_t first_node_offset_ = 0;

    Result last_result_;

    uint16_t child_idx_ = 0;
    // Result last_result_;
  };  // Header

  struct NodeHeader {
    int16_t type_idx_         = 0;
    uint32_t parent_          = 0;

    uint32_t children_offset_ = 0;
    uint16_t children_count_  = 0;

    uint32_t params_offset_   = 0;
    uint16_t params_count_    = 0;

    uint32_t comp_offset_     = 0;

    uint32_t node_size_       = 0;
  };  // NodeHeader

#pragma pack(pop)

  template <class T, class U>
  T read(const U* _ptr) {
    T result;
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    memcpy((void*)&result, (void*)_ptr, sizeof(T));
    return result;
  }  // read

  template <class T, class U>
  void write(const T& _val, U* _ptr) {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    memcpy((void*)_ptr, (void*)&_val, sizeof(T));
  }  // read

  inline NodeHeader read_node_header(const uint8_t* _node) { return read<NodeHeader>(_node); }  // read_node_header

  inline void write_global_node_header(const Header& _val, uint8_t* _node) {
    write(_val, _node);
  }  // write_global_node_header

  inline Header read_global_node_header(const uint8_t* _node) {
    return read<Header>(_node);
  }  // read_global_node_header

  inline uint32_t read_root_child(const int32_t& _i, const uint8_t* _node) {
    return read<uint32_t>(_node + sizeof(Header) + _i * sizeof(uint32_t));
  }  // read_child

  inline uint32_t read_child(const int32_t& _i, const NodeHeader& _header, const uint8_t* _node) {
    return read<uint32_t>(_node + _header.children_offset_ + _i * sizeof(uint32_t));
  }  // read_child

  inline Composite read_composite(const NodeHeader& _header, const uint8_t* _node) {
    return read<Composite>(_node + _header.comp_offset_);
  }  // read_child

  inline void write_composite(const Composite& _val, const NodeHeader& _header, uint8_t* _node) {
    write<Composite>(_val, _node + _header.comp_offset_);
  }  // read_child

  union Payload {
    char v1[4];
    int32_t v2;
    float v3;
    uint32_t v4;
  };

  [[nodiscard]] inline Parameter read_payload(const int32_t& _i, const NodeHeader& _header, const uint8_t* _node) {
    constexpr int32_t size = sizeof(uint8_t) + sizeof(int32_t);

    const uint8_t type     = read<uint8_t>(_node + _header.params_offset_ + _i * size);
    const Payload val      = read<Payload>(_node + _header.params_offset_ + _i * size + sizeof(uint8_t));

    if (type == (pt_bool))
      return val.v1[0] != 0;
    else if (type == (pt_int))
      return val.v2;
    else if (type == (pt_float))
      return val.v3;
    else if (type == (pt_dyn))
      return val.v4;

    std::unreachable();
  }  // read_payload
  // clang-format off
#define F_SPLIT                                                           \
const auto split = [](const std::string_view& _s, const char _delim) ->   \
  std::vector<std::string_view> {                                         \
  std::vector<std::string_view> result;                                   \
  size_t start = 0;                                                       \
  size_t end   = _s.find(_delim);                                         \
                                                                          \
  while (end != std::string::npos && end < _s.size()) {                   \
    if (start == end)                                                     \
      result.emplace_back();                                              \
    else                                                                  \
      result.push_back({&_s[start], end - start});                        \
    start = end + 1;                                                      \
    end   = _s.find(_delim, start);                                       \
  }                                                                       \
                                                                          \
  if (start == _s.size())                                                 \
    result.emplace_back();                                                \
  else                                                                    \
    result.push_back({&_s[start], _s.size()  - start});                   \
  return result;                                                          \
};

#define F_CLEAN                                                           \
const auto clean = [](std::string _in) -> std::string {                   \
    size_t j = 0;                                                         \
    for (size_t i = 0; i < _in.size(); ++i) {                             \
      switch (_in[i]) {                                                   \
        case 0x20: /* space ' ' */                                        \
        case 0x0c: /* form feed '\f' */                                   \
        case 0x0a: /* line feed '\n' */                                   \
        case 0x0d: /* carriage return '\r' */                             \
        case 0x09: /* horizontal tab '\t' */                              \
        case 0x0b: /* vertical tab '\v' */                                \
          continue;                                                       \
        default:                                                          \
          _in[j++] = _in[i];                                              \
      }                                                                   \
    }                                                                     \
    _in.resize(j);                                                        \
    return _in;                                                           \
  };

  #define F_PARAMS                                                        \
   const auto extract_params = [&](const std::string_view& _s)            \
    -> std::vector<Parameter> {                                           \
    std::vector<Parameter> out;                                           \
                                                                          \
    const auto parts = split(_s, ',');                                    \
                                                                          \
    for (const auto& r : parts) {                                         \
      if (r.empty()) continue;                                            \
                                                                          \
      if (r[0] == '$') {                                                  \
        const std::optional<uint32_t> val =                               \
          stn::StrToUInt32(std::string_view(&r[1], r.size() - 1));        \
        if (val) { out.push_back(val.value()); }                          \
        continue;                                                         \
      } else if (r == "true") {                                           \
        out.push_back(true);                                              \
        continue;                                                         \
      } else if (r == "false") {                                          \
        out.push_back(false);                                             \
        continue;                                                         \
      }                                                                   \
      if (r.contains('.') || r.contains('f')) {                           \
        const std::optional<float> val = stn::StrToFloat(r);              \
        if (val) { out.push_back(val.value()); }                          \
      } else {                                                            \
        const std::optional<int32_t> val = stn::StrToInt32(r);            \
        if (val) { out.push_back(val.value()); }                          \
      }                                                                   \
    }                                                                     \
    return out;                                                           \
  };

  #define H_EXTRACT                                                                   \
  const auto h_extract  = [&](std::string_view s)                                     \
    -> std::expected<std::vector<Node>, std::string_view> {                           \
    std::vector<Node> nodes;                                                          \
    /* {level, parent_id} */                                                          \
    std::vector<std::pair<int32_t, int32_t>> stack = {{0, 0}};                        \
    int32_t next_id                                = 1;                               \
    size_t i                                       = 0;                               \
                                                                                      \
    const auto parse_node_name = [&]()                                                \
      -> std::expected<std::string_view, std::string_view> {                          \
      size_t start = i;                                                               \
      while (i < s.size()) {                                                          \
        char c = s[i];                                                                \
        if (c == '(' || c == '[' || c == ']' || c == ',') break;                      \
        ++i;                                                                          \
      }                                                                               \
      if (i == start) return std::unexpected("empty node name");                      \
      return s.substr(start, i - start);                                              \
    };                                                                                \
                                                                                      \
    const auto parse_params = [&]() -> std::vector<Parameter> {                       \
      if (i >= s.size() || s[i] != '(') return {};                                    \
      ++i;  /* skip '(' */                                                            \
      int depth    = 1;                                                               \
      size_t start = i;                                                               \
      while (i < s.size() && depth > 0) {                                             \
        if (s[i] == '(')                                                              \
          ++depth;                                                                    \
        else if (s[i] == ')')                                                         \
          --depth;                                                                    \
        ++i;                                                                          \
      }                                                                               \
      return extract_params(s.substr(start, i - start - 1));                          \
    };                                                                                \
                                                                                      \
    const auto get_idx_for_type = [&](const std::string_view& _ss)                    \
      -> std::optional<size_t> {                                                      \
      const auto t = _ss.substr(0, _ss.find_first_of('('));                           \
      for (size_t i = 0; i < variant.size(); ++i)                                     \
        if (variant[i].second == t) return variant[i].first;                          \
      return std::nullopt;                                                            \
    };                                                                                \
                                                                                      \
    while (i < s.size()) {                                                            \
                                                                                      \
      /* Parse node name */                                                           \
      auto name_res = parse_node_name();                                              \
      if (!name_res) return std::unexpected(name_res.error());                        \
      std::string_view name = name_res.value();                                       \
                                                                                      \
      Node n;                                                                         \
      n.cl_      = name;                                                              \
      n.node_id_ = next_id++;                                                         \
      n.level_   = stack.back().first;                                                \
      n.parent_  = stack.back().second;                                               \
                                                                                      \
      auto idx   = get_idx_for_type(name);                                            \
      if (!idx) return std::unexpected(name);                                         \
      n.type_idx_ = idx.value();                                                      \
                                                                                      \
      /* Parse optional parameters */                                                 \
      if (i < s.size() && s[i] == '(') { n.p_ = parse_params(); }                     \
                                                                                      \
      nodes.push_back(std::move(n));                                                  \
                                                                                      \
      if (i >= s.size()) break;                                                       \
                                                                                      \
      char c = s[i];                                                                  \
                                                                                      \
      if (c == '[') {                                                                 \
        stack.push_back({stack.back().first + 1, n.node_id_});                        \
        ++i;                                                                          \
      } else if (c == ']') {                                                          \
        stack.pop_back();                                                             \
        ++i;                                                                          \
        /* Handle possible consecutive ]]... by continuing */                         \
        while (i < s.size() && s[i] == ']') {                                         \
          stack.pop_back();                                                           \
          ++i;                                                                        \
        }                                                                             \
        /* After closing brackets, expect either ',' or end or another node */        \
        if (i < s.size() && s[i] == ',') ++i;                                         \
      } else if (c == ',') {                                                          \
        ++i;                                                                          \
      } else {                                                                        \
        return std::unexpected(s.substr(i, 10));  /* invalid char */                  \
      }                                                                               \
    }                                                                                 \
                                                                                      \
    return nodes;                                                                     \
  };
  // clang-format on

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

    size_t out               = sizeof(Header) + root_children.size() * sizeof(uint32_t);

    for (const Node& n : nodes) {
      const auto children = gather_children(n.node_id_, nodes);
      out += (uint32_t)sizeof(NodeHeader) + (uint16_t)children.size() * sizeof(int32_t) +
             (uint16_t)n.p_.size() * (1 + sizeof(int32_t)) + sizeof(Composite);
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
    header.node_count_        = nodes.size();
    header.ptr_               = 0;
    header.node_count_        = (uint16_t)nodes.size();
    header.children_count_    = (uint16_t)root_children.size();
    header.first_node_offset_ = sizeof(Header) + header.children_count_ * sizeof(uint32_t);

    // write global header
    auto ha                   = std::bit_cast<std::array<uint8_t, sizeof(Header)>>(header);
    for (size_t i = 0; i < sizeof(Header); ++i) _vals[i] = ha[i];

    uint32_t ptr = header.first_node_offset_;
    for (Node& n : nodes) {
      NodeHeader header;
      header.type_idx_        = n.type_idx_;
      header.parent_          = n.parent_;

      const auto children     = gather_children(n.node_id_, nodes);
      header.children_count_  = (uint16_t)children.size();
      header.children_offset_ = ptr + (uint32_t)sizeof(NodeHeader);

      header.params_count_    = (uint16_t)n.p_.size();
      header.params_offset_   = header.children_offset_ + header.children_count_ * sizeof(int32_t);

      header.comp_offset_     = header.params_offset_ + header.params_count_ * (1 + sizeof(int32_t));

      header.node_size_       = header.comp_offset_ + sizeof(Composite) - ptr;

      n.offset_               = ptr;
      n.size_                 = header.node_size_;

      // write header
      const auto nha          = std::bit_cast<std::array<uint8_t, sizeof(NodeHeader)>>(header);
      for (size_t i = 0; i < sizeof(NodeHeader); ++i) _vals[ptr + i] = nha[i];

      // write params (5 bytes per param)
      for (size_t i = 0; i < n.p_.size(); ++i) {
        const auto& p       = n.p_[i];
        const size_t offset = header.params_offset_ + i * (1 + sizeof(int32_t));
        std::visit(overloaded(
                       [&](const bool _p) {
                         const auto pa     = std::bit_cast<std::array<uint8_t, sizeof(int32_t)>>((int32_t)_p);
                         _vals[offset]     = pt_bool;
                         _vals[offset + 1] = pa[0];
                         for (int32_t i = 1; i < sizeof(int32_t); ++i) _vals[offset + 1 + i] = 0x0;
                       },
                       [&](const int32_t _p) {
                         const auto pa = std::bit_cast<std::array<uint8_t, sizeof(int32_t)>>(_p);
                         _vals[offset] = pt_int;
                         for (int32_t i = 0; i < sizeof(int32_t); ++i) _vals[offset + 1 + i] = pa[i];
                       },
                       [&](const float _p) {
                         const auto pa = std::bit_cast<std::array<uint8_t, sizeof(float)>>(_p);
                         _vals[offset] = pt_float;
                         for (int32_t i = 0; i < sizeof(float); ++i) _vals[offset + 1 + i] = pa[i];
                       },
                       [&](const uint32_t _p) {
                         const auto pa = std::bit_cast<std::array<uint8_t, sizeof(uint32_t)>>(_p);
                         _vals[offset] = pt_dyn;
                         for (int32_t i = 0; i < sizeof(uint32_t); ++i) _vals[offset + 1 + i] = pa[i];
                       }),
                   p);
      }

      Composite cmp;
      cmp.cur_idx_ = 0;
      cmp.ptr_     = 0;

      auto cmpa    = std::bit_cast<std::array<uint8_t, sizeof(Composite)>>(cmp);
      for (size_t i = 0; i < sizeof(Composite); ++i) _vals[header.comp_offset_ + i] = cmpa[i];

      ptr += header.node_size_;
    }

    // link root children
    int32_t cc = 0;
    for (const int32_t id : root_children) {
      const uint32_t offset = sizeof(Header) + cc * sizeof(uint32_t);

      for (const Node& n : nodes) {
        if (id == n.node_id_) {
          auto cmpa = std::bit_cast<std::array<uint8_t, sizeof(int32_t)>>(n.offset_);
          for (size_t i = 0; i < sizeof(int32_t); ++i) _vals[offset + i] = cmpa[i];
          break;
        }
      }

      cc++;
    }

    // link children and parent
    for (const Node& n : nodes) {
      // parent
      if (n.parent_ != 0) {
        std::array<uint8_t, sizeof(NodeHeader)> tmp;
        for (int32_t j = 0; j < sizeof(NodeHeader); ++j) tmp[j] = _vals[n.offset_ + j];
        NodeHeader tar = std::bit_cast<NodeHeader>(tmp);

        for (const auto& nc : nodes) {
          if (nc.node_id_ == n.parent_) {
            tar.parent_   = nc.offset_;

            const auto ca = std::bit_cast<std::array<uint8_t, sizeof(NodeHeader)>>(tar);
            for (int32_t j = 0; j < sizeof(NodeHeader); ++j) _vals[n.offset_ + j] = ca[j];
            break;
          }
        }

        const auto nha = std::bit_cast<std::array<uint8_t, sizeof(NodeHeader)>>(tar);
        for (size_t i = 0; i < sizeof(NodeHeader); ++i) _vals[n.offset_ + i] = nha[i];
      }

      const uint32_t c_ptr = n.offset_ + (uint32_t)sizeof(NodeHeader);

      // children
      const auto children  = gather_children(n.node_id_, nodes);
      for (size_t i = 0; i < children.size(); ++i) {
        // find node with id
        for (const auto& nc : nodes) {
          if (nc.node_id_ == children[i]) {
            // write children (4 bytes per child)
            const auto ca = std::bit_cast<std::array<char, sizeof(int32_t)>>(nc.offset_);
            for (int32_t j = 0; j < sizeof(int32_t); ++j) _vals[c_ptr + i * sizeof(int32_t) + j] = ca[j];
            break;
          }
        }
      }
    }
  };

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