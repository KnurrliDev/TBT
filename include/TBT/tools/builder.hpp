#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/execute/states.hpp>
#include <TBT/strtonum/StrToNum.hpp>

/*
  Grammar:
    Hierarchy: Task($n, ...)[Task($n, ...), ...]
      special node: root if no custom root node
                    root[Task($n, ...)[Task($n, ...), ...], Task($n, ...)]

    Behaviour Tree: sequence[Task($n, ...), Task($n, ...)]

    State Machine: $[$n: Task($n, ...), ..., $n0->$n1:Task($n, ...), ...]
      $n:     State
      $n->$n: transition
*/

namespace TBT {

  template <class Iterator>
  using Part      = std::pair<Iterator, Iterator>;

  using Parameter = std::variant<bool, int32_t, float, uint32_t>;

  namespace Util {

    template <class String>
    [[nodiscard]] consteval size_t count_elements(String&& _s, const char _delim) noexcept {
      size_t count = 1;
      for (size_t i = 0; i < _s.size(); ++i) {
        if (_s[i] == _delim) ++count;
      }
      return count;
    }

    template <size_t Elements, class String>
    [[nodiscard]] consteval auto csplit(String&& _s, const char _delim) noexcept {
      using char_t   = std::decay_t<String>::value_type;
      using string_t = std::basic_string_view<char_t>;

      std::array<string_t, Elements> result;
      size_t start = 0;
      size_t end   = _s.find(_delim);
      size_t idx   = 0;

      while (end != std::string::npos) {
        if (start == end)
          result[idx++] = string_t();
        else
          result[idx++] = string_t(&_s[start], end - start);
        start = end + 1;
        end   = _s.find(_delim, start);
      }

      // Add the last segment
      if (start == _s.size())
        result[idx++] = string_t();
      else
        result[idx++] = string_t(&_s[start], _s.size() - start);
      return result;

    }  // csplit

    template <class Iterator>
    [[nodiscard]] constexpr std::pair<Iterator, Iterator> clause(Iterator _begin, Iterator _end, const char _open_delim,
                                                                 const char _close_delim) {
      constexpr char d1 = _open_delim;
      constexpr char d2 = _close_delim;

      int32_t c         = 0;

      // find open
      Iterator it_o     = _begin;
      for (; it_o != _end; ++it_o) {
        if (*it_o == _open_delim) {
          c++;
          break;
        }
      }

      // find close
      Iterator it_e = it_o + 1;
      for (; it_e != _end; ++it_e) {
        // new nested clause opens
        if (*it_e == _open_delim) {
          c++;
          continue;
        }

        // either nested clause closes or we found the end
        if (*it_e == _close_delim && c == 0) {
          // end points to end of clause + 1
          it_e++;
          break;
        } else
          c--;
      }

      return {it_o, it_e};

    }  // clause

  }  // namespace Util

  namespace Common {

    // template <class Iterator>
    // [[nodiscard]] constexpr inline std::vector<Parameter> extract_parameters(Iterator _begin, Iterator _end) {
    //   std::vector<std::variant<bool, int32_t, float, uint32_t>> out;

    //   size_t last = _ptr;
    //   for (; _ptr < _s.size(); ++_ptr) {
    //     const char t = _s[_ptr];
    //     if (t == ')') break;
    //   }

    //   const std::string v(&_s[last], &_s[_ptr]);
    //   const auto res = split(v, ',');

    //   for (const auto& r : res) {
    //     if (r[0] == '$') {
    //       const int32_t val = std::stoi(std::string(r.substr(1, r.size() - 1)));
    //       out.push_back(uint32_t(val));
    //       continue;
    //     } else if (r == "true") {
    //       out.push_back(true);
    //       continue;
    //     } else if (r == "false") {
    //       out.push_back(false);
    //       continue;
    //     }
    //     if (r.contains('.') || r.contains('f')) {
    //       try {
    //         const float val = std::stof(std::string(r));
    //         out.push_back(val);
    //         continue;
    //       } catch (std::exception&) {}
    //     } else {
    //       try {
    //         const int32_t val = std::stoi(std::string(r));
    //         out.push_back(val);
    //         continue;
    //       } catch (std::exception&) {}
    //     }
    //   }

    //   return out;
    // }  // extract_parameters

  }  // namespace Common

  namespace H {}

  namespace SM {}

  namespace BT {

    // [[nodiscard]] inline std::vector<std::shared_ptr<Node>> extract_decorators(
    //     size_t& _ptr, const std::string& _s, std::shared_ptr<Node> _parent,
    //     const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
    //   std::vector<std::shared_ptr<Node>> out;
    //   size_t last = _ptr;
    //   for (; _ptr < _s.size(); ++_ptr) {
    //     const char t = _s[_ptr];
    //     if (t == '>') break;
    //   }

    //   const std::string v(&_s[last], &_s[_ptr]);
    //   const auto res = split(v, ',');
    //   for (const auto& r : res) {
    //     size_t brack = r.find_first_of('(', 0);

    //     if (brack != r.npos) {
    //       std::shared_ptr<Node> d = std::make_shared<Node>();
    //       d->parent_              = _parent;
    //       d->type_                = _type_to_idx.at(std::string(&r[0], &r[brack]));
    //       brack++;
    //       d->payloads_ = extract_parameters(brack, r);
    //       out.push_back(std::move(d));
    //     }

    //     // no payload
    //     else {
    //       std::shared_ptr<Node> d = std::make_shared<Node>();
    //       d->parent_              = _parent;
    //       d->type_                = _type_to_idx.at(std::string(r));
    //       out.push_back(std::move(d));
    //     }
    //   }

    //   return out;
    // }  // extract_decorators

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