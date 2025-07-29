#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/execute/composite.hpp>
#include <TBT/execute/decorator.hpp>
#include <TBT/execute/task.hpp>

namespace TBT {

  template <class NodeTypes, class StateProvider, class... Ts>
  State execute_step(std::string& _bc, StateProvider&& _states, const std::tuple<Ts...>& _params) {
    using namespace Execute;
    using namespace States;

    // read global header
    Header global_header             = read_global_node_header(_bc.data());

    // if first entry. if yes set the pointer to the root node
    // [global header][root][...]
    const bool first_entry           = global_header.ptr_ < sizeof(Header);
    global_header.ptr_               = std::max<uint32_t>(global_header.ptr_, sizeof(Header));

    // read header of current node
    char* cur_node                   = &_bc[global_header.ptr_];
    const NodeHeader cur_node_header = read_node_header(cur_node);

    State res                        = SUCCESS;
    switch (cur_node_header.vidx_) {
      case vidx_root:
        res = Execute::execute_root(cur_node, global_header, cur_node_header);
        break;
      case vidx_sequence: {
        res = Execute::execute_sequence(cur_node, global_header, cur_node_header);
        break;
      }
      case vidx_fallback: {
        res = Execute::execute_fallback(cur_node, global_header, cur_node_header);
        break;
      }
      case vidx_nloop: {
        res = Execute::execute_nloop(cur_node, global_header, cur_node_header);
        break;
      }
      case vidx_cooldown: {
        res = Execute::execute_cooldown(cur_node, global_header, cur_node_header);
        break;
      }
      case vidx_always_fail: {
        res = Execute::execute_always_fail(cur_node, global_header, cur_node_header);
        break;
      }
      case vidx_always_succeed: {
        res = Execute::execute_always_succeed(cur_node, global_header, cur_node_header);
        break;
      }
      default:  // task
      {
        assert(cur_node_header.vidx_ >= 0 || cur_node_header.vidx_ < std::variant_size_v<NodeTypes>);
        res = Execute::execute_task<NodeTypes>(cur_node, global_header, cur_node_header, _states, _params);
        break;
      }
    }

    Execute::write_global_node_header(global_header, _bc.data());

    return res;

  }  // execute

  // prepares for the execution of a tree
  template <class NodeTypes, class StateProvider, class... Ts>
  std::function<State()> prepare(std::string _tree, StateProvider&& _states, Ts... _ts) {
    return
        [tree = std::move(_tree), states = std::ref(_states), params = std::make_tuple(std::move(_ts)...)]() -> State {
          return execute_step<NodeTypes>(*const_cast<std::string*>(&tree), states.get(), params);
        };
  }  // prepare
}  // namespace TBT