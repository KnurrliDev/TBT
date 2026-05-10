#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/ctre/ctre.hpp>

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

  struct Node {
    std::string_view name_;
    std::string_view options_;
    std::string_view parameters_;
    std::vector<Node> children_;
  };  // Node

  constexpr auto extract = [](std::string_view& input) -> std::optional<Node> {
    const auto extract_impl = [](std::string_view& input, auto& ref) -> std::optional<Node> {
      Node node;

      // 1. node_name (alphanumeric)
      constexpr auto name_pat = ctll::fixed_string{R"(^[a-zA-Z0-9]+)"};
      if (auto m = ctre::starts_with<name_pat>(input)) {
        node.name_ = m.get<0>().to_view();  // whole match
        input.remove_prefix(m.get<0>().size());
      } else {
        return std::nullopt;
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
        return std::nullopt;  // mandatory part missing
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

  constexpr inline size_t count_nodes(std::string_view _tree) {
    size_t out = 0;
    while (!_tree.empty()) {
      if (auto node = extract(_tree)) {
        out++;
      } else  // Malformed token: advance by one character to prevent infinite loop
        if (!_tree.empty()) { _tree.remove_prefix(1); }
    }
    return out;
  }  // count_nodes

}  // namespace TBT