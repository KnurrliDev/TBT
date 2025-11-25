#pragma once

#include <TBT/execute.hpp>

namespace TBT {

  enum ExecutionMode { STEPWISE_1, STEPWISE_INF, FULL_1, FULL_INF };

  using TaskQueue = std::forward_list<std::tuple<ExecutionMode, std::function<TBT::State()>, std::promise<TBT::State>>>;

#define EXECUTE_QUEUE(state_provider)                         \
  if (!states.tasks_queue_.empty()) {                         \
    auto prev = states.tasks_queue_.before_begin();           \
    auto curr = states.tasks_queue_.begin();                  \
    while (true) {                                            \
      if (!states.tasks_queue_.empty()) {                     \
        auto prev = states.tasks_queue_.before_begin();       \
        auto curr = states.tasks_queue_.begin();              \
        while (curr != states.tasks_queue_.end()) {           \
          auto& [mode, next, prm] = *curr;                    \
                                                              \
          switch (mode) {                                     \
            case STEPWISE_1: {                                \
              TBT::State r = next();                          \
              if (r != TBT::BUSY) {                           \
                prm.set_value(r);                             \
                curr = states.tasks_queue_.erase_after(prev); \
              }                                               \
            }                                                 \
            case STEPWISE_INF: {                              \
              next();                                         \
            }                                                 \
            case FULL_1: {                                    \
              TBT::State r = TBT::State::SUCCESS;             \
              while ((r = next()) != TBT::BUSY) {}            \
              prm.set_value(r);                               \
              curr = states.tasks_queue_.erase_after(prev);   \
            }                                                 \
            case FULL_INF: {                                  \
              while ((next()) != TBT::BUSY) {}                \
            }                                                 \
          }                                                   \
        }                                                     \
      }                                                       \
    }                                                         \
  }

#define COMPILE_AND_PREPARE(tree, states, ...)                                      \
  Execute::prepare<typename decltype(states)::Variant>(                             \
      compile_static<compute_size_static<typename decltype(states)::Variant>(tree), \
                     typename decltype(states)::Variant>(tree),                     \
      states __VA_OPT__(, ) __VA_ARGS__);

  template <class Variant, class Tree, class StateProvider, class... Ts>
  [[nodiscard]] auto compile_and_prepare(const std::string_view& _tree, StateProvider& _states, Ts... _ts) {
    return Execute::prepare<Variant>(compile_dynamic<Variant>(_tree), _states, std::forward<Ts>(_ts)...);
  }  // d_compile_and_prepare

#define COMPILE_AND_QUEUE(tree, states, mode, ...)                                                        \
  [&]() -> auto {                                                                                         \
    std::promise<TBT::State> prm;                                                                         \
    std::future<TBT::State> out = prm.get_future();                                                       \
    const auto ct               = COMPILE_AND_PREPARE(tree, states, __VA_ARGS__);                         \
    auto& ref                   = states.tasks_queue_.emplace_front(mode, std::move(ct), std::move(prm)); \
    return std::make_pair(std::move(out), std::ref(ref));                                                 \
  }();

#define COMPILE_AND_QUEUE_STEPWISE_1(tree, states, ...) COMPILE_AND_QUEUE(tree, states, STEPWISE_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_STEPWISE_INF(tree, states, ...) COMPILE_AND_QUEUE(tree, states, STEPWISE_INF, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_1(tree, states, ...) COMPILE_AND_QUEUE(tree, states, FULL_1, __VA_ARGS__)
#define COMPILE_AND_QUEUE_FULL_INF(tree, states, ...) COMPILE_AND_QUEUE(tree, states, FULL_INF, __VA_ARGS__)

}  // namespace TBT