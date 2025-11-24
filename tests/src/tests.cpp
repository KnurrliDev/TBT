#include <TBT/TBT>
#include <catch2/catch_test_macros.hpp>
#include <iomanip>
#include <iostream>

using namespace TBT;
using namespace Compiler;

TEST_CASE("string splitting", "[util]") {
  F_SPLIT

  SECTION("no empty parts") {
    const std::string_view input = "apple,banana,orange";
    constexpr char delim         = ',';

    const auto result            = split(input, delim);

    REQUIRE(result.size() == 3);

    REQUIRE(result[0] == "apple");
    REQUIRE(result[1] == "banana");
    REQUIRE(result[2] == "orange");
  }

  SECTION("string with empty parts") {
    const std::string_view input = "|||apple|banana|||orange|||";
    constexpr char delim         = '|';

    const auto result            = split(input, delim);

    REQUIRE(result.size() == 11);

    REQUIRE(result[0].empty());
    REQUIRE(result[1].empty());
    REQUIRE(result[2].empty());

    REQUIRE(result[3] == "apple");

    REQUIRE(result[4] == "banana");

    REQUIRE(result[5].empty());
    REQUIRE(result[6].empty());

    REQUIRE(result[7] == "orange");

    REQUIRE(result[8].empty());
    REQUIRE(result[9].empty());
    REQUIRE(result[10].empty());
  }

  SECTION("Empty string") {
    constexpr std::string_view input = "";
    constexpr char delim             = '|';

    const auto result                = split(input, delim);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].empty());
  }

  SECTION("Single segment without delimiter") {
    constexpr std::string_view input = "apple";
    constexpr char delim             = '|';

    const auto result                = split(input, delim);

    REQUIRE(result.size() == 1);

    REQUIRE(result[0] == "apple");
  }

  SECTION("Only empty parts") {
    constexpr std::string_view input = "||";
    constexpr char delim             = '|';

    const auto result                = split(input, delim);

    REQUIRE(result.size() == 3);
    REQUIRE(result[0].empty());
    REQUIRE(result[1].empty());
    REQUIRE(result[2].empty());
  }
}

TEST_CASE("string clean", "[util]") {
  F_CLEAN

  std::string input     = R"(   apple,

       banana  , orange)";
  const std::string res = clean(input);

  REQUIRE(res == "apple,banana,orange");
}

TEST_CASE("parameter extraction", "[util]") {
  F_SPLIT
  F_PARAMS

  SECTION("simple pack") {
    constexpr std::string_view input = "4.3,-12,false,$2,true";

    const auto result                = extract_params(input);

    REQUIRE(result.size() == 5);
    REQUIRE(std::get<float>(result[0]) == 4.3f);
    REQUIRE(std::get<int32_t>(result[1]) == -12);
    REQUIRE(std::get<bool>(result[2]) == false);
    REQUIRE(std::get<uint32_t>(result[3]) == 2);
    REQUIRE(std::get<bool>(result[4]) == true);
  }
}

