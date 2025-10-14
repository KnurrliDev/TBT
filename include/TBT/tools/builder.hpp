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
*/

namespace TBT {

  using Parameter = std::variant<bool, int32_t, float, uint32_t>;

  namespace Util {

    template <size_t N>
    [[nodiscard]] consteval size_t c_count_elements(const sString<N>& _s, const sStringView& _p,
                                                    const char _delim) noexcept {
      size_t count = 1;
      for (size_t i = _p.first; i < _p.second; ++i) {
        if (_s.s_[i] == _delim) ++count;
      }
      return count;
    }  // count_elements

    template <size_t Elements, size_t N>
    [[nodiscard]] consteval std::array<std::optional<sStringView>, Elements> c_split(const sString<N>& _s,
                                                                                     const sStringView& _p,
                                                                                     const char _delim) noexcept {
      std::array<std::optional<sStringView>, Elements> result;

      const std::string_view sv(_s.s_.data(), _s.size_);
      size_t start = _p.first;
      size_t end   = sv.find(_delim);
      size_t idx   = 0;

      while (end != std::string::npos && end < _p.second) {
        if (start == end)
          result[idx++] = std::nullopt;
        else
          result[idx++] = {start, end};
        start = end + 1;
        end   = sv.find(_delim, start);
      }

      // Add the last segment
      if (start == _p.second - 1)
        result[idx++] = std::nullopt;
      else
        result[idx++] = {start, _p.second - 1};  // string_t(&_s[start], _s.size() - start);
      return result;

    }  // csplit

    template <size_t N>
    [[nodiscard]] consteval std::optional<sStringView> c_clause(const sString<N>& _s, const sStringView& _p,
                                                                const char _open_delim, const char _close_delim) {
      const char d1 = _open_delim;
      const char d2 = _close_delim;

      int32_t c     = 0;

      // find open
      size_t i_o    = _p.first;
      for (; i_o < _s.size_; ++i_o) {
        if (_s.s_[i_o] == _open_delim) {
          c++;
          i_o++;
          break;
        }
      }

      if (i_o >= _p.second + 1) return std::nullopt;

      // find close
      size_t i_e = i_o;
      for (; i_e < _p.second; ++i_e) {
        // new nested clause opens
        if (_s.s_[i_e] == _open_delim) {
          c++;
          continue;
        }

        // either nested clause closes or we found the end
        if (_s.s_[i_e] == _close_delim) {
          if (c == 1) {
            // end points to end of clause + 1
            break;
          } else
            c--;
        }
      }

      return {{i_o, i_e}};

    }  // c_clause

    template <size_t Elements, size_t N>
    [[nodiscard]] consteval auto c_extract_parameters(const sString<N>& _s, const sStringView& _p) {
      std::array<Parameter, Elements> out;
      size_t idx       = 0;

      const auto parts = c_split<Elements>(_s, _p, ',');

      for (const auto& rr : parts) {
        if (!rr) continue;
        const std::string_view r(&_s.s_[rr->first], rr->second - rr->first);
        if (r[0] == '$') {
          const std::optional<uint32_t> val = stn::StrToUInt32(std::string_view(&r[1], r.size() - 1));
          if (val) { out[idx++] = val.value(); }
          continue;
        } else if (r == "true") {
          out[idx++] = true;
          continue;
        } else if (r == "false") {
          out[idx++] = false;
          continue;
        }
        if (r.contains('.') || r.contains('f')) {
          const std::optional<float> val = stn::StrToFloat(r);
          if (val) { out[idx++] = val.value(); }
        } else {
          const std::optional<int32_t> val = stn::StrToInt32(r);
          if (val) { out[idx++] = val.value(); }
        }
      }
      return out;
    }  // extract_parameters

  }  // namespace Util

  namespace H {

    // Task($n, ...)[Task($n, ...), ...], ...
    // Task,
    // Task[
    // Task(...),
    // Task(...)[

    struct Node {
      int32_t idx_ = 0;
    };  // Node

    template <size_t Elements, size_t Params>
    struct NodeHierarchy {
      std::array<Node, Elements> n_;
      std::array<std::pair<int32_t, int32_t>, Elements> c_;
      std::array<std::pair<size_t, std::optional<Parameter>>, Params> p_;
      size_t c_size_ = 0;

