#pragma once

#include <TBT/execute.hpp>

namespace TBT {

  enum ExecutionMode { STEPWISE_1, STEPWISE_INF, FULL_1, FULL_INF };

  struct ExecutionItem {
    ExecutionItem()                                = default;

    ExecutionItem(ExecutionItem&&)                 = default;
    ExecutionItem& operator=(ExecutionItem&&)      = default;

    ExecutionItem(const ExecutionItem&)            = delete;
    ExecutionItem& operator=(const ExecutionItem&) = delete;

    int32_t priority_                              = 0;
    ExecutionMode mode_;
    std::function<TBT::State()> tree_;
    std::promise<TBT::State> promise_;
  };  // ExecutionItem

  /*
    The TaskQueue needs fullfill multiple requirements:
      > removing items without changing the order
      > removing items while iterating
      > sorteable
      > the items shouldnt be copied around. the lambdas are potentially very heavy
      > memory fragmentation and pointer chasing can be adressed using an allocator
  */

  template <class Allocator = std::allocator<ExecutionItem>>
  struct TaskQueue {
    std::forward_list<ExecutionItem, Allocator> q_;
    bool dirty_ = true;
  };  // TaskQueue

#define EXECUTE_QUEUE(state_provider)                                                                              \
  if (!states.tasks_queue_.q_.empty()) {                                                                           \
    if (state_provider.tasks_queue_.dirty_) {                                                                      \
      state_provider.tasks_queue_.dirty_ = false;                                                                  \
      states.tasks_queue_.q_.sort([](const auto& _v1, const auto& _v2) { return _v1.priority_ > _v2.priority_; }); \
    }                                                                                                              \
    auto prev = states.tasks_queue_.q_.before_begin();                                                             \
    auto curr = states.tasks_queue_.q_.begin();                                                                    \
    while (curr != states.tasks_queue_.q_.end()) {                                                                 \
      ExecutionItem& item = *curr;                                                                                 \
      switch (item.mode_) {                                                                                        \
        case STEPWISE_1: {                                                                                         \
          TBT::State r = item.tree_();                                                                             \
          if (r != TBT::BUSY) {                                                                                    \
            item.promise_.set_value(r);                                                                            \
            curr = states.tasks_queue_.q_.erase_after(prev);                                                       \
          }                                                                                                        \
        }                                                                                                          \
        case STEPWISE_INF: {                                                                                       \
          item.tree_();                                                                                            \
        }                                                                                                          \
        case FULL_1: {                                                                                             \
          TBT::State r = TBT::State::SUCCESS;                                                                      \
          while ((r = item.tree_()) != TBT::BUSY) {}                                                               \
          item.promise_.set_value(r);                                                                              \
          curr = states.tasks_queue_.q_.erase_after(prev);                                                         \
        }                                                                                                          \
        case FULL_INF: {                                                                                           \
          while ((item.tree_()) != TBT::BUSY) {}                                                                   \
        }                                                                                                          \
      }                                                                                                            \
    }                                                                                                              \
  }
#ifdef __INTELLISENSE__
#define COMPILE_AND_PREPARE(...) []() -> std::function<State()> { return []() { return SUCCESS; }; }();
#else
#define COMPILE_AND_PREPARE(tree, states, ...)                                      \
  Execute::prepare<typename decltype(states)::Variant>(                             \
      compile_static<compute_size_static<typename decltype(states)::Variant>(tree), \
                     typename decltype(states)::Variant>(tree),                     \
      states __VA_OPT__(, ) __VA_ARGS__);

  template <class Variant, class Tree, class StateProvider, class... Ts>
  [[nodiscard]] auto compile_and_prepare(const std::string_view& _tree, StateProvider& _states, Ts... _ts) {
    return Execute::prepare<Variant>(compile_dynamic<Variant>(_tree), _states, std::forward<Ts>(_ts)...);
  }  // d_compile_and_prepare
#endif

#define COMPILE_AND_QUEUE(priority, tree, state_provider, mode, ...)                                           \
  [&]() -> auto {                                                                                              \
    ExecutionItem item;                                                                                        \
    item.priority_ = priority;                                                                                 \
    item.mode_     = mode;                                                                                     \
    item.tree_     = COMPILE_AND_PREPARE("TaskC, TaskA($0)[TaskB(5)[TaskA, TaskB]] TaskA[TaskC]", states, -5); \
    state_provider.tasks_queue_.dirty_ = true;                                                                 \
    std::future<TBT::State> out        = item.promise_.get_future();                                           \
    auto prev                          = state_provider.tasks_queue_.q_.before_begin();                        \
    auto ref                           = state_provider.tasks_queue_.q_.insert_after(prev, std::move(item));   \
    return std::make_pair(std::move(out), ref);                                                                \
  }();

#define COMPILE_AND_QUEUE_STEPWISE_1(tree, state_provider, ...) \
  COMPILE_AND_QUEUE(tree, state_provider, STEPWISE_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_STEPWISE_INF(tree, state_provider, ...) \
  COMPILE_AND_QUEUE(tree, state_provider, STEPWISE_INF, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_1(tree, state_provider, ...) COMPILE_AND_QUEUE(tree, state_provider, FULL_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_INF(tree, state_provider, ...) \
  COMPILE_AND_QUEUE(tree, state_provider, FULL_INF, __VA_ARGS__)

}  // namespace TBT