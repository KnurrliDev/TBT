#pragma once

#include <TBT/compiler.hpp>

namespace TBT {
  template <class T, class Allocator>
  struct TreeAwaitable;

  namespace Execute {
    struct CoStateValues;
  }

  template <class T>
  struct Awaitable {
    T val_;
    std::shared_ptr<Execute::CoStateValues> values_;
    std::function<T()> func_;

    Awaitable() = default;

    bool await_ready() noexcept { return false; }

    template <class Handle>
    void await_suspend(const Handle&) {
      init_awaitable([v = values_]() {
        assert(v);
        v->a_done_ = true;
      });
    }

    T await_resume() noexcept { return val_; }

    virtual void init_awaitable(std::function<void()> _set_done) = 0;

  };  // Awaitable
}  // namespace TBT

namespace TBT::Execute {

  enum CoStateState { YIELD, RETURN, AWAIT };

  template <class Awaitable>
  struct CoStateAwaitable;

  struct CoStateValues {
    State val_;
    CoStateState state_;
    std::exception_ptr exception_;
    bool a_done_ = false;
  };  // CoStateValues

  struct CoState {
    struct promise_type {
      std::shared_ptr<CoStateValues> values_;

      CoState get_return_object() {
        values_ = std::make_shared<CoStateValues>();
        return CoState(values_, std::coroutine_handle<CoState::promise_type>::from_promise(*this));
      }

      std::suspend_never initial_suspend() noexcept { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }

      template <class T>
      std::suspend_always yield_value(T&&) {
        values_->state_ = CoStateState::YIELD;
        values_->val_   = BUSY;
        return {};
      }

      void return_value(const State _val) noexcept {
        values_->state_ = CoStateState::RETURN;
        values_->val_   = _val;
      }

      void unhandled_exception() noexcept { values_->exception_ = std::current_exception(); }

      template <class T>
      CoStateAwaitable<Awaitable<T>> await_transform(Awaitable<T>&& _a) {
        values_->state_ = CoStateState::AWAIT;
        CoStateAwaitable<Awaitable<T>> out(std::move(_a));
        out.values_ = values_;
        return out;
      };

      template <class T, class Allocator>
      CoStateAwaitable<TreeAwaitable<T, Allocator>> await_transform(TreeAwaitable<T, Allocator>&& _a) {
        values_->state_  = CoStateState::AWAIT;
        _a.ref_->values_ = values_;
        CoStateAwaitable<TreeAwaitable<T, Allocator>> out(std::move(_a));
        out.values_ = values_;
        return out;
      };

    };  // promise_type

    std::shared_ptr<CoStateValues> values_;
    std::coroutine_handle<CoState::promise_type> handle_;

    CoState() = default;
    explicit CoState(std::shared_ptr<CoStateValues> _values, std::coroutine_handle<CoState::promise_type> _handle)
        : values_(std::move(_values)), handle_(std::move(_handle)) {}

    State get_value() { return values_->val_; }
    CoStateState get_costate() { return values_->state_; }
    bool is_awaitable_done() { return values_->a_done_; }

  };  // CoState

  template <class Awaitable>
  struct CoStateAwaitable {
    Awaitable other_;
    std::shared_ptr<CoStateValues> values_;
    bool await_ready() noexcept { return false; }
    void await_suspend(const std::coroutine_handle<CoState::promise_type>& _h) { other_.await_suspend(_h); }
    void await_resume() noexcept {}
    explicit CoStateAwaitable(Awaitable&& _other) : other_(std::forward<Awaitable>(_other)) {}
  };

  namespace Concepts {

    //-----------------------------------
    // concepts for all valid init signatures

    template <class TaskDerived, class StateProvider>
    concept has_init_sig_1 = requires(TaskDerived task, StateProvider state) {
      { init(task, state) } -> std::same_as<State>;
    };

    template <class TaskDerived>
    concept has_init_sig_2 = requires(TaskDerived task) {
      { init(task) } -> std::same_as<State>;
    };

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

    //-----------------------------------
    // concepts for all valid coroutines signatures

    template <class TaskDerived, class StateProvider>
    concept has_corun_sig_1 = requires(TaskDerived task, StateProvider state) {
      { co_run(task, state) } -> std::same_as<CoState>;
    };

    template <class TaskDerived>
    concept has_corun_sig_2 = requires(TaskDerived task) {
      { co_run(task) } -> std::same_as<CoState>;
    };

