# TBT

> [!WARNING]
> This library is far from stable and very much in constant development. The documentation is still very incomplete and features, the api or naming can change without warning.


**TBT** is a **header-only**, **purely functional**, **compile-time parsed** hierarchical task framework for modern C++.

No inheritance. No virtual calls.  
Just clean, declarative, human-readable tree definitions — fully resolved and validated at **compile time**.

Perfect for games, robotics, animation systems, tools, or any domain that benefits from composable, data-oriented logic.


## Features
- Fully static & functional task composition
- Compile-time tree validation and construction
- Human-readable tree definition syntax
- No inheritance, no pointers, no oop
- Easy integration into existing codebases
- Supports nested trees, parameters, and subtrees
- Full support for coroutines
- Excells at async task dispatches.
- Designed for games, robotics, AI, or any hierarchical task system

## Why TBT is Different

| Feature                        | TBT                          | Traditional OOP BT (UE5, etc.) | ECS + Scripted Nodes |
|-------------------------------|------------------------------|--------------------------------|----------------------|
| Zero virtual dispatch         | Yes                          | No                             | Sometimes            |
| Compile-time tree parsing     | Yes                          | No                             | No                   |
| Human-readable string syntax  | Yes                          | Limited                        | No                   |
| Purely functional tasks       | Yes                          | No                             | Possible             |
| Header-only                   | Yes                          | Varies                         | Usually              |
| Full Coroutine support        | Yes                          | Possible                       | Possible             |
| Parameterized subtrees        | Yes (via `$0`, `$1`, ...)    | Manual / limited               | Manual               |
| Nested tree spawning          | Yes (from inside tasks)      | Yes                            | Yes                  |

## Terminology

- **Task**: A small, self-contained unit of functionality. Instead of writing one large monolithic program, the logic can be split into multiple focused tasks - each responsible for a single, well-defined purpose (monofunctional).
- **Tree**: A collection of tasks organized and composed via a human-readable script.

## Lifetime of a Task

The lifetime of a task is in most cases over the span of multiple frames and it works as follows: The Task is executed every frame as long it returns `TBT::State::BUSY`. When it returns `TBT::State::SUCCESS` it will continue to either its children (if it has any) or to its parent. The Tree is traversed in a depth-first fashion. However, if the Task returns `TBT::State::FAILED` it will directly return to its parent and skip any children it might have.

A tree is considered finished when the last child returns `SUCCESS` or `FAILED`.

### Legacy Version

A task follows the classic lifecycle of a C++ object: **initialization → repeated execution → teardown**.  
All phases are optional and discovered via **Argument-Dependent Lookup (ADL)**. The task type itself serves as the unique identifier for the associated functions.

The framework supports the following ADL-discovered callables (normal ADL rules apply):

| Function signature                                            | Return type               | When called                                                                 |
|---------------------------------------------------------------|---------------------------|-----------------------------------------------------------------------------|
| `TBT::State init((const) Task&, const StateProvider&)`          | `SUCCESS` / `FAILED`      | Once, during task construction                                              |
| `TBT::State init((const) Task&)`                                | `SUCCESS` / `FAILED`      | Once, during task construction (fallback if the version with `StateProvider` is missing) |
| `TBT::State run((const) Task&, (const) StateProvider&)`           | `SUCCESS` / `FAILED` / `BUSY` | 1 to n times after `init`                                                       |
| `TBT::State run((const) Task&)`                                 | `SUCCESS` / `FAILED` / `BUSY` | 1 to n times after `init` (fallback)                                            |
| `void exit((const) Task&, (const) StateProvider&)`                | `void`                    | Once, when `run` no longer returns `BUSY`                                   |
| `void exit((const) Task&)`                                      | `void`                    | Once, when `run` no longer returns `BUSY` (fallback)                        |

### Coroutines

The preferred way to compose tasks are using coroutines. The legacy version might still be prefferable for performance reasons. Coroutines come with a slight overhead.

Nevertheless, coroutines allow to write very clear, linear code. See the examples for use cases.

| Function signature                                            | Return type               | When called                                                                 |
|---------------------------------------------------------------|---------------------------|-----------------------------------------------------------------------------|
| `CoState co_run((const) Task&, const StateProvider&)`          | `SUCCESS` / `FAILED`      | Replaces run and init. Arbitrary interrupts can be achieved with co_yield.                                              |
| `CoState co_run((const) Task&)`                                | `SUCCESS` / `FAILED`      |  Replaces run and init. Arbitrary interrupts can be achieved with co_yield. (fallback if the version with `StateProvider` is missing) | |
| `void exit((const) Task&, (const) StateProvider&)`                | `void`                    | Once, when `co_run` returns co_return                                   |
| `void exit((const) Task&)`                                      | `void`                    | Once, when `co_run` returns co_return (fallback)                        |


## Macro Magic (Task Registration)

