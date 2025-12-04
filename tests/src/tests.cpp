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

  const auto h_extract = [&](std::string_view s) -> std::expected<std::vector<Node>, std::string_view> {
    std::vector<Node> nodes; /* {level, parent_id} */
    std::vector<std::pair<int32_t, int32_t>> stack = {{0, 0}};
    int32_t next_id                                = 1;
    size_t i                                       = 0;

    const auto parse_node_name                     = [&]() -> std::expected<std::string_view, std::string_view> {
      size_t start = i;
      while (i < s.size()) {
        char c = s[i];
        if (c == '(' || c == '[' || c == ']' || c == ',') break;
        ++i;
      }
      if (i == start) return std::unexpected("empty node name");
      return s.substr(start, i - start);
    };

    const auto parse_params = [&]() -> std::vector<Parameter> {
      if (i >= s.size() || s[i] != '(') return {};
      ++i; /* skip '(' */
      int32_t depth = 1;
      size_t start  = i;
      while (i < s.size() && depth > 0) {
        if (s[i] == '(')
          ++depth;
        else if (s[i] == ')')
          --depth;
        ++i;
      }
      return extract_params(s.substr(start, i - start - 1));
    };

    const auto get_idx_for_type = [&](const std::string_view& _ss) -> std::optional<size_t> {
      const auto t = _ss.substr(0, _ss.find_first_of('('));
      for (size_t i = 0; i < variant.size(); ++i)
        if (variant[i].second == t) return variant[i].first;
      return std::nullopt;
    };

    while (i < s.size()) { /* Parse node name */
      auto name_res = parse_node_name();
      if (!name_res) return std::unexpected(name_res.error());
      std::string_view name = name_res.value();

      Node n;
      n.cl_      = name;
      n.node_id_ = next_id++;
      n.level_   = stack.back().first;
      n.parent_  = stack.back().second;

      auto idx   = get_idx_for_type(name);
      if (!idx) return std::unexpected(name);
      n.type_idx_ = (int32_t)idx.value();

      /* Parse optional parameters */
      if (i < s.size() && s[i] == '(') { n.p_ = parse_params(); }

      nodes.push_back(std::move(n));

      if (i >= s.size()) break;

      char c = s[i];

      if (c == '[') {
        stack.push_back({stack.back().first + 1, n.node_id_});
        ++i;
      } else if (c == ']') {
        stack.pop_back();
        ++i; /* Handle possible consecutive ]]... by continuing */
        while (i < s.size() && s[i] == ']') {
          stack.pop_back();
          ++i;
        } /* After closing brackets, expect either ',' or end or another node */
        if (i < s.size() && s[i] == ',') ++i;
      } else if (c == ',') {
        ++i;
      } else {
        return std::unexpected(s.substr(i, 10)); /* invalid char */
      }
    }

    return nodes;
  };

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
      // F_CLEAN
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

