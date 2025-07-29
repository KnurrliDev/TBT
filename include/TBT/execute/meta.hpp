#pragma once

#include <game/tbt/defines.hpp>

namespace TBT {

  namespace Execute {
    namespace Concepts {

      //-----------------------------------
      // concepts for all valid run signatures

      template <class TaskDerived, class StateProvider>
      concept has_run_sig_1 = requires(TaskDerived task, StateProvider state) {
        { run(task, state) } -> std::same_as<State>;
      };

      template <class TaskDerived>
      concept has_run_sig_2 = requires(TaskDerived task) {
        { run(task) } -> std::same_as<State>;
      };

      template <class TaskDerived, class StateProvider>
      concept has_any_run_sig = has_run_sig_1<TaskDerived, StateProvider> || has_run_sig_2<TaskDerived>;

      //-----------------------------------
      // concepts for all valid wait signatures

      template <class TaskDerived, class StateProvider>
      concept has_wait_sig_1 = requires(TaskDerived task, StateProvider state) {
        { wait(task, state) } -> std::same_as<State>;
      };

      template <class TaskDerived>
      concept has_wait_sig_2 = requires(TaskDerived task) {
        { wait(task) } -> std::same_as<State>;
      };

      template <class TaskDerived, class StateProvider>
      concept has_any_wait_sig = has_wait_sig_1<TaskDerived, StateProvider> || has_wait_sig_2<TaskDerived>;

      template <class TaskDerived, class StateProvider>
      concept has_any_sig = has_any_run_sig<TaskDerived, StateProvider> || has_any_wait_sig<TaskDerived, StateProvider>;

    }  // namespace Concepts
  }  // namespace Execute
}  // namespace TBT