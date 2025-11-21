#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/common/static_string.hpp>
#include <TBT/execute/states.hpp>

/*
  Grammer:
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

namespace TBT {
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

  #define F_CLAUSE                                                        \
    const auto clause = [](const std::string_view& _s,                    \
      const char _open_delim, const char _close_delim)                    \
      -> std::string_view {                                               \
    const char d1 = _open_delim;                                          \
    const char d2 = _close_delim;                                         \
                                                                          \
    if(!_s.contains(d1) && !_s.contains(d2)) return {};                   \
                                                                          \
    int32_t c     = 0;                                                    \
                                                                          \
    /* find open */                                                       \
    size_t i_o    = 0;                                                    \
    for (; i_o < _s.size(); ++i_o) {                                      \
      if (_s[i_o] == _open_delim) {                                       \
        c++;                                                              \
        i_o++;                                                            \
        break;                                                            \
      }                                                                   \
    }                                                                     \
                                                                          \
    if (i_o >= _s.size() + 1) return std::string_view();                  \
                                                                          \
    /* find close */                                                      \
    size_t i_e = i_o;                                                     \
    for (; i_e < _s.size(); ++i_e) {                                      \
      /* new nested clause opens */                                       \
      if (_s[i_e] == _open_delim) {                                       \
        c++;                                                              \
        continue;                                                         \
      }                                                                   \
                                                                          \
      /* either nested clause closes or we found the end */               \
      if (_s[i_e] == _close_delim) {                                      \
        if (c == 1) {                                                     \
          /* end points to end of clause + 1 */                           \
          break;                                                          \
        } else                                                            \
          c--;                                                            \
      }                                                                   \
    }                                                                     \
                                                                          \
    return {&_s[i_o], i_e - i_o};                                         \
  };

  #define F_PARAMS \
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

  #define H_COUNT_NODES                                                   \
    const auto h_count_nodes = [](const std::string_view& _s) {           \
      size_t c      = 0;                                                  \
      size_t last   = 0;                                                  \
                                                                          \
      bool in_param = false;                                              \
      for (size_t i = 0; i < _s.size(); ++i) {                            \
        switch (_s[i]) {                                                  \
          case ',':                                                       \
            if (!in_param) {                                              \
              last = i;                                                   \
              c++;                                                        \
            }                                                             \
            break;                                                        \
          case '[':                                                       \
            last = i;                                                     \
            c++;                                                          \
            break;                                                        \
          case ']':                                                       \
            if (i + 1 == _s.size() || (i + 1 < _s.size() &&               \
              _s[i + 1] != ',')) {                                        \
              last = i;                                                   \
              c++;                                                        \
            }                                                             \
            break;                                                        \
          case '(':                                                       \
            in_param = true;                                              \
            break;                                                        \
          case ')':                                                       \
            in_param = false;                                             \
            break;                                                        \
        }                                                                 \
      }                                                                   \
                                                                          \
      if (last != _s.size() - 1) { c++; }                                 \
      return c;                                                           \
    };

  #define H_COUNT_PARAMS                                                  \
    const auto h_count_params = [](const std::string_view& _s) {          \
    size_t c      = 0;                                                    \
                                                                          \
    bool in_param = false;                                                \
    for (size_t i = 0; i < _s.size(); ++i) {                              \
      switch (_s[i]) {                                                    \
        case ',':                                                         \
          if (in_param) c++;                                              \
          break;                                                          \
        case '(':                                                         \
          in_param = true;                                                \
          if (i + 1 < _s.size() && _s[i + 1] != ')') c++;                 \
          break;                                                          \
        case ')':                                                         \
          in_param = false;                                               \
          break;                                                          \
      }                                                                   \
    }                                                                     \
                                                                          \
    return c;                                                             \
  };

  #define H_EXTRACT                                                                               \
  const auto h_extract = [&](const std::string_view& _s) ->                                       \
    std::expected<std::vector<Node>, std::string_view> {                                          \
    std::vector<int32_t> dd;                                                                      \
                                                                                                  \
    int32_t temp_task_id = 1;                                                                     \
                                                                                                  \
    int32_t level        = 0;                                                                     \
    bool in_param        = false;                                                                 \
                                                                                                  \
    std::vector<std::pair<int32_t, int32_t>> stack;                                               \
    stack.push_back({0, 0});                                                                      \
                                                                                                  \
    const auto get_idx_for_type = [&](const std::string_view& _ss) -> std::optional<size_t> {     \
      const auto t = _ss.substr(0, _ss.find_first_of('('));                                       \
      for (size_t i = 0; i < variant.size(); ++i)                                                 \
        if (variant[i].second == t) return variant[i].first;                                      \
      return std::nullopt;                                                                        \
    };                                                                                            \
                                                                                                  \
    /* valid ends: 'Task,', 'Task[' 'Task]', 'Task],' */                                          \
    std::vector<Node> nodes;                                                                      \
    int32_t idd = 1;                                                                              \
    for (size_t cur_s = 0; cur_s < _s.size(); ++cur_s) {                                          \
      /* find end of current node */                                                              \
      for (size_t cur_e = cur_s + 1; cur_e < _s.size(); ++cur_e) {                                \
        bool next = false;                                                                        \
        if (cur_e == _s.size() - 1 && _s[cur_e] != ']') {                                         \
          Node n;                                                                                 \
          n.cl_          = std::string_view{&_s[cur_s], cur_e - cur_s + 1};                       \
          n.node_id_     = idd++;                                                                 \
          const auto idx = get_idx_for_type(n.cl_);                                               \
          if (!idx) return std::unexpected(n.cl_);                                                \
          n.type_idx_   = idx.value();                                                            \
          n.level_      = stack.back().first;                                                     \
          n.parent_     = stack.back().second;                                                    \
          const auto cl = clause(n.cl_, '(', ')');                                                \
          if (!cl.empty()) n.p_ = extract_params(cl);                                             \
          nodes.push_back(std::move(n));                                                          \
          cur_s = _s.size();                                                                      \
          next  = true;                                                                           \
        }                                                                                         \
        if (!next) {                                                                              \
          switch (_s[cur_e]) {                                                                    \
            case '[': {                                                                           \
              {                                                                                   \
                Node n;                                                                           \
                n.cl_          = std::string_view{&_s[cur_s], cur_e - cur_s};                     \
                n.node_id_     = idd++;                                                           \
                const auto idx = get_idx_for_type(n.cl_);                                         \
                if (!idx) return std::unexpected(n.cl_);                                          \
                n.type_idx_   = idx.value();                                                      \
                n.level_      = stack.back().first;                                               \
                n.parent_     = stack.back().second;                                              \
                const auto cl = clause(n.cl_, '(', ')');                                          \
                if (!cl.empty()) n.p_ = extract_params(cl);                                       \
                nodes.push_back(std::move(n));                                                    \
              }                                                                                   \
              stack.push_back({stack.back().first + 1, idd - 1});                                 \
              next = true;                                                                        \
              break;                                                                              \
            }                                                                                     \
            case ']':                                                                             \
              if (cur_e - 1 >= 0 && _s[cur_e - 1] != ']') {                                       \
                Node n;                                                                           \
                n.cl_          = std::string_view{&_s[cur_s], cur_e - cur_s};                     \
                n.node_id_     = idd++;                                                           \
                const auto idx = get_idx_for_type(n.cl_);                                         \
                if (!idx) return std::unexpected(n.cl_);                                          \
                n.type_idx_   = idx.value();                                                      \
                n.level_      = stack.back().first;                                               \
                n.parent_     = stack.back().second;                                              \
                const auto cl = clause(n.cl_, '(', ')');                                          \
                if (!cl.empty()) n.p_ = extract_params(cl);                                       \
                nodes.push_back(std::move(n));                                                    \
              }                                                                                   \
              stack.pop_back();                                                                   \
              next = true;                                                                        \
              if (cur_e + 1 < _s.size() && _s[cur_e + 1] == ',') { ++cur_e; }                     \
              break;                                                                              \
            case ',': /* set level and parent */                                                  \
              if (!in_param) {                                                                    \
                Node n;                                                                           \
                n.cl_          = std::string_view{&_s[cur_s], cur_e - cur_s};                     \
                n.node_id_     = idd++;                                                           \
                const auto idx = get_idx_for_type(n.cl_);                                         \
                if (!idx) return std::unexpected(n.cl_);                                          \
                n.type_idx_   = idx.value();                                                      \
                n.level_      = stack.back().first;                                               \
                n.parent_     = stack.back().second;                                              \
                const auto cl = clause(n.cl_, '(', ')');                                          \
                if (!cl.empty()) n.p_ = extract_params(cl);                                       \
                nodes.push_back(std::move(n));                                                    \
                next = true;                                                                      \
              }                                                                                   \
              break;                                                                              \
            case '(':                                                                             \
              in_param = true;                                                                    \
              break;                                                                              \
            case ')':                                                                             \
              in_param = false;                                                                   \
              break;                                                                              \
          }                                                                                       \
        }                                                                                         \
        if (next) { /* last node is a bt */                                                       \
          cur_s = cur_e;                                                                          \
          break;                                                                                  \
        }                                                                                         \
      }                                                                                           \
    }                                                                                             \
                                                                                                  \
    return nodes;                                                                                 \
  };
  
  #define F_ALL F_SPLIT F_CLEAN F_CLAUSE F_PARAMS H_COUNT_NODES           \
    H_COUNT_PARAMS H_EXTRACT

  // clang-format on

  using Parameter = std::variant<bool, int32_t, float, uint32_t>;

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

};  // namespace TBT

//[[nodiscard]] consteval size_t estimate_size(const std::string_view& _in) noexcept { return 20; }  // estimate_size

template <size_t Size>
consteval auto build(const std::string_view& _in) {
  // const auto extract_h = [&](const std::string_view& _s) -> NodeHierarchy {
  //   NodeHierarchy out;

  //   return out;
  // };  // extract_h

  // const auto extract_bt = [&]() -> void {
  //
  //};

  // const auto extract_sm = []() -> void {
  //
  //};

  // sString<Size> out;

  // extract nodes

  // flatten nodes to string

  // return out;
  //  return [out]() { return out; }
  //}  // build

}  // namespace TBT