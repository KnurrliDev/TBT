#pragma once

#include <TBT/compiler.hpp>

namespace TBT::Execute {

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

    template <class TaskDerived, class StateProvider>
    concept has_any_init_sig = has_init_sig_1<TaskDerived, StateProvider> || has_init_sig_2<TaskDerived>;

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

    template <class TaskDerived, class StateProvider>
    concept has_any_sig = has_any_run_sig<TaskDerived, StateProvider> || has_any_run_sig<TaskDerived, StateProvider>;

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

    template <class TaskDerived, class StateProvider>
    concept has_any_exit_sig = has_exit_sig_1<TaskDerived, StateProvider> || has_exit_sig_2<TaskDerived>;

  }  // namespace Concepts

  template <typename... Ts>
  std::variant<std::remove_reference_t<Ts>...> tuple_element_to_variant(const std::tuple<Ts...>& tuple,
                                                                        const size_t _index) {
    std::variant<std::remove_reference_t<Ts>...> result;
    size_t current = 0;
    ((current++ == _index ? result = std::get<Ts>(std::move(tuple)) : result), ...);
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

      Variant* state = alloc_task<Variant>(_header.type_idx_, idcs, payloads, _params);

      // run the task
      {
        const State res = std::visit(
            [&](auto& _t) {
              if constexpr (Concepts::has_init_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                return init(_t, _states);
              else if constexpr (Concepts::has_init_sig_2<std::decay_t<decltype(_t)>>)
                return init(_t);
              // else
              //   static_assert(false, "no signature found");
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
          _global_header.ptr_                = _header.parent_;
          _global_header.last_result_.dir_   = UP;
          _global_header.last_result_.state_ = res;
          return BUSY;
        }
      }

      // the task succeeded. check if wait exists, else return
      {
        const std::optional<State> res = std::visit(
            [&](auto& _t) {
              if constexpr (Concepts::has_run_sig_1<std::decay_t<decltype(_t)>, std::decay_t<StateProvider>>)
                return run(_t, _states);
              else if constexpr (Concepts::has_run_sig_2<std::decay_t<decltype(_t)>>)
                return run(_t);
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

          // return to parent if no children or last child
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

    //------------------------------------------------------------

    // re enter. either the task returned BUSY or returning from a child
    else {
      // returning from a child. go to next or return to parent
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

      // keep running the task
      assert(task.ptr_ != 0);
      Variant* state = reinterpret_cast<Variant*>(task.ptr_);

      State res      = FAILED;
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

        // return to parent if no children or last child
        _global_header.ptr_                = _header.parent_;
        _global_header.last_result_.dir_   = UP;
        _global_header.last_result_.state_ = res;
        return BUSY;
      }

      // wait longer
      _global_header.last_result_.dir_   = UP;
      _global_header.last_result_.state_ = res;
      return BUSY;
    }
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
