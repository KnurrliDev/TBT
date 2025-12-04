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

    std::shared_ptr<Execute::CoStateValues> values_;
    int32_t priority_ = 0;
    ExecutionMode mode_;
    std::function<TBT::State()> tree_;
    std::promise<TBT::State> promise_;
  };  // ExecutionItem

  template <class Allocator = std::allocator<ExecutionItem>>
  using TreeRef = typename std::forward_list<ExecutionItem, Allocator>::iterator;

  template <class T, class Allocator = std::allocator<T>>
  struct TreeAwaitable {
    std::future<TBT::State> future_;
    TreeRef<Allocator> ref_;

    bool await_ready() noexcept { return false; }

    void await_suspend(const std::coroutine_handle<Execute::CoState::promise_type>&) {}
    State await_resume() noexcept { return future_.get(); }

  };  // TreeAwaitable

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

#define EXECUTE_QUEUE(state_provider)                                                      \
  if (!state_provider.tasks_queue_.q_.empty()) {                                           \
    if (state_provider.tasks_queue_.dirty_) {                                              \
      state_provider.tasks_queue_.dirty_ = false;                                          \
      state_provider.tasks_queue_.q_.sort(                                                 \
          [](const auto& _v1, const auto& _v2) { return _v1.priority_ > _v2.priority_; }); \
    }                                                                                      \
    auto prev = state_provider.tasks_queue_.q_.before_begin();                             \
    auto curr = state_provider.tasks_queue_.q_.begin();                                    \
    while (curr != state_provider.tasks_queue_.q_.end()) {                                 \
      ExecutionItem& item = *curr;                                                         \
      switch (item.mode_) {                                                                \
        case TBT::STEPWISE_1: {                                                            \
          TBT::State r = item.tree_();                                                     \
          if (r != TBT::BUSY) {                                                            \
            item.promise_.set_value(r);                                                    \
            if (item.values_) item.values_->a_done_ = true;                                \
            curr = state_provider.tasks_queue_.q_.erase_after(prev);                       \
          }                                                                                \
          break;                                                                           \
        }                                                                                  \
        case TBT::STEPWISE_INF: {                                                          \
          item.tree_();                                                                    \
          break;                                                                           \
        }                                                                                  \
        case FULL_1: {                                                                     \
          TBT::State r = TBT::State::SUCCESS;                                              \
          while ((r = item.tree_()) != TBT::BUSY) {}                                       \
          item.promise_.set_value(r);                                                      \
          curr = state_provider.tasks_queue_.q_.erase_after(prev);                         \
          break;                                                                           \
        }                                                                                  \
        case TBT::FULL_INF: {                                                              \
          while ((item.tree_()) != TBT::BUSY) {}                                           \
          break;                                                                           \
        }                                                                                  \
      }                                                                                    \
      if (curr != state_provider.tasks_queue_.q_.end()) curr++;                            \
    }                                                                                      \
  }

#ifdef __INTELLISENSE__
#define COMPILE_AND_PREPARE(...) []() -> std::function<State()> { return []() { return SUCCESS; }; }();
#else
#define COMPILE_AND_PREPARE(tree, states, ...)                                                    \
  Execute::prepare<typename std::decay_t<decltype(states)>::Variant>(                             \
      compile_static<compute_size_static<typename std::decay_t<decltype(states)>::Variant>(tree), \
                     typename std::decay_t<decltype(states)>::Variant>(tree),                     \
      states __VA_OPT__(, ) __VA_ARGS__);
#endif

  template <class Variant, class Tree, class StateProvider, class... Ts>
  [[nodiscard]] auto compile_and_prepare(const std::string_view& _tree, StateProvider& _states, Ts... _ts) {
    return Execute::prepare<Variant>(compile_dynamic<Variant>(_tree), _states, std::forward<Ts>(_ts)...);
  }  // d_compile_and_prepare

#define COMPILE_AND_QUEUE(priority, tree, state_provider, mode, ...)                                         \
  [&]() -> auto {                                                                                            \
    ExecutionItem item;                                                                                      \
    item.priority_                     = priority;                                                           \
    item.mode_                         = mode;                                                               \
    item.tree_                         = COMPILE_AND_PREPARE(tree, state_provider, __VA_ARGS__);             \
    state_provider.tasks_queue_.dirty_ = true;                                                               \
    std::future<TBT::State> f          = item.promise_.get_future();                                         \
    auto prev                          = state_provider.tasks_queue_.q_.before_begin();                      \
    auto ref                           = state_provider.tasks_queue_.q_.insert_after(prev, std::move(item)); \
    TreeAwaitable<ExecutionItem> out;                                                                        \
    out.future_ = std::move(f);                                                                              \
    out.ref_    = ref;                                                                                       \
    return out;                                                                                              \
  }();

#define COMPILE_AND_QUEUE_STEPWISE_1(priority, tree, state_provider, ...) \
  COMPILE_AND_QUEUE(priority, tree, state_provider, TBT::STEPWISE_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_STEPWISE_INF(priority, tree, state_provider, ...) \
  COMPILE_AND_QUEUE(priority, tree, state_provider, TBT::STEPWISE_INF, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_1(priority, tree, state_provider, ...) \
  COMPILE_AND_QUEUE(priority, tree, state_provider, TBT::FULL_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_INF(priority, tree, state_provider, ...) \
  COMPILE_AND_QUEUE(priority, tree, state_provider, TBT::FULL_INF, __VA_ARGS__)

}  // namespace TBT