TEST_CASE("serialize", "[Compiler]") {
  SECTION("De/serialize base structs", "[Hierarchy]") {
    {
      constexpr Composite val{42, 42};

      constexpr auto s        = serialize_composite(val);
      constexpr Composite res = deserialize_composite(s);

      static_assert(val.cur_idx_ == res.cur_idx_);
      static_assert(val.ptr_ == res.ptr_);
    }

    {
      constexpr Result val{State::FAILED, Direction::UP};

      constexpr auto s     = serialize_result(val);
      constexpr Result res = deserialize_result(s);

      static_assert(val.state_ == res.state_);
      static_assert(val.dir_ == res.dir_);
    }

    {
      constexpr Header val{42, 42, 42, 42, {State::FAILED, Direction::UP}, 42};

      constexpr auto s     = serialize_header(val);
      constexpr Header res = deserialize_header(s);

      static_assert(val.node_count_ == res.node_count_);
      static_assert(val.ptr_ == res.ptr_);
      static_assert(val.ptr_ == res.ptr_);
      static_assert(val.children_count_ == res.children_count_);
      static_assert(val.last_result_.state_ == res.last_result_.state_);
      static_assert(val.last_result_.dir_ == res.last_result_.dir_);
      static_assert(val.child_idx_ == res.child_idx_);
    }

    {
      constexpr NodeHeader val{42, 42, 42, 42, 42, 42, 42, 42};

      constexpr auto s         = serialize_node_header(val);
      constexpr NodeHeader res = deserialize_node_header(s);

      static_assert(val.type_idx_ == res.type_idx_);
      static_assert(val.parent_ == res.parent_);

      static_assert(val.children_offset_ == res.children_offset_);
      static_assert(val.children_count_ == res.children_count_);

      static_assert(val.params_offset_ == res.params_offset_);
      static_assert(val.params_count_ == res.params_count_);

      static_assert(val.comp_offset_ == res.comp_offset_);
      static_assert(val.node_size_ == res.node_size_);
    }
  }

  SECTION("De/serialize arthimetic types", "[Hierarchy]") {
    {
      constexpr bool val      = true;

      // bool is encoded as a int32_t to keep in line with the other arithmetic types used
      constexpr size_t size_v = real_size<decltype(val)>();
      static_assert(size_v == sizeof(int32_t));

      constexpr auto s_v = serialize<decltype(val), size_v>(val);
      constexpr auto d_v = deserialize<decltype(val), size_v>(s_v);

      static_assert(val == d_v);
    }

    {
      constexpr int32_t val   = -42;

      constexpr size_t size_v = real_size<decltype(val)>();
      static_assert(size_v == sizeof(int32_t));

      constexpr auto s_v = serialize<decltype(val), size_v>(val);
      constexpr auto d_v = deserialize<decltype(val), size_v>(s_v);

      static_assert(val == d_v);
    }

    {
      constexpr uint32_t val  = 42;

      constexpr size_t size_v = real_size<decltype(val)>();
      static_assert(size_v == sizeof(int32_t));

      constexpr auto s_v = serialize<decltype(val), size_v>(val);
      constexpr auto d_v = deserialize<decltype(val), size_v>(s_v);

      static_assert(val == d_v);
    }

    {
      constexpr float val     = -42.335f;

      constexpr size_t size_v = real_size<decltype(val)>();
      static_assert(size_v == sizeof(int32_t));

      constexpr auto s_v = serialize<decltype(val), size_v>(val);
      constexpr auto d_v = deserialize<decltype(val), size_v>(s_v);

      static_assert(val == d_v);
    }
  }
}