TEST_CASE("node list", "[Hierarchy]") {
  F_SPLIT
  F_CLEAN
  F_PARAMS

  const std::array<std::pair<int32_t, std::string_view>, 4> variant = {
      std::make_pair<int32_t, std::string_view>(1, "Task")};

  H_EXTRACT

  SECTION("error in task") {
    const std::string input = "Tas";
    const std::string res   = clean(input);

    const auto hh           = h_extract({res.data(), res.size()});
    REQUIRE(!hh);
    REQUIRE(hh.error() == "Tas");

    // std::cout << std::format("Error: {}", hh.error()) << std::endl;
  }

  SECTION("Pure hierarchy") {
    const std::string input = "Task(true), Task(true, true)[ Task[ Task(4.2)], Task, Task(true)] Task";
    const std::string res   = clean(input);

    const auto hh           = h_extract({res.data(), res.size()});
    REQUIRE(hh);
    const std::vector<Node> h = hh.value();

    REQUIRE(h.size() == 7);

    REQUIRE(h[0].node_id_ == 1);
    REQUIRE(h[0].type_idx_ == 1);
    REQUIRE(h[0].level_ == 0);
    REQUIRE(h[0].parent_ == 0);
    REQUIRE(h[0].p_.size() == 1);

    REQUIRE(h[1].node_id_ == 2);
    REQUIRE(h[1].type_idx_ == 1);
    REQUIRE(h[1].level_ == 0);
    REQUIRE(h[1].parent_ == 0);
    REQUIRE(h[1].p_.size() == 2);

    REQUIRE(h[2].node_id_ == 3);
    REQUIRE(h[2].type_idx_ == 1);
    REQUIRE(h[2].level_ == 1);
    REQUIRE(h[2].parent_ == 2);
    REQUIRE(h[2].p_.size() == 0);

    REQUIRE(h[3].node_id_ == 4);
    REQUIRE(h[3].type_idx_ == 1);
    REQUIRE(h[3].level_ == 2);
    REQUIRE(h[3].parent_ == 3);
    REQUIRE(h[3].p_.size() == 1);

    REQUIRE(h[4].node_id_ == 5);
    REQUIRE(h[4].type_idx_ == 1);
    REQUIRE(h[4].level_ == 1);
    REQUIRE(h[4].parent_ == 2);
    REQUIRE(h[4].p_.size() == 0);

    REQUIRE(h[5].node_id_ == 6);
    REQUIRE(h[5].type_idx_ == 1);
    REQUIRE(h[5].level_ == 1);
    REQUIRE(h[5].parent_ == 2);
    REQUIRE(h[5].p_.size() == 1);

    REQUIRE(h[6].node_id_ == 7);
    REQUIRE(h[6].type_idx_ == 1);
    REQUIRE(h[6].level_ == 0);
    REQUIRE(h[6].parent_ == 0);
    REQUIRE(h[6].p_.size() == 0);
  }

  SECTION("two children nested hierarchy") {
    const std::string input = "Task[Task[Task[Task]]]";
    const std::string res   = clean(input);

    const auto hh           = h_extract({res.data(), res.size()});
    REQUIRE(hh);
    const std::vector<Node> h = hh.value();

    REQUIRE(h.size() == 4);

    for (int32_t i = 0; i < 4; ++i) {
      REQUIRE(h[i].node_id_ == i + 1);
      REQUIRE(h[i].type_idx_ == 1);
      REQUIRE(h[i].level_ == i);
      // REQUIRE(h[i].parent_ == (i == 0 ? -1 : i));
      REQUIRE(h[i].p_.empty());
      REQUIRE(h[i].cl_ == "Task");
    }
  }

  SECTION("Deep hierarchy") {
    const std::string input =
        "Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task]]]]]]]]]]]]]]]]";
    const std::string res = clean(input);

    const auto hh         = h_extract({res.data(), res.size()});
    REQUIRE(hh);
    const std::vector<Node> h = hh.value();

    REQUIRE(h.size() == 17);

    for (int32_t i = 0; i < 16; ++i) {
      REQUIRE(h[i].node_id_ == i + 1);
      REQUIRE(h[i].type_idx_ == 1);
      REQUIRE(h[i].level_ == i);
      REQUIRE(h[i].p_.empty());
      REQUIRE(h[i].cl_ == "Task");
    }
  }
}

TEST_CASE("static node list", "[Hierarchy]") {
  SECTION("static error in task") {
    constexpr auto static_ex = [](const std::string_view& _s) constexpr -> bool {
      F_SPLIT
      F_CLEAN
      F_PARAMS

      constexpr std::array<std::pair<int32_t, std::string_view>, 4> variant = {
          std::make_pair<int32_t, std::string_view>(1, "Task")};

      H_EXTRACT

      const auto hh = h_extract(_s);
      return hh.error() == "Tas";
    };

    constexpr bool res = static_ex("Tas");
    static_assert(res);
  }
}

struct TaskA {
  int32_t val_ = 1;
};
#define TASK_TYPE TaskA
#include <TBT/magic.hpp>

struct TaskB {
  int32_t val_ = 2;
};
#define TASK_TYPE TaskB
#include <TBT/magic.hpp>

struct TaskC {
  int32_t val_ = 3;
  int32_t it_  = 0;
};
#define TASK_TYPE TaskC
#include <TBT/magic.hpp>

using Variant = std::variant<TASK_TYPES>;

