# TBT — Functional Task-Based Trees

**TBT** is a **header-only**, **purely functional**, **compile-time parsed** behavior tree / hierarchical task framework for modern C++.

No inheritance. No virtual calls.  
Just clean, declarative, human-readable tree definitions — fully resolved and validated at **compile time**.

Perfect for games, robotics, animation systems, tools, or any domain that benefits from composable, data-oriented logic.

## Requirements
C++23, glaze for compile time reflection and a working preprocessor for the macro.

## Why TBT is Different

| Feature                        | TBT                          | Traditional OOP BT (UE5, etc.) | ECS + Scripted Nodes |
|-------------------------------|------------------------------|--------------------------------|----------------------|
| Zero virtual dispatch         | Yes                          | No                             | Sometimes            |
| Compile-time tree parsing     | Yes                          | No                             | No                   |
| Human-readable string syntax  | Yes                          | Limited                        | No                   |
| Purely functional tasks       | Yes                          | No                             | Possible             |
| Header-only                   | Yes                          | Varies                         | Usually              |
| Parameterized subtrees        | Yes (via `$0`, `$1`, ...)    | Manual / limited               | Manual               |
| Nested tree spawning          | Yes (from inside tasks)      | Yes                            | Yes                  |

## Features

- Fully static & functional task composition
- Compile-time tree validation and construction
- Human-readable tree definition syntax
- No inheritance, no pointers, no runtime type erasure (except one `std::variant`)
- Easy integration into existing codebases
- Supports nested trees, parameters, and subtrees
- Designed for games, robotics, AI, or any hierarchical task system

## Minimal Example

```cpp
// main.cpp
#include <TBT/TBT>  // Single header include

// Define your tasks anywhere in your .cpp files
struct TaskA {
    int32_t value = 0;
};
#define TASK_TYPE TaskA
#include <TBT/magic.hpp>  // Registers TaskA with the system

// Optional: implement lifecycle hooks
template <class States>
TBT::State init(const TaskA& t, States& s) {
    // Called once when task starts
    return TBT::SUCCESS;
}

template <class States>
TBT::State run(const TaskA& t, States& s) {
    // Called every tick while active
    printf("TaskA running with value = %d\n", t.value);
    return TBT::SUCCESS;
}

template <class States>
void exit(const TaskA& t, States& s) {
    // Called on task completion/failure/interruption
}

// Another task
struct TaskB {
    int32_t count = 1;
};
#define TASK_TYPE TaskB
#include <TBT/magic.hpp>

template <class States>
TBT::State run(const TaskB& t, States& s) {
    printf("TaskB repeating %d times\n", t.count);
    return t.count-- > 0 ? TBT::RUNNING : TBT::SUCCESS;
}

// Composing and Running a Tree

// This must appear before main() — collects all registered task types
using Variant = std::variant<TASK_TYPES>;  // Expands to variant<TaskA, TaskB, ...>

// Global state container (passed to all tasks)
template<class V>
struct StateProvider {
    using Variant = V;
    TaskQueue task_queue;  // Required for dynamic tree spawning
};

// In main()
int main() {
    StateProvider<Variant> states;

    // Compile-time parsed tree with parameters and nesting!
    auto [promise, tree] = COMPILE_AND_QUEUE(
        "TaskA, "
        "TaskA($0)[TaskB(5)[TaskA, TaskB]] "  // $0 = -5 → TaskA(-5)
        "TaskA[TaskB($1)]",                   // $1 = 99 → TaskB(99)
        states,
        FULL_1,   // execution mode
        -5,       // First argument: passed as $0
        99        // Second argument: passed as $1
    );

    // Main loop
    while (true) {
        EXECUTE_QUEUE(states);  // Processes all active trees

        // Break when root tree finishes
        if (promise.ready()) {
            std::cout << "Tree completed with: " 
                      << (promise.result() == TBT::SUCCESS ? "SUCCESS" : "FAILURE")
                      << std::endl;
            break;
        }

        // Your game/frame update here...
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
```
