#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/ctre/ctre.hpp>
#include <format>
#include <iostream>
#include <print>
#include <queue>

/*
Node (recursive):
- Leaf Task:          identifier[modifier]( param1, param2, ... )
- Composite Node:     identifier[modifier]( param1, param2, ... )[ Node, Node, ... ]
- State Machine:      FSM[modifier](StateDefinition, ..., TransitionDefinition, ... )

StateDefinition:        identifier : Node
TransitionDefinition:   identifier -> identifier : Node

identifier is any valid name (e.g., move, sequence, selector, parallel, MyCustomTask, fsm).

Foo(dong=5.1, ding=$1, )  by name
Foo(5.1, $1)              initializer list

FSM(s1:Foo, s2:Bar, s1->s2: Foo, s2->s1: Bar)
sequence[nloop(n=4)()[ Foo(bar=2), Foo[]()[] ]]
*/

namespace TBT {

  namespace Parser {

    [[nodiscard]] constexpr inline std::string_view to_view(const std::string& _s) noexcept {
      return std::string_view(_s.data(), _s.size());
    }  // to_view

    [[nodiscard]] constexpr inline char ascii_to_lower(const char c) noexcept {
      return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
    }  // ascii_to_lower

    enum class ErrorType {
      //
    };

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
      out.resize(j);
      return out;
    };  // clean

    constexpr auto is_valid = []([[maybe_unused]] const std::string_view& _in) -> bool {
      return true;  //
    };

    constexpr auto split = [](const std::string_view& _s, const char _delim) -> std::vector<std::string_view> {
      std::vector<std::string_view> result;
      size_t start = 0;
      size_t end   = _s.find(_delim);

      while (end != std::string::npos && end < _s.size()) {
        if (start == end)
          result.emplace_back();
        else
          result.push_back({&_s[start], end - start});
        start = end + 1;
        end   = _s.find(_delim, start);
      }

      if (start == _s.size())
        result.emplace_back();
      else
        result.push_back({&_s[start], _s.size() - start});
      return result;
    };

    //------------------------------------------------------

    struct Param_Value {
      std::string_view val;
    };

    struct Param_KeyValue {
      std::string_view key;
      std::string_view val;
    };

    using Parameter = std::variant<Param_Value, Param_KeyValue>;

    struct Default_Node;
    struct FSM_Node;
    using Node = std::variant<Default_Node, FSM_Node>;

    struct Default_Node {
      size_t idx_;
      std::string_view name_;
      std::vector<Parameter> options_;
      std::vector<Parameter> parameters_;
      std::vector<std::unique_ptr<Node>> children_;
    };  // Default_Node

    struct FSM_State {
      std::string_view id_;
      std::unique_ptr<Node> node_;
    };  // FSM_State

    struct FSM_Transition {
      std::string_view from_, to_;
    };

    struct FSM_Node {
      size_t idx_;
      std::vector<Parameter> options_;
      std::vector<FSM_State> states_;
      std::vector<FSM_Transition> transitions_;
    };  // FSM_Node

    //------------------------------------------------------

    // reads the next node_name (alphanumeric, extended with underscore for robustness)
    constexpr auto next_node_name = [](std::string_view& input) -> std::string_view {
      if (auto m = ctre::starts_with<"^[a-zA-Z0-9_]+">(input)) {
        const std::string_view out = m.get<0>().to_view();
        input.remove_prefix(m.get<0>().size());
        return out;
      }
      return {};
    };

    constexpr auto is_fsm = [](const std::string_view& input) -> bool {
      return input.size() == 3 && ascii_to_lower(input[0]) == 'f' && ascii_to_lower(input[1]) == 's' &&
             ascii_to_lower(input[2]) == 'm';
    };

    // reads the next options [key=value,value]
    constexpr auto next_options = [](std::string_view& input) -> std::vector<Parameter> {
      if (auto m_opt = ctre::starts_with<"^\\[([^)]*)\\]">(input)) {
        const std::string_view sv = m_opt.get<1>().to_view();
        if (sv.empty()) {
          input.remove_prefix(2);  // remove braces
          return {};
        }
        input.remove_prefix(m_opt.get<0>().size());
        const auto s = split(sv, ',');
        std::vector<Parameter> out;
        for (const auto& ss : s) {
          if (ctre::search<"=">(ss)) {
            const auto kv = split(ss, '=');
            out.push_back(Param_KeyValue{kv[0], kv[1]});
          } else {
            out.push_back(Param_Value{ss});
          }
        }
        return out;
      }
      return {};
    };

    // reads the next params (value,key=value,...)
    constexpr auto next_parameters = [](std::string_view& input) -> std::vector<Parameter> {
      if (auto m_opt = ctre::starts_with<"^\\(([^)]*)\\)">(input)) {
        const std::string_view sv = m_opt.get<1>().to_view();
        if (sv.empty()) {
          input.remove_prefix(2);  // remove braces
          return {};
        }
        input.remove_prefix(m_opt.get<0>().size());
        const auto s = split(sv, ',');
        std::vector<Parameter> out;
        for (const auto& ss : s) {
          if (ctre::search<"=">(ss)) {
            const auto kv = split(ss, '=');
            out.push_back(Param_KeyValue{kv[0], kv[1]});
          } else {
            out.push_back(Param_Value{ss});
          }
        }
        return out;
      }
      return {};
    };

    // reads the next identifier
    constexpr auto next_fsm_identifier = [](std::string_view& input) -> std::string_view {
      // find a name
      if (auto m = ctre::starts_with<"^[a-zA-Z0-9_]+">(input)) {
        const std::string_view out = m.get<0>().to_view();
        // the next token should be :
        if (input.size() < out.size() || input[out.size()] != ':') { return {}; }
        input.remove_prefix(out.size() + 1);  // +1 for :
        return out;
      }
      return {};
    };

