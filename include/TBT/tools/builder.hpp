#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/execute/states.hpp>

namespace TBT {

  namespace Detail {

    [[nodiscard]] inline std::vector<std::string_view> split(const std::string& _s, const char _delim) {
      std::vector<std::string_view> result;
      size_t start = 0;
      size_t end   = _s.find(_delim);

      while (end != std::string::npos) {
        result.emplace_back(_s.data() + start, end - start);
        start = end + 1;
        end   = _s.find(_delim, start);
      }

      // Add the last segment
      result.emplace_back(_s.data() + start, _s.length() - start);
      return result;
    }  // split

    [[nodiscard]] inline std::vector<std::variant<bool, int32_t, float, uint32_t>> extract_parameters(
        size_t& _ptr, const std::string_view& _s) {
      std::vector<std::variant<bool, int32_t, float, uint32_t>> out;

      size_t last = _ptr;
      for (; _ptr < _s.size(); ++_ptr) {
        const char t = _s[_ptr];
        if (t == ')') break;
      }

      const std::string v(&_s[last], &_s[_ptr]);
      const auto res = split(v, ',');

      for (const auto& r : res) {
        if (r[0] == '$') {
          const int32_t val = std::stoi(std::string(r.substr(1, r.size() - 1)));
          out.push_back(uint32_t(val));
          continue;
        } else if (r == "true") {
          out.push_back(true);
          continue;
        } else if (r == "false") {
          out.push_back(false);
          continue;
        }
        if (r.contains('.') || r.contains('f')) {
          try {
            const float val = std::stof(std::string(r));
            out.push_back(val);
            continue;
          } catch (std::exception&) {}
        } else {
          try {
            const int32_t val = std::stoi(std::string(r));
            out.push_back(val);
            continue;
          } catch (std::exception&) {}
        }
      }

      return out;
    }  // extract_parameters

    [[nodiscard]] inline std::vector<std::shared_ptr<Node>> extract_decorators(
        size_t& _ptr, const std::string& _s, std::shared_ptr<Node> _parent,
        const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
      std::vector<std::shared_ptr<Node>> out;
      size_t last = _ptr;
      for (; _ptr < _s.size(); ++_ptr) {
        const char t = _s[_ptr];
        if (t == '>') break;
      }

      const std::string v(&_s[last], &_s[_ptr]);
      const auto res = split(v, ',');
      for (const auto& r : res) {
        size_t brack = r.find_first_of('(', 0);

        if (brack != r.npos) {
          std::shared_ptr<Node> d = std::make_shared<Node>();
          d->parent_              = _parent;
          d->type_                = _type_to_idx.at(std::string(&r[0], &r[brack]));
          brack++;
          d->payloads_ = extract_parameters(brack, r);
          out.push_back(std::move(d));
        }

        // no payload
        else {
          std::shared_ptr<Node> d = std::make_shared<Node>();
          d->parent_              = _parent;
          d->type_                = _type_to_idx.at(std::string(r));
          out.push_back(std::move(d));
        }
      }

      return out;
    }  // extract_decorators

    [[nodiscard]] inline std::shared_ptr<Node> extract_node(size_t& _ptr, const std::string& _s,
                                                            std::shared_ptr<Node> _parent,
                                                            const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
      std::shared_ptr<Node> out = std::make_shared<Node>();
      out->parent_              = _parent;

      const size_t last         = _ptr;
      bool extracted            = false;
      for (; _ptr < _s.size(); ++_ptr) {
        const char t = _s[_ptr];

        if (t == '[' || t == ']' || t == ',') break;

        if (t == '<') {
          extracted  = true;
          out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr]));
          size_t idx = _ptr + 1;
          out->d_    = extract_decorators(idx, _s, out, _type_to_idx);
          _ptr       = idx;
        }