      constexpr NodeHierarchy() {
        for (size_t i = 0; i < Elements; ++i) { n_[i] = Node(); }
        for (size_t i = 0; i < Elements; ++i) { c_[i] = {0, 0}; }
        for (size_t i = 0; i < Params; ++i) { p_[i] = {0, std::nullopt}; }
      }
    };  // NodeHierarchy

    template <size_t N>
    [[nodiscard]] consteval size_t c_count(const sString<N>& _s, const sStringView& _p) {
      if (_s.size_ == 0) return 0;

      size_t c      = 1;

      bool in_param = false;
      for (size_t i = _p.first; i < _p.second; ++i) {
        switch (_s.s_[i]) {
          case ',':
            if (!in_param) c++;
            break;
          case '[':
            c++;
            break;
          case ']':
            if (i + 1 < _s.size_ && _s.s_[i + 1] != ',') c++;
            break;
          case '(':
            in_param = true;
            break;
          case ')':
            in_param = false;
            break;
        }
      }

      return c;
    }  // c_count

    template <size_t N>
    [[nodiscard]] consteval size_t c_count_params(const sString<N>& _s, const sStringView& _p) {
      size_t c      = 0;

      bool in_param = false;
      for (size_t i = _p.first; i < _p.second; ++i) {
        switch (_s.s_[i]) {
          case ',':
            if (in_param) c++;
            break;
          case '(':
            in_param = true;
            if (i + 1 < _s.size_ && _s.s_[i + 1] != ')') c++;
            break;
          case ')':
            in_param = false;
            break;
        }
      }

      return c;
    }  // c_count_params

    /*

    */

    template <size_t Elements, size_t Params, size_t N>
    [[nodiscard]] consteval NodeHierarchy<Elements, Params> c_extract(const sString<N>& _s, const sStringView& _p) {
      NodeHierarchy<Elements, Params> out;

      std::vector<int32_t> dd;

      int32_t temp_task_id = 1;

      int32_t level        = 0;
      bool in_param        = false;

      size_t cur_s         = 0;
      size_t cur_e         = 1;

      size_t cur_p         = 0;

      for (size_t t = 0; t < Elements; ++t) {
        // find end
        for (; cur_e < _s.size_; ++cur_e) {
          switch (_s.s_[cur_e]) {
            case '[':
              level++;
              break;
            case ']':
              level--;
              break;
            case ',':
              break;
            case '(':
              in_param = true;
              break;
            case ')':
              in_param = false;
              break;
          }
        }

        const auto clause = Util::c_clause(_s, {cur_s, cur_e}, '(', ')');
        // has params
        if (clause) {
          const size_t s    = Util::c_count_elements(_s, clause.value(), ',');
          const auto result = Util::c_extract_parameters<s>(_s, clause.value());
          // for (const auto& p : result) out.p_[cur_p++] = p;
        }
      }

      return out;
    }  // c_extract
  }  // namespace H

  namespace SM {}

  namespace BT {

    // [[nodiscard]] inline std::shared_ptr<Node> extract_node(size_t& _ptr, const std::string& _s,
    //                                                         std::shared_ptr<Node> _parent,
    //                                                         const tsl::robin_map<std::string, int16_t>& _type_to_idx)
    //                                                         {
    //   std::shared_ptr<Node> out = std::make_shared<Node>();
    //   out->parent_              = _parent;

    //   const size_t last         = _ptr;
    //   bool extracted            = false;
    //   for (; _ptr < _s.size(); ++_ptr) {
    //     const char t = _s[_ptr];

    //     if (t == '[' || t == ']' || t == ',') break;

    //     if (t == '<') {
    //       extracted  = true;
    //       out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr]));
    //       size_t idx = _ptr + 1;
    //       out->d_    = extract_decorators(idx, _s, out, _type_to_idx);
    //       _ptr       = idx;
    //     }

    //     if (t == '(') {
    //       if (!extracted) {
    //         extracted  = true;
    //         out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr]));
    //       }
    //       size_t idx     = _ptr + 1;
    //       out->payloads_ = extract_parameters(idx, std::string_view(_s));
    //       _ptr           = idx;
    //     }
    //   }