    template <class TaskDerived, class StateProvider>
    concept is_corun = has_corun_sig_1<TaskDerived, StateProvider> || has_corun_sig_2<TaskDerived>;

    template <class Variant, class StateProvider>
    consteval auto corun_mask_for() {
      using V                 = std::decay_t<Variant>;

      constexpr std::size_t N = std::variant_size_v<V>;

      std::array<bool, N> result{};

      auto fill = []<std::size_t... I>(std::index_sequence<I...>) constexpr {
        return std::array<bool, sizeof...(I)>{is_corun<std::variant_alternative_t<I, V>, StateProvider>...};
      };

      result = fill(std::make_index_sequence<N>{});
      return result;
    }  // corun_mask_for

    //-----------------------------------
    // concepts for all valid exit signatures

    template <class TaskDerived, class StateProvider>
    concept has_exit_sig_1 = requires(TaskDerived task, StateProvider state) {
      { exit(task, state) };
    };

    template <class TaskDerived>
    concept has_exit_sig_2 = requires(TaskDerived task) {
      { exit(task) };
    };

  }  // namespace Concepts

  template <typename... Ts>
  Parameter tuple_element_to_variant(const std::tuple<Ts...>& tuple, size_t _index) {
    Parameter result;
    std::apply(
        [&](const auto&... elements) {
          size_t i = 0;
          ((i++ == _index ? result = Parameter(elements) : result), ...);
        },
        tuple);
    return result;
  }  // tuple_element_to_variant

  template <class Task, class... Ts>
  [[nodiscard]] Task construct_task(const std::vector<uint32_t>& _idxs,
                                    const std::vector<std::variant<bool, int32_t, float, uint32_t>>& _pl,
                                    const std::tuple<Ts...>& _params) {
    static_assert(std::is_class_v<Task>, "Task must be a class/struct");

    // this creates a copy of all params. maybe not the best way
    const auto& args         = _params;
    constexpr auto args_size = std::tuple_size_v<std::decay_t<decltype(_params)>>;
    constexpr auto N         = glz::reflect<Task>::size;

    Task out;

    // default constructed
    if (args_size + _pl.size() == 0) return out;

    // if not default constructed the params will be matched in order
    if constexpr (N > 0) {
      auto tie = glz::to_tie(out);

      for (size_t i = 0; i < N; ++i) {
        // more members than payloads
        if (i >= _idxs.size()) break;

        const auto field = glz::detail::get_runtime(tie, i);

        // dynamic payload
        if (_idxs[i] >= _pl.size()) {
          if constexpr (args_size > 0) {
            const auto val = tuple_element_to_variant(args, _idxs[i] - _pl.size());

            std::visit(
                [&](auto p, auto& v) {
                  using ValueType   = std::decay_t<decltype(v)>;
                  using PointerType = std::remove_pointer_t<std::decay_t<decltype(p)>>;
                  if constexpr (std::is_same_v<PointerType, ValueType>) *p = std::move(v);
                },
                field, val);
          }
        }

        // static payload
        else {
          std::visit(
              [&](auto ptr) {
                if constexpr (std::is_same_v<bool, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                  if (_pl[_idxs[i]].index() == 0) *ptr = std::get<0>(_pl[_idxs[i]]);
                } else if constexpr (std::is_same_v<int32_t, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                  if (_pl[_idxs[i]].index() == 1) *ptr = std::get<1>(_pl[_idxs[i]]);
                } else if constexpr (std::is_same_v<float, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                  if (_pl[_idxs[i]].index() == 2) *ptr = std::get<2>(_pl[_idxs[i]]);
                } else {
                  // only bool, float and signed ints are allowed as static payload
                  std::unreachable();
                }
              },
              field);
        }
      }
    }

    return out;
  }  // construct_task

  template <class Variant, class... Ts>
  [[nodiscard]] Variant* alloc_task(uint32_t _idx, const std::vector<uint32_t>& _idxs,
                                    const std::vector<std::variant<bool, int32_t, float, uint32_t>>& _pl,
                                    const std::tuple<Ts...>& _params) {
    constexpr size_t variant_size = std::variant_size_v<Variant>;
    assert(_idx < variant_size);
    Variant* v            = new Variant();

    auto construct_params = [&]<size_t... Is>(std::index_sequence<Is...>) {
      ((Is == _idx ? ((*v = construct_task<std::variant_alternative_t<Is, Variant>>(_idxs, _pl, _params)), true)
                   : false),
       ...);
    };

    auto construct = [v, _idx]<size_t... Is>(std::index_sequence<Is...>) {
      ((Is == _idx ? (v->template emplace<Is>(), true) : false), ...);
    };

    if (_idxs.empty())
      construct(std::make_index_sequence<variant_size>{});
    else
      construct_params(std::make_index_sequence<variant_size>{});

    return v;
  }  // alloc_task