TEST_CASE("dynamic extract node list", "[Composite]") {
  constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskA($1)[TaskB($2), TaskC($3)] TaskA($4)";

  const auto res               = compile_dynamic<Variant>(s);

  const Header gh              = read_global_node_header(res.data());
  REQUIRE(gh.node_count_ == 4);
  REQUIRE(gh.children_count_ == 2);

  const uint32_t rc0  = read_root_child(0, res.data());
  const uint32_t rc1  = read_root_child(1, res.data());

  const NodeHeader n1 = read_node_header(res.data() + rc0);
  REQUIRE(n1.type_idx_ == 0);
  REQUIRE(n1.children_count_ == 2);
  REQUIRE(n1.parent_ == 0);
  {
    const auto pl = read_payload(0, n1, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 1);
  }

  const uint32_t c11  = read_child(0, n1, res.data());
  const uint32_t c12  = read_child(1, n1, res.data());

  const NodeHeader n2 = read_node_header(res.data() + c11);
  REQUIRE(n2.type_idx_ == 1);
  REQUIRE(n2.children_count_ == 0);
  REQUIRE(n2.parent_ != 0);
  {
    const auto pl = read_payload(0, n2, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 2);
  }

  const NodeHeader n3 = read_node_header(res.data() + c12);
  REQUIRE(n3.type_idx_ == 2);
  REQUIRE(n3.children_count_ == 0);
  REQUIRE(n3.parent_ == n2.parent_);
  {
    const auto pl = read_payload(0, n3, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 3);
  }

  //-----------------------------------------

  const NodeHeader n4 = read_node_header(res.data() + rc1);
  REQUIRE(n4.type_idx_ == 0);
  REQUIRE(n4.children_count_ == 0);
  REQUIRE(n4.parent_ == 0);
  {
    const auto pl = read_payload(0, n4, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 4);
  }
}

TEST_CASE("static extract node list", "[Composite]") {
  using Variant                = std::variant<TaskA, TaskB, TaskC>;

  constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskA($1)[TaskB($2), TaskC($3)] TaskA($4)";
  constexpr size_t r_size      = compute_size_static<Variant>(s);
  constexpr auto res           = compile_static<r_size, Variant>(s);

  const auto res1              = compile_dynamic<Variant>(s);

  REQUIRE(res.size() == res1.size());

  for (size_t i = 0; i < res.size(); ++i) { REQUIRE(res[i] == res1[i]); }

  const Header gh = read_global_node_header(res.data());
  REQUIRE(gh.node_count_ == 4);
  REQUIRE(gh.children_count_ == 2);

  const uint32_t rc0  = read_root_child(0, res.data());
  const uint32_t rc1  = read_root_child(1, res.data());

  const NodeHeader n1 = read_node_header(res.data() + rc0);
  REQUIRE(n1.type_idx_ == 0);
  REQUIRE(n1.children_count_ == 2);
  REQUIRE(n1.parent_ == 0);
  {
    const auto pl = read_payload(0, n1, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 1);
  }

  const uint32_t c11  = read_child(0, n1, res.data());
  const uint32_t c12  = read_child(1, n1, res.data());

  const NodeHeader n2 = read_node_header(res.data() + c11);
  REQUIRE(n2.type_idx_ == 1);
  REQUIRE(n2.children_count_ == 0);
  REQUIRE(n2.parent_ != 0);
  {
    const auto pl = read_payload(0, n2, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 2);
  }

  const NodeHeader n3 = read_node_header(res.data() + c12);
  REQUIRE(n3.type_idx_ == 2);
  REQUIRE(n3.children_count_ == 0);
  REQUIRE(n3.parent_ == n2.parent_);
  {
    const auto pl = read_payload(0, n3, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 3);
  }

  //-----------------------------------------

  const NodeHeader n4 = read_node_header(res.data() + rc1);
  REQUIRE(n4.type_idx_ == 0);
  REQUIRE(n4.children_count_ == 0);
  REQUIRE(n4.parent_ == 0);
  {
    const auto pl = read_payload(0, n4, res.data());
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 4);
  }
}

//---------------------------------------

template <class States>
TBT::State init(const TaskA& _t, States& _s) {
  _s.t_.push_back(std::format("init [{}]", _t.val_));
  return SUCCESS;
}
template <class States>
TBT::State run(const TaskA& _t, States& _s) {
  _s.t_.push_back(std::format("run [{}]", _t.val_));
  return SUCCESS;
}
template <class States>
void exit(const TaskA& _t, States& _s) {
  _s.t_.push_back(std::format("exit [{}]", _t.val_));
}