A core feature of the framework is the automatic collection of all task types into a comma-separated list inside a `std::variant`. True reflection will make this trivial in C++26, but until then we rely on black macro magic.

> [!IMPORTANT]  
> A detailed explanation of the macro's inner workings is beyond the scope of this README.  
> The technique is thoroughly documented in [Jackson Allan's excellent article](https://github.com/JacksonAllan/CC/blob/main/articles/Better_C_Generics_Part_1_The_Extendible_Generic.md).  
>   
> This entire library exists only because of Jackson's work on extensible generics in C. Huge thanks to [Jackson Allan](https://github.com/JacksonAllan) — this library would not exist without him. (At least until C++26...)

**Important:** Follow the exact pattern below — the include is mandatory.

```cpp
struct MyTask { };
#define TASK_TYPE MyTask      // tells the magic which type we are registering
#include <TBT/magic.hpp>      // must be included exactly here for every task
```
The macro + include combination generates the necessary boilerplate so that MyTask appears in the global task variant. See the minimal implementation for a reference use-case. 

## Requirements

C++23 (required language features)

The glaze library is used for reflection/metadata handling
If glaze is not already present, CMake will automatically fetch it with fetchcontent.

No other external dependencies are required.

## Minimal Example

```cpp
// main.cpp
#include <TBT/TBT>  // Single header include

// ----------------------------------------------------------
// Legacy Definitions
// ----------------------------------------------------------

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

// ----------------------------------------------------------
// Coroutine Definitions
// ----------------------------------------------------------

// Another task
// empty tasks are just fine
struct TaskB {};
#define TASK_TYPE TaskB
#include <TBT/magic.hpp>

template <class States>
CoState co_run(const TaskB& t, States& s) {
    
    // anything can be made an awaitable using the Awaitable struct
    std::shared_ptr img = co_await load_some_img(...)

    // we can co_await trees very easily
    // this statically compiles the tree, will run once every frame until completion
    // and takes img as dynamic parameter.
    State res = co_await TBT_RUN("PostProcessImageTask($0)", s, SINGLE_1, img);

    // for example we draw this image every frame

    while(...){
        draw(img);
        //it doesnt matter what is returned with co_yield
        // it can only be BUSY
        co_yield 0;
    }

    // when the task is done
    co_return SUCCESS;

}

//exit can be used with coroutines, but in this case it is not needed

//template <class States>
//void exit(const TaskB& t, States& s) {
    // Called on task completion/failure/interruption
//}

// ----------------------------------------------------------
// ----------------------------------------------------------

// This must appear before main() — collects all registered task types
// make sure that #include <TBT/magic.hpp> is included at least once in the .cpp file
using Variant = std::variant<TASK_TYPES>;  // Expands to variant<TaskA, TaskB, ...>

// Global state container (passed to all tasks)
// the provided taskqueue is allocator aware
// there are plans to make the rest as well.
template<class V>
struct StateProvider {
    using Variant = V;
    TaskQueue<> task_queue;  // Required for dynamic tree spawning
};

int main() {
    StateProvider<Variant> states;

    // Compile-time parsed tree with parameters and nesting!
    // the awaitable gives access to an iterator, and the future.
    // mostly unused and just discarded
    auto awaitable = TBT_RUN(
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
        TBT_EXECUTE_QUEUE(states);  // Processes all active trees

        // Your game/frame update here...
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
```
## Data-flow from and into tasks
A common question is on how to retrieve data from a task without abusing the global state as catch-all blackboard. Following two ways how this can be achieved.

### Event Queue
Using an event driven approach is always a good idea.
```cpp
template<class Variant>
TBT::State run(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    // when the file is loaded we dispatch it over an event queue
    if(_task.async,wait_for(std::chrono::seconds(0)) == std::future_status::ready){
        _state.events_.dispatch(FILE_LOADED_EVENT, _task.async.get());
        return TBT::SUCCESS;
    }
    return TBT::BUSY;
}
```
### Shared State
For longer running tasks or when tasks should share a common state a shared state is an excellent choice.
```cpp
//define your state
std::shared_ptr<TaskState> ptr = std::make_shared...
//hand the state to all tasks
TBT_RUN_FULL_INF(0, "Some($0), Example($0), Tree($0)", state_provider, ptr);
```
## Terminating a task/ tree
When queueing a new task the macro returns a pair containing a std::future\<TBT::State> and a iterator to the item in the queue. ~~The iterator can be used to erase the element~~. Important: Don't erase the element while it is executing, this leads to memory leaks.

```cpp
    const auto&[future, tree_iterator] = TBT_RUN_FULL_INF(0, "Some, Example, Tree", state_provider);
```

Instead terminate the task from inside with return FAILED or wait for termination of the tree.

## Assorted use-case examples
For all the following examples it is assumed the framework is set up properly. All the trees are assumed to be static. An dynamic implementation would work along the same line.
### Executing a full tree every frame
This will run the tree every frame until SUCCESS or FAILED is returned. Be weary of returning BUSY in a task. It might lead to being stuck.
```cpp
TBT_RUN_FULL_INF(0, "Some, Example, Tree", state_provider);
```
### Executing a tree node by node a single time
This will call the tree once every frame until SUCCESS or FAILED is returned. In this case it is perfectly fine to return BUSY.
```cpp
TBT_RUN_STEPWISE_1(0, "Some, Example, Tree", state_provider);
```
### Async task using an executor
This example shows how convenient it to async load a file from disk using Imgui. For example purposes it uses a taskflow executor.
#### Legacy version
```cpp
struct File; //some user defined implementation
File load_from_disk(const std::filesystem::path& _p); // user defined loading function

//---------------------------------------------------------------

template<class Variant>
struct StateProvider {
    //...
    //the executor
    tf::Executor executor {8};
};

struct LoadFileFromDiskTask {
    std::filesystem::path path_to_file;
    std:::future<File> async;
};
#define TASK_TYPE LoadFileFromDiskTask
#include <TBT/magic.hpp>

//---------------------------------------------------------------

template<class Variant>
TBT::State init(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    _task.async = _state.executor.async([&]() {
        // there is no danger accessing the task state. 
        // only the corresponding functions will ever read/ write to it.
        // in this case only this lambda will read from it. 
        return load_from_disk(_task.path_to_file);
    });
    return TBT::SUCCESS;
}

template<class Variant>
TBT::State run(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    //check if the loading has finished
    if(_task.async,wait_for(std::chrono::seconds(0)) == std::future_status::ready){
        auto file = _task.async.get();
        //do something with the file.
        return TBT::SUCCESS;
    }
    return TBT::BUSY;
}

//---------------------------------------------------------------

//somewhere in your gui code
if(Imgui::Button("Load File")){
    //path from file. for example from file picker.
    const std::filesystem::path = ...;
    //move here to avoid a copy
    TBT_RUN_STEPWISE_1(0, "LoadFileFromDiskTask($0)", state_provider, std::move(path));
}
```

#### Coroutine version
```cpp
struct File; //some user defined implementation
Awaitable<File> load_from_disk(const std::filesystem::path& _p); // user defined loading function

//---------------------------------------------------------------

template<class Variant>
struct StateProvider {
    //...
    //the executor
    tf::Executor executor {8};
};

struct LoadFileFromDiskTask {
    std::filesystem::path path_to_file;
    std:::future<File> async;
};
#define TASK_TYPE LoadFileFromDiskTask
#include <TBT/magic.hpp>

//---------------------------------------------------------------

template<class Variant>
CoState co_run(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    
    // the awaitable needs to call the executor on its own. this might need some boilerplate
    auto file = co_await load_from_disk(_task.path_to_file);

    //do something with the file

    co_return SUCCESS;

}

//---------------------------------------------------------------

//somewhere in your gui code
if(Imgui::Button("Load File")){
    //path from file. for example from file picker.
    const std::filesystem::path = ...;
    //move here to avoid a copy
    TBT_RUN_STEPWISE_1(0, "LoadFileFromDiskTask($0)", state_provider, std::move(path));
}
```

### Waiting for a tree from a tree
Executing a new tree from a tree is very simple. In fact you can execute any tree from anywhere as long you have a reference to the global states. Let's say we want to run postprocessing async on the file we just loaded and wait on it. Finally, we send it back to the caller.
#### Legacy Version
```cpp
template<class Variant>
TBT::State run(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    //check if the loading has finished
    if(!_task.file_loaded_){
        if(_task.async.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
            //this is a good usecase for a shared state to make data transfer easier.
            _task.shared_state_->file_ = _task.async.get();
            _task.wait_for_pp_ = TBT_RUN_STEPWISE_1(0, "PostProcessFile($0)", _state, _task.shared_state_);
            _task.file_loaded_ = true;
            return TBT::BUSY;
        }
    } else {
        //wait for the new tree to finish
        if(_task.wait_for_pp_.wait_for(std::chrono::seconds(0)) == std::future_status::ready){
            // send the finished file back to caller
            _state.events_.dispatch(FILE_LOADED_EVENT, std::move(_task.shared_state_->file_));
            return TBT::SUCCESS;
        }
    }
    return TBT::BUSY;
}
```

#### Coroutine Version
```cpp

template<class Variant>
CoState co_run(LoadFileFromDiskTask& _task, StateProvider<Variant>& _state){
    
    // the awaitable needs to call the executor on its own. this might need some boilerplate
    auto file = co_await load_from_disk(_task.path_to_file);

    co_await TBT_RUN_STEPWISE_1(0, "PostProcessFile($0)", _state, _task.shared_state_);

    // send the finished file back to caller
    _state.events_.dispatch(FILE_LOADED_EVENT, std::move(file));

    co_return SUCCESS;

}
```