  template <class Variant, class StateProvider, class... Ts>
  State execute_task(std::span<uint8_t> _node, Compiler::Header& _global_header, const Compiler::NodeHeader& _header,
                     StateProvider& _states, const std::tuple<Ts...>& _params) {
    using namespace Compiler;

    Composite task = read_composite(_header, _node);

    // first time entering the task
    if (_global_header.last_result_.dir_ == DOWN) {
      // create mapping
      std::vector<uint32_t> idcs;

      std::vector<std::variant<bool, int32_t, float, uint32_t>> payloads;

      std::vector<int32_t> d_ics;

      uint32_t s_pl = 0;
      for (int32_t i = 0; i < _header.params_count_; ++i) {
        const auto pl = read_payload(i, _header, _node);

        switch (pl.index()) {
          case 3: {
            idcs.push_back(std::get<3>(pl));  // idx into dynamic params
            d_ics.push_back(i);
          } break;
          default:
            payloads.push_back(std::move(pl));
            idcs.push_back(s_pl++);  // idx into static params
            break;
        }
      }

      // add offset to dynamic params
      for (const int32_t i : d_ics) idcs[i] += (uint32_t)payloads.size();

      constexpr auto co_mask = Concepts::corun_mask_for<Variant, std::decay_t<StateProvider>>();

      Variant* state         = alloc_task<Variant>(_header.type_idx_, idcs, payloads, _params);

      const bool is_co       = std::visit(
          [&](auto& _t) {
            if constexpr (Concepts::is_corun<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
              return true;
            else
              return false;
          },
          *state);

      // a coroutine
      if (is_co) {
        // start the coroutine
        CoState cstate;
        std::visit(
            [&](auto& _t) {
              if constexpr (Concepts::has_corun_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>) {
                cstate = co_run(_t, _states);
              } else if constexpr (Concepts::has_corun_sig_2<std::decay_t<decltype(_t)>>) {
                cstate = co_run(_t);
              }
            },
            *state);

        // check the state of the coroutine
        const CoStateState res = cstate.get_costate();

        switch (res) {
          case YIELD: {
            // the tasks wants to wait. write states and return.
            const State value = cstate.get_value();
            assert(value == BUSY);
            task.ptr_                          = reinterpret_cast<intptr_t>(state);
            task.co_                           = reinterpret_cast<intptr_t>(cstate.handle_.address());
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = value;
            write_composite(task, _header, _node);
            return BUSY;
          }
          case RETURN: {  // the coroutine has co_returned
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);

            delete state;
            task.ptr_         = 0;
            task.co_          = 0;
            // cres.coro_.destroy();
            const State value = cstate.get_value();
            // return to parent if no children or last child or failed
            if (value == FAILED || task.cur_idx_ >= _header.children_count_) {
              task.cur_idx_                    = 0;
              _global_header.ptr_              = _header.parent_;
              _global_header.last_result_.dir_ = UP;
            } else {
              const uint32_t optr              = read_child(task.cur_idx_, _node);
              _global_header.last_result_.dir_ = DOWN;
              task.cur_idx_++;
              _global_header.ptr_ = optr;
            }
            write_composite(task, _header, _node);
            _global_header.last_result_.state_ = value;
            return BUSY;
          }
          case AWAIT: {  // task will wait until the awaitable has finished
            task.ptr_                          = reinterpret_cast<intptr_t>(state);
            task.co_                           = reinterpret_cast<intptr_t>(cstate.handle_.address());
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = BUSY;
            write_composite(task, _header, _node);
            return BUSY;
          }
        }
      }

      // not a coroutine
      else {
        // init the task
        {
          State res;
          std::visit(
              [&](auto& _t) {
                if constexpr (Concepts::has_init_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  res = init(_t, _states);
                else if constexpr (Concepts::has_init_sig_2<std::decay_t<decltype(_t)>>)
                  res = init(_t);
              },
              *state);

          // the task failed. return to parent
          if (res == FAILED) {
            // call exit if present
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);

            delete state;
            task.ptr_ = 0;
            task.co_  = 0;
            write_composite(task, _header, _node);

            _global_header.ptr_                = _header.parent_;
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = res;
            return BUSY;
          }
        }

        // the task succeeded. check if wait exists, else return
        {
          std::optional<State> res = std::visit(
              [&](auto& _t) -> std::optional<State> {
                if constexpr (Concepts::has_run_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  return run(_t, _states);
                else if constexpr (Concepts::has_run_sig_2<std::decay_t<decltype(_t)>>)
                  return run(_t);
                else
                  return std::nullopt;
              },
              *state);

          // no wait signature found and run succeed: task is done
          // if the task is not busy return to parent or next child
          if (!res || (res && (*res == FAILED || *res == SUCCESS))) {
            // call exit if present
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);

            delete state;
            task.ptr_ = 0;
            task.co_  = 0;

            // return to parent if no children or last child
            if (*res == FAILED || task.cur_idx_ >= _header.children_count_) {
              task.cur_idx_                    = 0;
              _global_header.ptr_              = _header.parent_;
              _global_header.last_result_.dir_ = UP;
            } else {
              const uint32_t optr              = read_child(task.cur_idx_, _node);
              _global_header.last_result_.dir_ = DOWN;
              task.cur_idx_++;
              _global_header.ptr_ = optr;
            }
            write_composite(task, _header, _node);

            // either wait has returned a state or the state must be success from run
            _global_header.last_result_.state_ = res ? *res : SUCCESS;
            return BUSY;
          }
          // the tasks wants to wait. write states and return.
          task.ptr_                          = reinterpret_cast<intptr_t>(state);
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = *res;
          write_composite(task, _header, _node);
          return BUSY;
        }
      }
    }

    //------------------------------------------------------------

    // re enter. either the task returned BUSY or returning from a child or coyielded or coawaited
    else {
      // returning from a child. go to next or return to parent
      if (task.ptr_ == 0) {
        if (_global_header.last_result_.state_ != State::BUSY) {
          if (task.cur_idx_ >= _header.children_count_) {
            task.cur_idx_                    = 0;
            _global_header.ptr_              = _header.parent_;
            _global_header.last_result_.dir_ = UP;
          } else {
            const uint32_t optr              = read_child(task.cur_idx_, _node);
            _global_header.last_result_.dir_ = DOWN;
            task.cur_idx_++;
            write_composite(task, _header, _node);
            _global_header.ptr_ = optr;
          }
          return _global_header.last_result_.state_;
        }
      }

      // keep running the task
      assert(task.ptr_ != 0);
      Variant* state   = reinterpret_cast<Variant*>(task.ptr_);

      const bool is_co = std::visit(
          [&](auto& _t) {
            if constexpr (Concepts::is_corun<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
              return true;
            else
              return false;
          },
          *state);

      // a coroutine
      if (is_co) {
        assert(task.co_ != 0);

        auto co_handle = std::coroutine_handle<CoState::promise_type>::from_address(reinterpret_cast<void*>(task.co_));
        CoState co_state(co_handle.promise().values_, co_handle);

        // check last costate
        const CoStateState res_prev = co_state.get_costate();
        assert(res_prev != RETURN);

        switch (res_prev) {
          case YIELD:  // coninue execution
          {
            co_handle.resume();
            break;
          }
          case AWAIT:  // continue if the awaitable has finished
          {
            const bool a_done = co_state.is_awaitable_done();
            if (a_done) co_handle.resume();
            break;
          }
        }

        // check the coroutine after resuming
        const CoStateState res_after = co_state.get_costate();

        switch (res_after) {
          case YIELD: {
            // the tasks wants to wait. write states and return.
            const State value = co_state.get_value();
            assert(value == BUSY);
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = value;
            return BUSY;
          }
          case RETURN: {  // the coroutine has co_returned
            std::visit(
                [&](auto& _t) {
                  if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                    exit(_t, _states);
                  else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                    exit(_t);
                },
                *state);
            delete state;
            task.ptr_         = 0;
            task.co_          = 0;
            // cres.coro_.destroy();
            const State value = co_state.get_value();
            // return to parent if no children or last child or failed
            if (value == FAILED || task.cur_idx_ >= _header.children_count_) {
              task.cur_idx_                    = 0;
              _global_header.ptr_              = _header.parent_;
              _global_header.last_result_.dir_ = UP;
            } else {
              const uint32_t optr              = read_child(task.cur_idx_, _node);
              _global_header.last_result_.dir_ = DOWN;
              task.cur_idx_++;
              _global_header.ptr_ = optr;
            }
            write_composite(task, _header, _node);
            _global_header.last_result_.state_ = value;
            return BUSY;
          }
          case AWAIT: {  // task will wait until the awaitable has finished
            _global_header.last_result_.dir_   = UP;
            _global_header.last_result_.state_ = BUSY;
            return BUSY;
          }
        }
      }

      // not a coroutin
      else {
        State res;
        std::visit(
            [&](auto& _t) {
              if constexpr (Concepts::has_run_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                res = run(_t, _states);
              else if constexpr (Concepts::has_run_sig_2<std::decay_t<decltype(_t)>>)
                res = run(_t);
            },
            *state);

        // task is finished
        if (res == FAILED || res == SUCCESS) {
          // call exit if present
          std::visit(
              [&](auto& _t) {
                if constexpr (Concepts::has_exit_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                  exit(_t, _states);
                else if constexpr (Concepts::has_exit_sig_2<std::decay_t<decltype(_t)>>)
                  exit(_t);
              },
              *state);

          delete state;
          task.ptr_ = 0;
          task.co_  = 0;

          // return to parent if no children or last child
          if (res == FAILED || task.cur_idx_ >= _header.children_count_) {
            task.cur_idx_                    = 0;
            _global_header.ptr_              = _header.parent_;
            _global_header.last_result_.dir_ = UP;
          } else {
            const uint32_t optr              = read_child(task.cur_idx_, _node);
            _global_header.last_result_.dir_ = DOWN;
            task.cur_idx_++;
            _global_header.ptr_ = optr;
          }

          write_composite(task, _header, _node);
          // either wait has returned a state or the state must be success from run
          _global_header.last_result_.state_ = res;
          return BUSY;
        }

        // wait longer
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = res;
        return BUSY;
      }
    }
    // todo: a path is missing
    return BUSY;
  }  // execute_task

  template <class Variant, class Tree, class StateProvider, class... Ts>
  State execute_step(Tree& _tree, StateProvider&& _states, const std::tuple<Ts...>& _params) {
    // static_assert(std::is_same_v<Tree, DynamicTree> || std::is_same_v<Tree, StaticTree>);

    using namespace Compiler;

    // read global header
    Header global_header = read_global_node_header({_tree.begin(), _tree.end()});

    // if first entry. if yes set the pointer to the first child and reset index
    const bool first_entry =
        global_header.ptr_ < global_header.first_node_offset_ && global_header.last_result_.dir_ == DOWN;
    if (first_entry) {
      global_header.ptr_       = global_header.first_node_offset_;
      global_header.child_idx_ = 0;
    }

    // read header of current node
    const NodeHeader cur_node_header = read_node_header({_tree.begin() + global_header.ptr_, _tree.end()});

    assert(cur_node_header.type_idx_ >= 0 || cur_node_header.type_idx_ < (int16_t)std::variant_size_v<Variant>);
    execute_task<Variant>({_tree.begin() + global_header.ptr_, cur_node_header.node_size_}, global_header,
                          cur_node_header, _states, _params);

    // check if returned to root
    if (global_header.last_result_.dir_ == UP && global_header.ptr_ == 0) {
      global_header.child_idx_++;

      // the last task was executed. the tree is done
      if (global_header.child_idx_ >= global_header.children_count_) {
        // reset the tree
        global_header.child_idx_        = 0;
        global_header.ptr_              = 0;
        global_header.last_result_.dir_ = DOWN;
        write_global_node_header(global_header, {_tree.begin(), _tree.end()});
        return SUCCESS;
      }
      // proceed to next child
      else {
        global_header.ptr_              = read_root_child(global_header.child_idx_, {_tree.begin(), _tree.end()});

        global_header.last_result_.dir_ = DOWN;
        write_global_node_header(global_header, {_tree.begin(), _tree.end()});
        return BUSY;
      }
    }

    write_global_node_header(global_header, {_tree.begin(), _tree.end()});
    return BUSY;

  }  // execute

  // prepares for the execution of a tree
  template <class Variant, class Tree, class StateProvider, class... Ts>
  [[nodiscard]] std::function<State()> prepare(Tree _tree, StateProvider& _states, Ts... _ts) {
    return
        [tree = std::move(_tree), states = std::ref(_states), params = std::make_tuple(std::move(_ts)...)]() -> State {
          return execute_step<Variant>(*const_cast<Tree*>(&tree), states.get(), params);
        };
  }  // prepare

}  // namespace TBT::Execute