        if (t == '(') {
          if (!extracted) {
            extracted  = true;
            out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr]));
          }
          size_t idx     = _ptr + 1;
          out->payloads_ = extract_parameters(idx, std::string_view(_s));
          _ptr           = idx;
        }
      }

      // neither decorator, nor payload
      if (!extracted) { out->type_ = _type_to_idx.at(std::string(&_s[last], &_s[_ptr])); }

      return out;
    }  // extract_node

    void extract(size_t& _ptr, const std::string& _s, std::stack<std::shared_ptr<Node>>& _stack,
                 const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
      // single task
      if (!_s.contains('[')) {
        // TODO error checking?
        size_t idx = 0;
        auto node  = extract_node(idx, _s, _stack.top(), _type_to_idx);
        _stack.top()->c_.push_back(std::move(node));
        return;
      }

      size_t idx                 = _ptr;
      auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
      std::shared_ptr<Node> next = node;
      _stack.top()->c_.push_back(std::move(node));
      _stack.push(next);
      _ptr = idx - 1;

      // tree
      for (; _ptr < _s.size(); ++_ptr) {
        const char t = _s[_ptr];

        if (t == '[') {
          size_t idx                 = _ptr + 1;
          auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
          std::shared_ptr<Node> next = node;
          node->parent_              = _stack.top();
          _stack.top()->c_.push_back(std::move(node));
          _stack.push(next);
          _ptr = idx - 1;
          continue;
        }

        if (t == ',') {
          _stack.pop();
          if (_stack.empty()) break;
          size_t idx                 = _ptr + 1;
          auto node                  = extract_node(idx, _s, _stack.top(), _type_to_idx);
          std::shared_ptr<Node> next = node;
          _stack.top()->c_.push_back(std::move(node));
          _stack.push(next);
          _ptr = idx - 1;
          continue;
        }

        if (t == ']') {
          _stack.pop();
          continue;
        }
      }

    }  // extract

  }  // namespace Detail

  inline void link_decorator(std::shared_ptr<Detail::Node>& _root) {
    std::stack<std::shared_ptr<Detail::Node>> stack;
    stack.push(_root);

    while (!stack.empty()) {
      std::shared_ptr<Detail::Node> node = stack.top();
      stack.pop();

      for (auto& c : node->c_) stack.push(c);

      if (node->d_.empty()) continue;

      // link decorators
      // parent -> nloop -> always succeed -> always fail -> cd -> node
      std::array<std::shared_ptr<Detail::Node>, 6> line = {node->parent_, nullptr, nullptr, nullptr, nullptr, node};

      // nloop
      for (auto& c : node->d_) {
        if (c->type_ == Execute::vidx_nloop) {
          line[1] = c;
          break;
        }
      }
      // always ...
      for (auto& c : node->d_) {
        if (c->type_ == Execute::vidx_always_fail) {
          line[2] = c;
          break;
        }
      }
      for (auto& c : node->d_) {
        if (c->type_ == Execute::vidx_always_succeed) {
          line[3] = c;
          break;
        }
      }
      // cd
      for (auto& c : node->d_) {
        if (c->type_ == Execute::vidx_cooldown) {
          line[4] = c;
          break;
        }
      }

      // chain the decorators
      for (int32_t i = 0; i < 5; ++i) {
        if (line[i] == nullptr) continue;
        auto n1 = line[i];
        // find next
        for (int32_t j = i + 1; j < 6; ++j) {
          //
          if (line[j] == nullptr) continue;
          auto n2 = line[j];
          // replace first decorator with child to keep order
          if (i == 0) {
            for (auto& p : line[0]->c_)
              if (p == line[5]) {
                p = n2;
                break;
              }
            n2->parent_ = n1;
          } else {
            n1->c_.push_back(n2);
            n2->parent_ = n1;
          }
          break;
        }
      }
    }
  }  // link_decorator

  [[nodiscard]] inline std::shared_ptr<Detail::Node> build(const std::string& _tree,
                                                           const tsl::robin_map<std::string, int16_t>& _type_to_idx) {
    // clean string from all white space characters
    const std::string tree = [](std::string _input) {
      _input.erase(std::remove_if(_input.begin(), _input.end(),
                                  [](char c) { return std::isspace(static_cast<unsigned char>(c)); }),
                   _input.end());
      return _input;
    }(_tree);

    //----------------------------------------------------
    // extract tree structure
    std::shared_ptr<Detail::Node> root = std::make_shared<Detail::Node>();
    root->type_                        = _type_to_idx.at("root");
    std::stack<std::shared_ptr<Detail::Node>> stack;
    stack.push(root);

    size_t ptr = 0;
    Detail::extract(ptr, tree, stack, _type_to_idx);

    // link decorators
    link_decorator(root);

    return root;
  }  // compile

  namespace Detail {
    template <typename Variant>
    struct variant_types;

    template <typename... Ts>
    struct variant_types<std::variant<Ts...>> {
      using type = std::tuple<Ts...>;
    };
  }  // namespace Detail

  template <typename Variant>
  [[nodiscard]] tsl::robin_map<std::string, int16_t> create_mapping() {
    return []<typename... Ts>(std::tuple<Ts...>) {
      tsl::robin_map<std::string, int16_t> out;
      out["root"]           = Execute::vidx_root;
      out["sequence"]       = Execute::vidx_sequence;
      out["fallback"]       = Execute::vidx_fallback;
      out["nloop"]          = Execute::vidx_nloop;
      out["cooldown"]       = Execute::vidx_cooldown;
      out["always_fail"]    = Execute::vidx_always_fail;
      out["always_succeed"] = Execute::vidx_always_succeed;

      int16_t i             = 0;
      ((out[Detail::TypeName<Ts>::Get()] = i++), ...);
      return out;
    }(typename Detail::variant_types<Variant>::type{});
  }  // create_mapping

}  // namespace TBT