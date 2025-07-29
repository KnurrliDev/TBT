
#include <TBT/TBT>

template <class TaskTypes_>
struct States {
  using TaskTypes = TaskTypes_;
};  // States

struct TaskA {};
#define TASK_TYPE TaskA
#include <TBT/magic.hpp>

template <class TaskTypes>
TBT::State run(TaskA& _task, States<TaskTypes>& _states) {
  return TBT::State::SUCCESS;
}  // run

struct TaskB {};
#define TASK_TYPE TaskB
#include <TBT/magic.hpp>

template <class TaskTypes>
TBT::State run(TaskB& _task, States<TaskTypes>& _states) {
  return TBT::State::SUCCESS;
}  // run

template <class TaskTypes>
TBT::State wait(TaskB& _task, States<TaskTypes>& _states) {
  return TBT::State::SUCCESS;
}  // wait

struct TaskC {};
#define TASK_TYPE TaskC
#include <TBT/magic.hpp>

template <class TaskTypes>
TBT::State run(TaskC& _task, States<TaskTypes>& _states) {
  return TBT::State::SUCCESS;
}  // run

namespace TBT {
  using TaskTypes          = std::variant<TASK_TYPES>;
  constexpr size_t tt_size = sizeof(TaskTypes);
  using StateProvider      = States<TaskTypes>;
}  // namespace TBT

TEST_CASE("Factorials are computed", "[factorial]") {}