//---------------------------------------

template <class States>
TBT::State init(const TaskB& _t, States& _s) {
  _s.t_.push_back(std::format("init [{}]", _t.val_));
  return SUCCESS;
}
template <class States>
TBT::State run(const TaskB& _t, States& _s) {
  _s.t_.push_back(std::format("run [{}]", _t.val_));
  return SUCCESS;
}
template <class States>
void exit(const TaskB& _t, States& _s) {
  _s.t_.push_back(std::format("exit [{}]", _t.val_));
}

//---------------------------------------

template <class States>
TBT::State init(const TaskC& _t, States& _s) {
  _s.t_.push_back(std::format("init [{}]", _t.val_));
  return SUCCESS;
}
template <class States>
TBT::State run(TaskC& _t, States& _s) {
  if (_t.it_ < 3) {
    _s.t_.push_back(std::format("run [{}]", _t.val_));
    _t.it_++;
    return BUSY;
  }
  return SUCCESS;
}
template <class States>
void exit(const TaskC& _t, States& _s) {
  _s.t_.push_back(std::format("exit [{}]", _t.val_));
}

TEST_CASE("hierarchy", "[Execute]") {
  using Variant                = std::variant<TaskA, TaskB, TaskC>;

  constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskC, TaskA($0)[TaskB(5)[TaskA, TaskB]] TaskA[TaskC]";
  constexpr size_t r_size      = compute_size_static<Variant>(s);
  constexpr auto res           = compile_static<r_size, Variant>(s);

  // const auto res          = compile_dynamic<Variant>(s);

  struct States {
    std::vector<std::string> t_;
  } states;

  auto tree = res;
  while (Execute::execute_step<Variant>(tree, states, std::make_tuple(-5)) == BUSY) {
    //
  }

  size_t i = 0;
  REQUIRE("init [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("exit [3]" == states.t_[i++]);
  REQUIRE("init [-5]" == states.t_[i++]);
  REQUIRE("run [-5]" == states.t_[i++]);
  REQUIRE("exit [-5]" == states.t_[i++]);
  REQUIRE("init [5]" == states.t_[i++]);
  REQUIRE("run [5]" == states.t_[i++]);
  REQUIRE("exit [5]" == states.t_[i++]);
  REQUIRE("init [1]" == states.t_[i++]);
  REQUIRE("run [1]" == states.t_[i++]);
  REQUIRE("exit [1]" == states.t_[i++]);
  REQUIRE("init [2]" == states.t_[i++]);
  REQUIRE("run [2]" == states.t_[i++]);
  REQUIRE("exit [2]" == states.t_[i++]);
  REQUIRE("init [1]" == states.t_[i++]);
  REQUIRE("run [1]" == states.t_[i++]);
  REQUIRE("exit [1]" == states.t_[i++]);
  REQUIRE("init [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("run [3]" == states.t_[i++]);
  REQUIRE("exit [3]" == states.t_[i++]);
}

template <class Variant_>
struct StateProvider {
  using Variant = Variant_;
  std::vector<std::string> t_;

  TaskQueue tasks_queue_;
};

TEST_CASE("prepare", "[Execute]") {
  using Variant                = std::variant<TaskA, TaskB, TaskC>;

  constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskC, TaskA($0)[TaskB(5)[TaskA, TaskB]] TaskA[TaskC]";

  StateProvider<Variant> states;

  const auto prep  = Execute::prepare<Variant>(compile_static<compute_size_static<Variant>(s), Variant>(s), states, -5);

  const auto prep0 = Execute::prepare<typename decltype(states)::Variant>(
      compile_static<compute_size_static<typename decltype(states)::Variant>(s), typename decltype(states)::Variant>(s),
      states, -5);

  const auto prep1 = COMPILE_AND_PREPARE(s, states, -5);
  const auto prep2 = COMPILE_AND_PREPARE("TaskC, TaskA($0)[TaskB(5)[TaskA, TaskB]] TaskA[TaskC]", states, -5);

  const auto [promise, tree_ptr] =
      COMPILE_AND_QUEUE("TaskC, TaskA($0)[TaskB(5)[TaskA, TaskB]] TaskA[TaskC]", states, -5);

  while (true) { EXECUTE_QUEUE(states) }
}