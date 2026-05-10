#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/ctre/ctre.hpp>
#include <format>
#include <queue>
/*
Node (recursive):
  - Leaf Task:          identifier[modifier]( param1, param2, ... )
  - Composite Node:     identifier[modifier]( param1, param2, ... )[ Node, Node, ... ]
  - State Machine:      FSM[ StateDefinition, ..., TransitionDefinition, ... ]

StateDefinition:        identifier : Node
TransitionDefinition:   identifier -> identifier : Node

identifier is any valid name (e.g., move, sequence, selector, parallel, MyCustomTask, fsm).

sequence[nloop(n=4)()[ Foo(bar=2), Foo[]()[] ]]
*/

namespace TBT {

  enum class ErrorType {
    //
  };

  struct Node {
    std::string_view name_;
    std::string_view options_;
    std::string_view parameters_;
    std::vector<Node> children_;
  };  // Node

  struct Error {
    // size_t idx_;
    // std::string_view snippet_;
    // ErrorType type_;
    // size_t line_;
  };  // Error

  consteval std::array<char, 512> print_error_msg([[maybe_unused]] const Error& _er) {
    std::array<char, 512> out = {'\0'};
    size_t i                  = 0;
    out[i++]                  = '\n';
    out[i++]                  = '\n';
    out[i++]                  = '\n';
    out[i++]                  = 'd';
    out[i++]                  = 'i';
    out[i++]                  = 'n';
    out[i++]                  = 'g';
    out[i++]                  = '\n';
    out[i++]                  = '\n';
    out[i++]                  = '\n';
    return out;
  }  // print_error_msg

  constexpr auto clean = [](const std::string_view& _in) -> std::string {
    std::string out;
    out.resize(_in.size());
    size_t j = 0;
    for (size_t i = 0; i < _in.size(); ++i) {
      switch (_in[i]) {
        case 0x20: /* space ' ' */
        case 0x0c: /* form feed '\f' */
        case 0x0a: /* line feed '\n' */
        case 0x0d: /* carriage return '\r' */
        case 0x09: /* horizontal tab '\t' */
        case 0x0b: /* vertical tab '\v' */
          continue;
        default:
          out[j++] = _in[i];
      }
    }
    out.shrink_to_fit();
    return out;
  };  // clean

  constexpr auto extract = [](std::string_view input) -> std::expected<Node, Error> {
    const auto extract_impl = [](std::string_view& input, auto& ref) -> std::expected<Node, Error> {
      Node node;

      // 1. node_name (alphanumeric)
      constexpr auto name_pat = ctll::fixed_string{R"(^[a-zA-Z0-9]+)"};
      if (auto m = ctre::starts_with<name_pat>(input)) {
        node.name_ = m.get<0>().to_view();  // whole match
        input.remove_prefix(m.get<0>().size());
      } else {
        return std::unexpected(Error());
      }

      // 2. optional [options]
      constexpr auto options_pat = ctll::fixed_string{R"(^\[([^\]]+)\])"};
      if (auto m = ctre::starts_with<options_pat>(input)) {
        node.options_ = m.get<1>().to_view();
        input.remove_prefix(m.get<0>().size());
      }

      // 3. required (parameters...)
      constexpr auto params_pat = ctll::fixed_string{R"(^\(([^)]*)\))"};
      if (auto m = ctre::starts_with<params_pat>(input)) {
        node.parameters_ = m.get<1>().to_view();
        input.remove_prefix(m.get<0>().size());
      } else {
        return std::unexpected(Error());  // mandatory part missing
      }

      // 4. optional [child_node, child_node, ...]
      constexpr auto open_bracket_pat = ctll::fixed_string{R"(^\[)"};
      if (auto m = ctre::starts_with<open_bracket_pat>(input)) {
        input.remove_prefix(m.get<0>().size());  // consume '['

        bool expect_comma = false;
        while (!input.empty()) {
          // closing ']' ?
          constexpr auto close_bracket_pat = ctll::fixed_string{R"(^\])"};
          if (auto close_m = ctre::starts_with<close_bracket_pat>(input)) {
            input.remove_prefix(close_m.get<0>().size());
            break;
          }

          // optional comma separator
          if (expect_comma) {
            constexpr auto comma_pat = ctll::fixed_string{R"(^,)"};
            if (auto comma_m = ctre::starts_with<comma_pat>(input)) {
              input.remove_prefix(comma_m.get<0>().size());
            } else {
              break;  // malformed – abort gracefully
            }
          }

          // recursive child node
          if (auto child = ref(input, ref)) {
            node.children_.push_back(std::move(*child));
            expect_comma = true;
          } else {
            break;
          }
        }
      }

      return node;
    };
    return extract_impl(input, extract_impl);
  };  // extract

  constexpr auto count_children_nodes = [](const Node& _node) -> size_t {
    size_t out = 0;
    std::vector<const Node*> q;
    q.push_back(&_node);
    while (!q.empty()) {
      out++;
      const Node* nn = q.back();
      q.pop_back();
      for (const auto& c : nn->children_) { q.push_back(&c); }
    }
    return out;
  };

  template <size_t NodeCount>
  consteval std::array<Node, NodeCount> parse(std::string_view _tree) {
    std::array<Node, NodeCount> nodes;

    // while (!_tree.empty()) {
    //   if (auto node = extract(_tree)) {
    //     nodes.push_back(std::move(*node));
    //   } else {
    //     // Malformed token: advance by one character to prevent infinite loop
    //     if (!_tree.empty()) { _tree.remove_prefix(1); }
    //   }
    // }

    return nodes;
  }  // parse

  constexpr inline std::expected<size_t, Error> count_nodes(const std::string_view& _tree) {
    const std::string ctree = clean(_tree);

    size_t out              = 0;

    for (size_t idx = 0; idx < ctree.size(); ++idx) {
      const char next = ctree[idx];

      if (next == ',') { continue; }

      const auto node = extract(std::string_view(&ctree[idx], ctree.size()));

      if (node) {
        out += count_children_nodes(node.value());
      } else {
        return std::unexpected(std::move(node.error()));
      }
    }
    return out;
  }  // count_nodes

}  // namespace TBT