    // reads the next transition
    constexpr auto next_fsm_transition = [](std::string_view& input) -> std::optional<FSM_Transition> {
      // find a name
      if (auto m = ctre::starts_with<"^[a-zA-Z0-9_]+">(input)) {
        FSM_Transition out;
        out.from_ = m.get<0>().to_view();

        // the next token should be a - and >
        if (input.size() < out.from_.size() ||
            (input[out.from_.size() + 1] != '-' && input[out.from_.size() + 1] != '>')) {
          return std::nullopt;
        }

        input.remove_prefix(out.from_.size() + 2);
        out.to_ = ctre::starts_with<"^[a-zA-Z0-9_]+">(input).get<0>().to_view();
        input.remove_prefix(out.to_.size());
        return out;
      }
      return std::nullopt;
    };

    // reads the next node
    constexpr auto extract = [](std::string_view& input) -> std::expected<Node, Error> {
      const auto impl = [&](std::string_view& input, auto& ref) -> std::expected<Node, Error> {
        const auto name = next_node_name(input);

        if (name.empty()) return std::unexpected(Error());

        //------------------------------------------------------
        // FSM

        if (is_fsm(name)) {
          FSM_Node node;
          // optional options
          node.options_ = next_options(input);

          if (auto m_b = ctre::starts_with<"^\\(">(input)) {
            input.remove_prefix(1);
            while (!input.empty()) {
              //
              // closing ')' ?
              if (auto close_m = ctre::starts_with<"^\\)">(input)) {
                input.remove_prefix(close_m.get<0>().size());
                break;
              }

              // optional comma separator
              if (auto comma_m = ctre::starts_with<"^,">(input)) { input.remove_prefix(comma_m.get<0>().size()); }

              const auto trans = next_fsm_transition(input);
              if (trans) {
                node.transitions_.push_back(std::move(trans.value()));
                continue;
              }

              FSM_State id;
              const auto identifier = next_fsm_identifier(input);
              if (!identifier.empty()) {
                id.id_ = identifier;
              } else {
                // we didnt reach the end but it isnt a transition or an identifier -> error
                return std::unexpected(Error());
              }

              // recursive child node
              if (auto child = ref(input, ref)) {
                if (child) {
                  id.node_ = std::make_unique<Node>(std::move(*child));
                  node.states_.push_back(std::move(id));
                } else
                  return std::unexpected(child.error());

              } else {
                break;
              }
            }
          } else {
            // fsm needs the () brackets
            return std::unexpected(Error());
          }
          return node;
        }

        //------------------------------------------------------
        // Hierarchy

        Default_Node node;
        node.name_       = name;
        node.options_    = next_options(input);
        node.parameters_ = next_parameters(input);

        // [ -> node -> ]

        // children
        if (auto m_b = ctre::starts_with<"^\\[">(input)) {
          input.remove_prefix(m_b.get<0>().size());

          bool expect_comma = false;
          while (!input.empty()) {
            // closing ']' ?
            if (auto close_m = ctre::starts_with<"^\\]">(input)) {
              input.remove_prefix(close_m.get<0>().size());
              break;
            }

            // optional comma separator
            if (expect_comma) {
              if (auto comma_m = ctre::starts_with<"^,">(input)) {
                input.remove_prefix(comma_m.get<0>().size());
              } else {
                break;  // malformed – abort gracefully
              }
            }

            // recursive child node
            if (auto child = ref(input, ref)) {
              if (child)
                node.children_.push_back(std::make_unique<Node>(std::move(*child)));
              else
                return std::unexpected(child.error());
              expect_comma = true;
            } else {
              break;
            }
          }
        }

        return node;
      };
      return impl(input, impl);
    };

    constexpr auto root_extract = [](std::string_view& input) -> std::expected<Default_Node, Error> {
      Default_Node root;
      root.name_        = "root";

      bool expect_comma = false;
      while (!input.empty()) {
        // optional comma separator
        if (expect_comma) {
          if (auto comma_m = ctre::starts_with<"^,">(input)) {
            input.remove_prefix(comma_m.get<0>().size());
          } else {
            break;  // malformed – abort gracefully
          }
        }

        // recursive child node
        if (auto child = extract(input)) {
          if (child)
            root.children_.push_back(std::make_unique<Node>(std::move(*child)));
          else
            return std::unexpected(child.error());
          expect_comma = true;
        } else {
          break;
        }
      }

      return root;
    };

    constexpr auto count_children_nodes = [](const Parser::Node& _node) -> size_t {
      size_t out = 0;
      std::vector<const Node*> q;
      q.push_back(&_node);
      while (!q.empty()) {
        out++;
        const Node* nn = q.back();
        q.pop_back();

        // default nodes
        if (const auto* def = std::get_if<Default_Node>(nn)) {
          for (const auto& c : def->children_) { q.push_back(c.get()); }
          continue;
        }

        // FSM
        const auto& fsm = std::get<FSM_Node>(*nn);
        for (const auto& [_, node] : fsm.states_) { q.push_back(node.get()); }
      }
      return out;
    };

    [[nodiscard]] constexpr inline std::expected<size_t, Parser::Error> count_nodes(const std::string_view& _tree) {
      const std::string ctree = clean(_tree);
      std::string_view to_ex  = to_view(ctree);

      const auto root         = root_extract(to_ex);
      if (!root) return std::unexpected(std::move(root.error()));

      size_t out = 1;
      for (const auto& n : root.value().children_) out += count_children_nodes(*n);
      return out;
    }  // count_nodes

  }  // namespace Parser

}  // namespace TBT