    //   // neither decorator, nor payload
    //   if (!extracted) { out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr])); }

    //   return out;
    // }  // extract_node

    // void extract(size_t& _ptr, const std::string& _s, std::stack<std::shared_ptr<Node>>& _stack,
    //              const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
    //   // single task
    //   if (!_s.contains('[')) {
    //     // TODO error checking?
    //     size_t idx = 0;
    //     auto node  = extract_node(idx, _s, _stack.top(), _type_to_idx);
    //     _stack.top()->c_.push_back(std::move(node));
    //     return;
    //   }

    //   size_t idx                 = _ptr;
    //   auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
    //   std::shared_ptr<Node> next = node;
    //   _stack.top()->c_.push_back(std::move(node));
    //   _stack.push(next);
    //   _ptr = idx - 1;

    //   // tree
    //   for (; _ptr < _s.size(); ++_ptr) {
    //     const char t = _s[_ptr];

    //     if (t == '[') {
    //       size_t idx                 = _ptr + 1;
    //       auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
    //       std::shared_ptr<Node> next = node;
    //       node->parent_              = _stack.top();
    //       _stack.top()->c_.push_back(std::move(node));
    //       _stack.push(next);
    //       _ptr = idx - 1;
    //       continue;
    //     }

    //     if (t == ',') {
    //       _stack.pop();
    //       if (_stack.empty()) break;
    //       size_t idx                 = _ptr + 1;
    //       auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
    //       std::shared_ptr<Node> next = node;
    //       _stack.top()->c_.push_back(std::move(node));
    //       _stack.push(next);
    //       _ptr = idx - 1;
    //       continue;
    //     }

    //     if (t == ']') {
    //       _stack.pop();
    //       continue;
    //     }
    //   }

    // }  // extract

  }  // namespace BT

  // [[nodiscard]] inline std::shared_ptr<Detail::Node> build(const std::string& _tree,
  //                                                          const tsl::robin_map<std::string, int16_t>& _type_to_idx)
  //                                                          {
  //   // clean string from all white space characters
  //   const std::string tree = [](std::string _input) {
  //     _input.erase(std::remove_if(_input.begin(), _input.end(),
  //                                 [](char c) { return std::isspace(static_cast<unsigned char>(c)); }),
  //                  _input.end());
  //     return _input;
  //   }(_tree);

  //   //----------------------------------------------------
  //   // extract tree structure
  //   std::shared_ptr<Detail::Node> root = std::make_shared<Detail::Node>();
  //   root->type_                        = _type_to_idx.at("root");
  //   std::stack<std::shared_ptr<Detail::Node>> stack;
  //   stack.push(root);

  //   size_t ptr = 0;
  //   Detail::extract(ptr, tree, stack, _type_to_idx);

  //   // link decorators
  //   link_decorator(root);

  //   return root;
  // }  // compile

  // namespace Detail {
  //   template <typename Variant>
  //   struct variant_types;

  //   template <typename... Ts>
  //   struct variant_types<std::variant<Ts...>> {
  //     using type = std::tuple<Ts...>;
  //   };
  // }  // namespace Detail

  // template <typename Variant>
  // [[nodiscard]] tsl::robin_map<std::string, int16_t> create_mapping() {
  //   return []<typename... Ts>(std::tuple<Ts...>) {
  //     tsl::robin_map<std::string, int16_t> out;
  //     out["root"]           = Execute::vidx_root;
  //     out["sequence"]       = Execute::vidx_sequence;
  //     out["fallback"]       = Execute::vidx_fallback;
  //     out["nloop"]          = Execute::vidx_nloop;
  //     out["cooldown"]       = Execute::vidx_cooldown;
  //     out["always_fail"]    = Execute::vidx_always_fail;
  //     out["always_succeed"] = Execute::vidx_always_succeed;

  //     int16_t i             = 0;
  //     ((out[Detail::TypeName<Ts>::Get()] = i++), ...);
  //     return out;
  //   }(typename Detail::variant_types<Variant>::type{});
  // }  // create_mapping

}  // namespace TBT