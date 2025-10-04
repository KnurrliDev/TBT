#pragma once

#include <game/tbt/execute/helper.hpp>
#include <game/tbt/execute/meta.hpp>
#include <game/tbt/execute/states.hpp>

namespace TBT {

  namespace Execute {

    template <class NodeTypes, class StateProvider, class... Ts>
    State execute_task(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header,
                       StateProvider&& _states, const std::tuple<Ts...>& _params) {
      using namespace States;

      Task task = read_node_state<Task>(_cur_node);

      // first time entering the task
      if (_global_header.last_result_.dir_ == DOWN) {
        // create mapping
        std::vector<uint32_t> idcs;

        std::vector<std::variant<bool, int32_t, float, uint32_t>> payloads;

        std::vector<int32_t> d_ics;

        uint32_t s_pl = 0;
        for (int32_t i = 0; i < _header.payload_count_; ++i) {
          const auto pl = read_payload(i, _header, _cur_node);

          switch (pl.index()) {
            case 3: {
              idcs.push_back(std::get<3>(pl));  // idx into dynamic params
              d_ics.push_back(i);
            } break;
            default:
              payloads.push_back(std::move(pl));
              idcs.push_back(s_pl++);  // idx into static params
              break;
          }
        }

        // add offset to dynamic params
        for (const int32_t i : d_ics) idcs[i] += (uint32_t)payloads.size();

        NodeTypes* state = alloc_task<NodeTypes>(_header.vidx_, idcs, payloads, _params);

        // run the task
        {
          State res = FAILED;
          std::visit(
              [&](auto& _t) {
                if constexpr (Concepts::has_run_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  res = run(_t, _states);
                else if constexpr (Concepts::has_run_sig_2<std::decay_t<decltype(_t)>>)
                  res = run(_t);
                // else
                //   static_assert(false, "no signature found");
              },
              *state);

          // the task failed. return to parent
          if (res == FAILED) {
            // call exit if present
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);

            delete state;
            _global_header.ptr_                = _header.parent_;
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = res;
            return BUSY;
          }
        }

        // the task succeeded. check if wait exists, else return
        {
          std::optional<State> res;
          std::visit(
              [&](auto& _t) {
                if constexpr (Concepts::has_wait_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  res = wait(_t, _states);
                else if constexpr (Concepts::has_wait_sig_2<std::decay_t<decltype(_t)>>)
                  res = wait(_t);
              },
              *state);

          // no wait signature found and run succeed: task is done
          // if the task is not busy return to parent
          if (!res || (res && (*res == FAILED || *res == SUCCESS))) {
            // call exit if present
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);

            delete state;
            _global_header.ptr_                = _header.parent_;
            _global_header.last_result_.dir_   = UP;
            // either wait has returned a state or the state must be success from run
            _global_header.last_result_.state_ = res ? *res : SUCCESS;
            return BUSY;
          }
          // the tasks wants to wait. write states and return.
          task.ptr_                          = reinterpret_cast<intptr_t>(state);
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = *res;
          write_node_state<Task>(task, _cur_node);
          return BUSY;
        }
      }

      // re enter. this only happens if the task has wait and returned busy
      else {
        assert(task.ptr_ != 0);
        NodeTypes* state = reinterpret_cast<NodeTypes*>(task.ptr_);

        State res        = FAILED;
        std::visit(
            [&](auto& _t) {
              if constexpr (Concepts::has_wait_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                res = wait(_t, _states);
              else if constexpr (Concepts::has_wait_sig_2<std::decay_t<decltype(_t)>>)
                res = wait(_t);
            },
            *state);

        // task is finished
        if (res == FAILED || res == SUCCESS) {
          // call exit if present
          std::visit(
              [&](auto& _t) {
                if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  exit(_t, _states);
                else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                  exit(_t);
              },
              *state);

          delete state;
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = res;
          return BUSY;
        }

        // wait longer
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = res;
        return BUSY;
      }
      return BUSY;
    }  // execute_task

  }  // namespace Execute

}  // namespace TBT