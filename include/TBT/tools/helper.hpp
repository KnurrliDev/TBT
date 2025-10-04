#pragma once

#include <TBT/common/defines.hpp>

namespace TBT {

  void exe() {
    for (auto it = proxy.tasks_.tasks_.begin(); it != proxy.tasks_.tasks_.end();) {
      auto& [state, next, prm] = *it;

      if (state == 0) {
        TBT::State r = TBT::State::SUCCESS;
        while ((r = next()) != TBT::BUSY) {}
        prm.set_value(r);
        it = proxy.tasks_.tasks_.erase(it);
      } else {
        bool finished = false;
        for (int32_t i = 0; i < state; ++i) {
          const TBT::State r = next();
          if (r != TBT::BUSY) {
            prm.set_value(r);
            finished = true;
            break;
          }
        }
        if (finished) {
          it = proxy.tasks_.tasks_.erase(it);
          continue;
        };
      }
      ++it;
    }
  }

  template <class TaskTypes>
  [[nodiscard]] std::string compile_tree(RenderProxy<TaskTypes>& _proxy, const std::string& _tree) {
    static const tsl::robin_map<std::string, int16_t> types = TBT::create_mapping<TaskTypes>();
    const std::shared_ptr<TBT::Detail::Node> root           = TBT::build(_tree, types);
    return TBT::compile(root);
  }  // compile_tree

  template <class TaskTypes, class... Ts>
  std::function<TBT::State()> prepare_tree(RenderProxy<TaskTypes>& _proxy, std::string _tree, Ts&&... ts) {
    return TBT::prepare<TaskTypes>(std::move(_tree), _proxy, std::forward<Ts>(ts)...);
  }  // prepare_tree

  // state: 0 - runs until done, >0 state steps/ iteration
  template <class TaskTypes>
  std::future<TBT::State> queue_tree(RenderProxy<TaskTypes>& _proxy, std::function<TBT::State()> _tree,
                                     const int32_t _state) {
    std::promise<TBT::State> prm;
    std::future<TBT::State> out = prm.get_future();
    _proxy.tasks_.cache_.emplace(_state, std::move(_tree), std::move(prm));
    return out;
  }  // queue_tree

  template <class TaskTypes, class... Ts>
  std::future<TBT::State> queue_tree(RenderProxy<TaskTypes>& _proxy, const std::string& _tree, const int32_t _state,
                                     Ts&&... ts) {
    auto t = prepare_tree(_proxy, compile_tree(_proxy, _tree), std::forward<Ts>(ts)...);
    return queue_tree(_proxy, std::move(t), _state);
  }  // run_tree_from_cache

  template <class TaskTypes, class... Ts>
  TBT::State run_tree(RenderProxy<TaskTypes>& _proxy, const std::string& _tree, Ts&&... ts) {
    auto t         = prepare_tree(_proxy, compile_tree(_proxy, _tree), std::forward<Ts>(ts)...);
    TBT::State res = TBT::State::SUCCESS;
    while ((res = t()) == TBT::BUSY) {}
    return res;
  }  // run_tree_from_cache

  //------------------------------------------------------

  template <class TaskTypes>
  void compile_and_cache_tree(RenderProxy<TaskTypes>& _proxy, const std::string& _id, const std::string& _tree) {
    _proxy.tasks_.tree_cache_[_id] = compile_tree(_proxy, _tree);
  }  // compile_and_cache_tree

  template <class TaskTypes, class... Ts>
  std::function<TBT::State()> prepare_from_cache(RenderProxy<TaskTypes>& _proxy, const std::string& _id, Ts&&... ts) {
    return prepare_tree(_proxy, _proxy.tasks_.tree_cache_.at(_id), std::forward<Ts>(ts)...);
  }  // prepare_from_cache

  // state: 0 - runs until done, >0 state steps/ iteration
  template <class TaskTypes, class... Ts>
  std::future<TBT::State> queue_tree_from_cache(RenderProxy<TaskTypes>& _proxy, const std::string& _id,
                                                const int32_t _state, Ts&&... ts) {
    assert(_proxy.tasks_.tree_cache_.contains(_id));
    return queue_tree(_proxy, prepare_from_cache(_proxy, _id, std::forward<Ts>(ts)...), _state);
  }  // queue_tree_from_cache

  template <class TaskTypes, class... Ts>
  TBT::State run_tree_from_cache(RenderProxy<TaskTypes>& _proxy, const std::string& _id, Ts&&... ts) {
    assert(_proxy.tasks_.tree_cache_.contains(_id));
    auto t         = prepare_from_cache(_proxy, _id, std::forward<Ts>(ts)...);
    TBT::State res = TBT::State::SUCCESS;
    while ((res = t()) == TBT::BUSY) {}
    return res;
  }  // run_tree_from_cache

}  // namespace TBT