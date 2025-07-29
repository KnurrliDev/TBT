#pragma once

#include <game/tbt/execute/helper.hpp>
#include <game/tbt/execute/meta.hpp>
#include <game/tbt/execute/states.hpp>

namespace TBT {

  namespace Execute {

    State execute_root(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      // root has no state

      // if no child the tree is done
      if (_header.children_count_ == 0) return SUCCESS;

      // re-enter root. return the last state and finish
      if (_global_header.last_result_.dir_ == UP) {
        assert(_global_header.last_result_.state_ != BUSY);
        return _global_header.last_result_.state_;
      }

      // move to child
      const uint32_t child               = Execute::read_child(0, _header, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;
      return BUSY;

    }  // execute_root

    State execute_sequence(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      Composite node = read_node_state<Composite>(_cur_node);

      // no children. return to parent with success
      if (_header.children_count_ == 0) {
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = SUCCESS;
        return BUSY;
      }

      // first time entering the sequence. reset the state
      if (_global_header.last_result_.dir_ == DOWN) {
        // execute first child
        node.cur_idx_ = 0;
        Execute::write_node_state(node, _cur_node);

      } else {
        // if a child failed the sequence fails.
        if (_global_header.last_result_.state_ == FAILED) {
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = FAILED;
          return BUSY;
        }

        // the last child succeeded. returning to parent with success
        if (node.cur_idx_ >= _header.children_count_ - 1) {
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = SUCCESS;
          return BUSY;
        }

        node.cur_idx_ += 1;

        Execute::write_node_state(node, _cur_node);
      }

      // execute the child
      const uint32_t child               = Execute::read_child(node.cur_idx_, _header, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;

      return BUSY;

    }  // execute_sequence

    State execute_fallback(char* _cur_node, States::Header& _global_header, const States::NodeHeader& _header) {
      using namespace States;

      Composite node = read_node_state<Composite>(_cur_node);

      // no children. return to parent with failed
      if (_header.children_count_ == 0) {
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = FAILED;
        return BUSY;
      }

      // first time entering the fallback. reset the state
      if (_global_header.last_result_.dir_ == DOWN) {
        // execute first child
        node.cur_idx_ = 0;
        Execute::write_node_header(_header, _cur_node);

      } else {
        // if a child succeeds the fallback succeeds
        if (_global_header.last_result_.state_ == SUCCESS) {
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = SUCCESS;
          return BUSY;
        }

        // the last child failed. returning to parent with failed
        if (node.cur_idx_ >= _header.children_count_ - 1) {
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = FAILED;
          return BUSY;
        }

        node.cur_idx_ += 1;

        Execute::write_node_header(_header, _cur_node);
      }

      // execute the child
      const uint32_t child               = Execute::read_child(node.cur_idx_, _header, _cur_node);

      _global_header.ptr_                = child;
      _global_header.last_result_.dir_   = DOWN;
      _global_header.last_result_.state_ = SUCCESS;

      return BUSY;

    }  // execute_fallback

  }  // namespace Execute
}  // namespace TBT