TEST_CASE("serialize helper functions", "[Compiler]") {
  SECTION("global header") {
    constexpr Header val{42, 42, 42, 42, {State::BUSY, Direction::UP}, 42};
    constexpr std::array<uint8_t, RealSize::header> ar = [val]() constexpr {
      std::array<uint8_t, RealSize::header> out = {};
      write_global_node_header(val, out);
      return out;
    }();

    constexpr Header res = read_global_node_header(ar);

    static_assert(val.node_count_ == res.node_count_);
    static_assert(val.ptr_ == res.ptr_);

    static_assert(val.children_count_ == res.children_count_);
    static_assert(val.first_node_offset_ == res.first_node_offset_);

    static_assert(val.last_result_.state_ == res.last_result_.state_);
    static_assert(val.last_result_.dir_ == res.last_result_.dir_);

    static_assert(val.child_idx_ == res.child_idx_);
  }

  SECTION("Composite") {
    constexpr Composite val{42, 42};
    constexpr std::array<uint8_t, RealSize::composite> ar = [val]() constexpr {
      std::array<uint8_t, RealSize::composite> out = {};
      NodeHeader header;
      header.comp_offset_ = 0;
      write_composite(val, header, out);
      return out;
    }();

    constexpr Composite res = [ar]() constexpr {
      NodeHeader header;
      header.comp_offset_ = 0;
      return read_composite(header, ar);
    }();

    static_assert(val.ptr_ == res.ptr_);
    static_assert(val.cur_idx_ == res.cur_idx_);
  }

  SECTION("NodeHeader") {
    constexpr NodeHeader val{42, 42, 42, 42, 42, 42, 42, 42};
    constexpr std::array<uint8_t, RealSize::node_header> ar = [val]() constexpr {
      std::array<uint8_t, RealSize::node_header> out = {};
      write_node_header(val, out);
      return out;
    }();

    constexpr NodeHeader res = [ar]() constexpr { return read_node_header(ar); }();

    static_assert(val.type_idx_ == res.type_idx_);
    static_assert(val.parent_ == res.parent_);

    static_assert(val.children_offset_ == res.children_offset_);
    static_assert(val.children_count_ == res.children_count_);

    static_assert(val.params_offset_ == res.params_offset_);
    static_assert(val.params_count_ == res.params_count_);

    static_assert(val.comp_offset_ == res.comp_offset_);
    static_assert(val.node_size_ == res.node_size_);
  }

  SECTION("root children") {
    constexpr int32_t c1                                                = 42;
    constexpr int32_t c2                                                = 43;
    constexpr int32_t c3                                                = 44;
    constexpr std::array<uint8_t, RealSize::header + 3 * sizeof(42)> ar = [&]() constexpr {
      std::array<uint8_t, RealSize::header + 3 * sizeof(42)> out = {};
      write_root_child(0, c1, out);
      write_root_child(1, c2, out);
      write_root_child(2, c3, out);
      return out;
    }();

    constexpr int32_t r_c1 = [ar]() constexpr { return read_root_child(0, ar); }();
    constexpr int32_t r_c2 = [ar]() constexpr { return read_root_child(1, ar); }();
    constexpr int32_t r_c3 = [ar]() constexpr { return read_root_child(2, ar); }();

    static_assert(c1 == r_c1);
    static_assert(c2 == r_c2);
    static_assert(c3 == r_c3);
  }

  SECTION("children") {
    constexpr int32_t c1                                                     = 42;
    constexpr int32_t c2                                                     = 43;
    constexpr int32_t c3                                                     = 44;
    constexpr std::array<uint8_t, RealSize::node_header + 3 * sizeof(42)> ar = [&]() constexpr {
      std::array<uint8_t, RealSize::node_header + 3 * sizeof(42)> out = {};
      write_child(0, c1, out);
      write_child(1, c2, out);
      write_child(2, c3, out);
      return out;
    }();

    constexpr int32_t r_c1 = [ar]() constexpr { return read_child(0, ar); }();
    constexpr int32_t r_c2 = [ar]() constexpr { return read_child(1, ar); }();
    constexpr int32_t r_c3 = [ar]() constexpr { return read_child(2, ar); }();

    static_assert(c1 == r_c1);
    static_assert(c2 == r_c2);
    static_assert(c3 == r_c3);
  }

  SECTION("Payload") {
    constexpr Parameter p1                                       = true;
    constexpr Parameter p2                                       = -42.333f;
    constexpr Parameter p3                                       = int32_t(42);
    constexpr Parameter p4                                       = uint32_t(42);
    //
    constexpr std::array<uint8_t, 4 * real_size<Parameter>()> ar = [p1, p2, p3, p4]() constexpr {
      NodeHeader header;
      header.params_offset_                               = 0;

      std::array<uint8_t, 4 * real_size<Parameter>()> out = {};

      write_payload(0, header, p1, out);
      write_payload(1, header, p2, out);
      write_payload(2, header, p3, out);
      write_payload(3, header, p4, out);

      return out;
    }();

    constexpr Parameter p1r = [ar]() constexpr {
      NodeHeader header;
      header.params_offset_ = 0;

      return read_payload(0, header, ar);
    }();

    static_assert(p1r.index() == 0);
    static_assert(std::get<0>(p1r) == std::get<0>(p1));

    constexpr Parameter p2r = [ar]() constexpr {
      NodeHeader header;
      header.params_offset_ = 0;

      return read_payload(1, header, ar);
    }();

    static_assert(p2r.index() == 2);
    static_assert(std::get<2>(p2r) == std::get<2>(p2));

    constexpr Parameter p3r = [ar]() constexpr {
      NodeHeader header;
      header.params_offset_ = 0;

      return read_payload(2, header, ar);
    }();

    static_assert(p3r.index() == 1);
    static_assert(std::get<1>(p3r) == std::get<1>(p3));

    constexpr Parameter p4r = [ar]() constexpr {
      NodeHeader header;
      header.params_offset_ = 0;

      return read_payload(3, header, ar);
    }();

    static_assert(p4r.index() == 3);
    static_assert(std::get<3>(p4r) == std::get<3>(p4));
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

struct TaskD {
  int32_t val_ = 40;
};
#define TASK_TYPE TaskD
#include <TBT/magic.hpp>

struct TaskE {
  int32_t val_ = 50;
};
#define TASK_TYPE TaskE
#include <TBT/magic.hpp>

struct MoveTask {
  bool enable{};
  int32_t steps{};
  float speed{};
  int32_t id{};
};
template <>
struct glz::meta<MoveTask> {
  using T                     = MoveTask;
  static constexpr auto value = glz::object(&T::enable, "enable", &T::steps, "steps", &T::speed, "speed", &T::id, "id");
};

struct JumpTask {
  float height{};
  bool repeat{};
};
template <>
struct glz::meta<JumpTask> {
  using T                     = JumpTask;
  static constexpr auto value = glz::object(&T::height, "height", &T::repeat, "repeat");
};

TEST_CASE("tuple_element_to_variant extracts the correct element", "[utility]") {
  auto tup = std::make_tuple(42, 3.14f, true, -42);

  auto v0  = Execute::tuple_element_to_variant(tup, 0);
  auto v1  = Execute::tuple_element_to_variant(tup, 1);
  auto v2  = Execute::tuple_element_to_variant(tup, 2);
  auto v3  = Execute::tuple_element_to_variant(tup, 3);

  REQUIRE(std::holds_alternative<int32_t>(v0));
  REQUIRE(std::get<int32_t>(v0) == 42);

  REQUIRE(std::holds_alternative<float>(v1));
  REQUIRE(std::get<float>(v1) == 3.14f);

  REQUIRE(std::holds_alternative<bool>(v2));
  REQUIRE(std::get<bool>(v2) == true);

  REQUIRE(std::holds_alternative<int32_t>(v3));
  REQUIRE(std::get<int32_t>(v3) == -42);
}

TEST_CASE("construct_task - only static payload", "[construct_task]") {
  std::vector<uint32_t> idxs{0, 1, 2, 3};  // map fields 0-3 to payload indices
  std::vector<Parameter> pl{true, int32_t{-500}, 2.5f, int32_t{99}};
  // only bool, float and signed ints are allowed as static payload
  auto task = Execute::construct_task<MoveTask>(idxs, pl, std::tuple<>{});

  REQUIRE(task.enable == true);
  REQUIRE(task.steps == -500);
  REQUIRE(task.speed == 2.5f);
  REQUIRE(task.id == 99u);
}

TEST_CASE("construct_task - only dynamic parameters (tuple)", "[construct_task]") {
  std::vector<uint32_t> idxs{0, 1, 2, 3};                        // indices >= pl.size() → taken from tuple
  std::vector<std::variant<bool, int32_t, float, uint32_t>> pl;  // empty

  auto params = std::make_tuple(false, int32_t{1000}, 10.0f, int32_t{7});

  auto task   = Execute::construct_task<MoveTask>(idxs, pl, params);

  REQUIRE(task.enable == false);
  REQUIRE(task.steps == 1000);
  REQUIRE(task.speed == 10.0f);
  REQUIRE(task.id == 7u);
}

TEST_CASE("construct_task - mixed static + dynamic payload", "[construct_task]") {
  // field order: enable, steps, speed, id
  // idxs:  0 → static payload[0] = true
  //        3 → dynamic tuple[0]   = -200   (3 - 1 = 2, but we will use tuple index 0)
  //        1 → static payload[1] = 5.5f
  //        2 → static payload[2] = 42u
  std::vector<uint32_t> idxs{0, 3, 1, 2};
  std::vector<Parameter> pl{true, 5.5f, int32_t{42}};

  auto params = std::make_tuple(int32_t{-200});  // only one dynamic value

  auto task   = Execute::construct_task<MoveTask>(idxs, pl, params);

  REQUIRE(task.enable == true);  // static[0]
  REQUIRE(task.steps == -200);   // dynamic tuple[0]
  REQUIRE(task.speed == 5.5f);   // static[1]
  REQUIRE(task.id == 42u);       // static[2]
}

TEST_CASE("construct_task - fewer indices than members → remaining default constructed", "[construct_task]") {
  std::vector<uint32_t> idxs{0};  // only first field supplied
  std::vector<Parameter> pl{true};

  auto task = Execute::construct_task<MoveTask>(idxs, pl, std::tuple<>{});

  REQUIRE(task.enable == true);
  REQUIRE(task.steps == 0);
  REQUIRE(task.speed == 0.0f);
  REQUIRE(task.id == 0u);
}

TEST_CASE("alloc_task - empty index list (default construction)", "[alloc_task]") {
  using TaskVariant = std::variant<MoveTask, JumpTask>;

  auto* ptr         = Execute::alloc_task<TaskVariant>(0, {}, {}, std::tuple<>{});
  REQUIRE(ptr != nullptr);
  REQUIRE(std::holds_alternative<MoveTask>(*ptr));

  auto* ptr2 = Execute::alloc_task<TaskVariant>(1, {}, {}, std::tuple<>{});
  REQUIRE(std::holds_alternative<JumpTask>(*ptr2));

  delete ptr;
  delete ptr2;
}

TEST_CASE("alloc_task - full construction via construct_task", "[alloc_task]") {
  using TaskVariant = std::variant<MoveTask, JumpTask>;

  std::vector<uint32_t> idxs{0, 1, 2, 3};
  std::vector<Parameter> pl{false, int32_t{777}, 3.0f, int32_t{123}};

  // allocate variant index 0 → MoveTask
  auto* ptr = Execute::alloc_task<TaskVariant>(0, idxs, pl, std::tuple<>{});
  REQUIRE(std::holds_alternative<MoveTask>(*ptr));

  const auto& move = std::get<MoveTask>(*ptr);
  REQUIRE(move.enable == false);
  REQUIRE(move.steps == 777);
  REQUIRE(move.speed == 3.0f);
  REQUIRE(move.id == 123);

  delete ptr;
}

TEST_CASE("alloc_task - JumpTask with mixed payload", "[alloc_task]") {
  using TaskVariant = std::variant<MoveTask, JumpTask>;

  // JumpTask has two members: height (float), repeat (bool)
  std::vector<uint32_t> idxs{1, 0};  // height ← payload[1] (float), repeat ← payload[0] (bool)
  std::vector<Parameter> pl{true, 12.5f};

  auto* ptr = Execute::alloc_task<TaskVariant>(1, idxs, pl, std::tuple<>{});  // index 1 = JumpTask
  REQUIRE(std::holds_alternative<JumpTask>(*ptr));

  const auto& jump = std::get<JumpTask>(*ptr);
  REQUIRE(jump.height == 12.5f);
  REQUIRE(jump.repeat == true);

  delete ptr;
}

// in an actual project we would use this.
// using Variant = std::variant<TASK_TYPES>;
using Variant = std::variant<TaskA, TaskB, TaskC>;

TEST_CASE("dynamic extract node list", "[Composite]") {
  // constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskA($1)[TaskB($2), TaskC($3)] TaskA($4)";

  const auto res               = compile_dynamic<Variant>(s);

  const Header gh              = read_global_node_header(res);
  REQUIRE(gh.node_count_ == 4);
  REQUIRE(gh.children_count_ == 2);

  const uint32_t rc0  = read_root_child(0, res);
  const uint32_t rc1  = read_root_child(1, res);

  const NodeHeader n1 = read_node_header({res.cbegin() + rc0, res.cend()});
  REQUIRE(n1.type_idx_ == 0);
  REQUIRE(n1.children_count_ == 2);
  REQUIRE(n1.parent_ == 0);
  {
    const auto pl = read_payload(0, n1, {res.cbegin() + rc0, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 1);
  }

  const uint32_t c11  = read_child(0, {res.cbegin() + rc0, res.cend()});
  const uint32_t c12  = read_child(1, {res.cbegin() + rc0, res.cend()});

  const NodeHeader n2 = read_node_header({res.cbegin() + c11, res.cend()});
  REQUIRE(n2.type_idx_ == 1);
  REQUIRE(n2.children_count_ == 0);
  REQUIRE(n2.parent_ != 0);
  {
    const auto pl = read_payload(0, n2, {res.cbegin() + c11, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 2);
  }

  const NodeHeader n3 = read_node_header({res.cbegin() + c12, res.cend()});
  REQUIRE(n3.type_idx_ == 2);
  REQUIRE(n3.children_count_ == 0);
  REQUIRE(n3.parent_ == n2.parent_);
  {
    const auto pl = read_payload(0, n3, {res.cbegin() + c12, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 3);
  }

  //-----------------------------------------

  const NodeHeader n4 = read_node_header({res.cbegin() + rc1, res.cend()});
  REQUIRE(n4.type_idx_ == 0);
  REQUIRE(n4.children_count_ == 0);
  REQUIRE(n4.parent_ == 0);
  {
    const auto pl = read_payload(0, n4, {res.cbegin() + rc1, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 4);
  }
}

TEST_CASE("static extract node list", "[Composite]") {
  // constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskA($1)[TaskB($2), TaskC($3)] TaskA($4)";
  constexpr size_t r_size      = compute_size_static<Variant>(s);
  constexpr auto res           = compile_static<r_size, Variant>(s);

  const auto res1              = compile_dynamic<Variant>(s);

  REQUIRE(res.size() == res1.size());

  for (size_t i = 0; i < res.size(); ++i) { REQUIRE(res[i] == res1[i]); }

  const Header gh = read_global_node_header(res);
  REQUIRE(gh.node_count_ == 4);
  REQUIRE(gh.children_count_ == 2);

  const uint32_t rc0  = read_root_child(0, res);
  const uint32_t rc1  = read_root_child(1, res);

  const NodeHeader n1 = read_node_header({res.cbegin() + rc0, res.cend()});
  REQUIRE(n1.type_idx_ == 0);
  REQUIRE(n1.children_count_ == 2);
  REQUIRE(n1.parent_ == 0);
  {
    const auto pl = read_payload(0, n1, {res.cbegin() + rc0, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 1);
  }

  const uint32_t c11  = read_child(0, {res.cbegin() + rc0, res.cend()});
  const uint32_t c12  = read_child(1, {res.cbegin() + rc0, res.cend()});

  const NodeHeader n2 = read_node_header({res.cbegin() + c11, res.cend()});
  REQUIRE(n2.type_idx_ == 1);
  REQUIRE(n2.children_count_ == 0);
  REQUIRE(n2.parent_ != 0);
  {
    const auto pl = read_payload(0, n2, {res.cbegin() + c11, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 2);
  }

  const NodeHeader n3 = read_node_header({res.cbegin() + c12, res.cend()});
  REQUIRE(n3.type_idx_ == 2);
  REQUIRE(n3.children_count_ == 0);
  REQUIRE(n3.parent_ == n2.parent_);
  {
    const auto pl = read_payload(0, n3, {res.cbegin() + c12, res.cend()});
    REQUIRE(pl.index() == 3);
    REQUIRE(std::get<3>(pl) == 3);
  }

  //-----------------------------------------

  const NodeHeader n4 = read_node_header({res.cbegin() + rc1, res.cend()});
  REQUIRE(n4.type_idx_ == 0);
  REQUIRE(n4.children_count_ == 0);
  REQUIRE(n4.parent_ == 0);
  {
    const auto pl = read_payload(0, n4, {res.cbegin() + rc1, res.cend()});
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

//---------------------------------------

template <class States>
Execute::CoState co_run(TaskD& _t, States& _s) {
  _s.t_.push_back(std::format("co_await start [{}]", _t.val_));
  const State res = co_await COMPILE_AND_QUEUE(0, "TaskE", _s, STEPWISE_1);
  assert(res == SUCCESS);
  _s.t_.push_back(std::format("co_await end [{}]", _t.val_));
  co_return SUCCESS;
}
template <class States>
void exit(const TaskD& _t, States& _s) {
  _s.t_.push_back(std::format("exit [{}]", _t.val_));
}

//---------------------------------------

template <class States>
Execute::CoState co_run(TaskE& _t, States& _s) {
  _s.t_.push_back(std::format("co_yield [{}]", _t.val_));
  co_yield 0;
  _s.t_.push_back(std::format("co_yield [{}]", _t.val_));
  co_yield 0;
  _s.t_.push_back(std::format("co_yield [{}]", _t.val_));
  co_return SUCCESS;
}

template <class States>
void exit(const TaskE& _t, States& _s) {
  _s.t_.push_back(std::format("exit [{}]", _t.val_));
}

//---------------------------------------

TEST_CASE("hierarchy", "[Execute]") {
  using Variant                = std::variant<TaskA, TaskB, TaskC>;

  // constexpr auto vr            = variant_type_index_name_pairs<Variant>();

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

  TBT::TaskQueue<> tasks_queue_;
};

TEST_CASE("legacy", "[Execute]") {
  using Variant1 = std::variant<TaskA, TaskB, TaskC, TaskD, TaskE>;

  StateProvider<Variant1> sp;

  auto p = COMPILE_AND_QUEUE(0, "TaskA($0)[TaskB($1), TaskC($2)] TaskA($3)", sp, STEPWISE_1, 10, 20, 30, 40);

  for (int32_t i = 0; i < 1000; ++i) { EXECUTE_QUEUE(sp) }

  REQUIRE(sp.t_[0] == "init [10]");
  REQUIRE(sp.t_[1] == "run [10]");
  REQUIRE(sp.t_[2] == "exit [10]");

  REQUIRE(sp.t_[3] == "init [20]");
  REQUIRE(sp.t_[4] == "run [20]");
  REQUIRE(sp.t_[5] == "exit [20]");

  REQUIRE(sp.t_[6] == "init [30]");
  REQUIRE(sp.t_[7] == "run [30]");
  REQUIRE(sp.t_[8] == "run [30]");
  REQUIRE(sp.t_[9] == "run [30]");
  REQUIRE(sp.t_[10] == "exit [30]");

  REQUIRE(sp.t_[11] == "init [40]");
  REQUIRE(sp.t_[12] == "run [40]");
  REQUIRE(sp.t_[13] == "exit [40]");
}

TEST_CASE("co-routines", "[Execute]") {
  using Variant1 = std::variant<TaskA, TaskB, TaskC, TaskD, TaskE>;

  StateProvider<Variant1> sp;

  auto p = COMPILE_AND_QUEUE(0, "TaskD($0)[TaskE($1)]", sp, STEPWISE_1, 10, 20);

  for (int32_t i = 0; i < 1000; ++i) { EXECUTE_QUEUE(sp) }

  REQUIRE(sp.t_[0] == "co_await start [10]");

  REQUIRE(sp.t_[1] == "co_yield [50]");
  REQUIRE(sp.t_[2] == "co_yield [50]");
  REQUIRE(sp.t_[3] == "co_yield [50]");
  REQUIRE(sp.t_[4] == "exit [50]");

  REQUIRE(sp.t_[5] == "co_await end [10]");
  REQUIRE(sp.t_[6] == "exit [10]");

  REQUIRE(sp.t_[7] == "co_yield [20]");
  REQUIRE(sp.t_[8] == "co_yield [20]");
  REQUIRE(sp.t_[9] == "co_yield [20]");
  REQUIRE(sp.t_[10] == "exit [20]");
}

TEST_CASE("mixed", "[Execute]") {
  using Variant1 = std::variant<TaskA, TaskB, TaskC, TaskD, TaskE>;

  StateProvider<Variant1> sp;

  auto p = COMPILE_AND_QUEUE(0, "TaskA($0)[TaskE($1)]", sp, STEPWISE_1, 10, 20);

  for (int32_t i = 0; i < 1000; ++i) { EXECUTE_QUEUE(sp) }

  REQUIRE(sp.t_[0] == "init [10]");
  REQUIRE(sp.t_[1] == "run [10]");
  REQUIRE(sp.t_[2] == "exit [10]");

  REQUIRE(sp.t_[3] == "co_yield [20]");
  REQUIRE(sp.t_[4] == "co_yield [20]");
  REQUIRE(sp.t_[5] == "co_yield [20]");
  REQUIRE(sp.t_[6] == "exit [20]");
}