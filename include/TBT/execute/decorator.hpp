#pragma once

#include <game/tbt/execute/helper.hpp>
#include <game/tbt/execute/meta.hpp>
#include <game/tbt/execute/states.hpp>

namespace TBT {

  namespace Execute {

    State execute_nloop(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      NLoop node = read_node_state<NLoop>(_cur_node);

      // first time entering the nloop. reset the state
      if (_global_header.last_result_.dir_ == DOWN) {
        const auto max_its  = read_payload(0, _header, _cur_node);
        node.max_it_        = std::get<int32_t>(max_its);
        node.inf_loop_      = node.max_it_ < 0;
        node.cur_it_        = 0;
        node.break_on_fail_ = false;
        if (_header.payload_count_ > 1) {
          const auto breakfail = read_payload(1, _header, _cur_node);
          node.break_on_fail_  = std::get<bool>(breakfail);
        }
        write_node_state(node, _cur_node);
      }

      // no children or no iterations
      if (_header.children_count_ == 0 || node.max_it_ == 0) {
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = SUCCESS;
        return BUSY;
      }

      // if the loop should terminate when child returns failed
      if (_global_header.last_result_.state_ == FAILED && node.break_on_fail_) {
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = FAILED;
        return BUSY;
      }

      // last child reached and not in an infinite loop
      if (node.max_it_ > 0 && node.cur_it_ >= node.max_it_) {
        _global_header.ptr_ = _header.parent_;
        return BUSY;
      }

      const uint32_t child = Execute::read_child(0, _header, _cur_node);

      // next iteration
      node.cur_it_ += 1;
      write_node_state(node, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;
      return BUSY;
    }  // execute_nloop

    State execute_always_succeed(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      assert(_header.children_count_ == 1);

      // re-enter decorator. return succeeded
      if (_global_header.last_result_.dir_ == UP) {
        assert(_global_header.last_result_.state_ != BUSY);
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = SUCCESS;
        return BUSY;
      }

      // move to child
      const uint32_t child               = Execute::read_child(0, _header, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;

      return BUSY;
    }  // execute_always_succeed

    State execute_always_fail(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      assert(_header.children_count_ == 1);

      // re-enter decorator. return failed
      if (_global_header.last_result_.dir_ == UP) {
        assert(_global_header.last_result_.state_ != BUSY);
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = FAILED;
        return BUSY;
      }

      // move to child
      const uint32_t child               = Execute::read_child(0, _header, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;

      return BUSY;
    }  // execute_always_fail

    State execute_cooldown(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      assert(_header.children_count_ == 1);

      // re-enter decorator. pass through
      if (_global_header.last_result_.dir_ == UP && _global_header.last_result_.state_ != BUSY) {
        _global_header.ptr_ = _header.parent_;
        return BUSY;
      }

      Cooldown state = read_node_state<Cooldown>(_cur_node);

      // first time entering the cd. reset the state
      if (_global_header.last_result_.dir_ == DOWN && state.is_first_) {
        state.is_first_ = false;
        const auto now  = std::chrono::high_resolution_clock::now();
        state.start_    = now.time_since_epoch().count();
        const auto dt   = read_payload(0, _header, _cur_node);
        switch (dt.index()) {
          case 1:
            state.dt_ = std::get<int32_t>(dt);
            break;
          case 3:
            state.dt_ = std::get<uint32_t>(dt);
            break;
          default:
            state.dt_ = 0;
        }

        Execute::write_node_state(state, _cur_node);
      }

      const auto d =
          std::chrono::high_resolution_clock::now().time_since_epoch() - std::chrono::nanoseconds(state.start_);

      const auto dd = std::chrono::milliseconds(state.dt_);

      // guard child until time is up
      if (d >= dd) {
        // reset state
        state.is_first_ = true;
        Execute::write_node_state(state, _cur_node);
        // move to child
        const uint32_t child               = Execute::read_child(0, _header, _cur_node);

        _global_header.ptr_                = child;
        _global_header.last_result_.dir_   = DOWN;
        _global_header.last_result_.state_ = SUCCESS;
        return BUSY;
      }

      _global_header.last_result_.dir_   = UP;
      _global_header.last_result_.state_ = BUSY;

      return BUSY;

    }  // execute_cooldown

  }  // namespace Execute

}  // namespace TBT