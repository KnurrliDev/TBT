#include <TBT/TBT>
#include <TBT/common/defines.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace TBT;

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

TEST_CASE("clause extraction", "[util]") {
  F_CLAUSE

  SECTION("no clause") {
    constexpr std::string_view input = "apple";
    constexpr char open_delim        = '<';
    constexpr char close_delim       = '>';

    const auto result                = clause(input, open_delim, close_delim);

    REQUIRE(result.empty());
  }

  SECTION("single clause") {
    constexpr std::string_view input = "<apple>";
    constexpr char open_delim        = '<';
    constexpr char close_delim       = '>';

    const auto result                = clause(input, open_delim, close_delim);

    REQUIRE(result == "apple");
  }

  SECTION("multi clause") {
    constexpr std::string_view input = "<orange<apple>banana>";
    constexpr char open_delim        = '<';
    constexpr char close_delim       = '>';

    const auto result                = clause(input, open_delim, close_delim);

    REQUIRE(result == "orange<apple>banana");
  }

  SECTION("multi brackets") {
    constexpr std::string_view input = "<<orange<<<apple>>>banana>>";
    constexpr char open_delim        = '<';
    constexpr char close_delim       = '>';

    const auto result                = clause(input, open_delim, close_delim);

    REQUIRE(result == "<orange<<<apple>>>banana>");
  }

  SECTION("missing brackets") {
    constexpr std::string_view input = "<<<orange<<<apple>>>banana>>";
    constexpr char open_delim        = '<';
    constexpr char close_delim       = '>';

    constexpr auto result            = clause(input, open_delim, close_delim);

    REQUIRE(result == "<<orange<<<apple>>>banana>>");
  }
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

TEST_CASE("count nodes", "[Hierarchy]") {
  F_CLEAN
  H_COUNT_NODES

  SECTION("count with task at end") {
    const std::string input = "Task(true), Task(true, true)[Task, Task, Task(true)] Task";
    const std::string res   = clean(input);

    const size_t h          = h_count_nodes({res.data(), res.size()});

    REQUIRE(h == 6);
  }

  SECTION("count with ] at end") {
    const std::string input = "Task(true), Task(true, true)[Task, Task, Task(true)] ";
    const std::string res   = clean(input);

    const size_t h          = h_count_nodes({res.data(), res.size()});

    REQUIRE(h == 5);
  }

  SECTION("count with ],") {
    const std::string input = "Task(true), Task(true, true)[Task, Task, Task(true)], Task";
    const std::string res   = clean(input);

    const size_t h          = h_count_nodes({res.data(), res.size()});

    REQUIRE(h == 6);
  }
}

TEST_CASE("count params", "[Hierarchy]") {
  F_CLEAN
  H_COUNT_PARAMS

  const std::string input = "Task(true), Task(true, true)[Task, Task, Task(true)] Task";
  const std::string res   = clean(input);

  const size_t h          = h_count_params({res.data(), res.size()});

  REQUIRE(h == 4);
}

TEST_CASE("node list", "[Hierarchy]") {
  F_SPLIT
  F_CLEAN
  F_CLAUSE
  F_PARAMS
  H_COUNT_NODES

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
      REQUIRE(h[i].parent_ == i);
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
      REQUIRE(h[i].parent_ == i);
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
      F_CLAUSE
      F_PARAMS
      H_COUNT_NODES

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

struct TaskA {};
ENABLE_TBT_TYPENAME(TaskA, "TaskA")

struct TaskB {};
ENABLE_TBT_TYPENAME(TaskB, "TaskB")

struct TaskC {};
ENABLE_TBT_TYPENAME(TaskC, "TaskC")

template <typename Variant>
consteval auto variant_type_index_name_pairs() {
  using V            = std::remove_cvref_t<Variant>;
  constexpr size_t N = std::variant_size_v<V>;
  return []<size_t... I>(std::index_sequence<I...>) consteval {
    using tuple_t = std::pair<size_t, std::string_view>;
    return std::array<tuple_t, N>{{tuple_t{I, Detail::TypeName<std::variant_alternative_t<I, V>>::Get()}...}};
  }(std::make_index_sequence<N>{});
}  // variant_type_index_name_pairs

template <class Variant>
consteval size_t compute_size_static(const std::string_view& _s) {
  F_SPLIT
  F_CLEAN
  F_CLAUSE
  F_PARAMS

  constexpr auto variant = variant_type_index_name_pairs<Variant>();

  H_EXTRACT

  const auto gather_children = [](const int32_t _parent, const std::vector<Node>& _nodes) -> std::vector<int32_t> {
    std::vector<int32_t> out;
    for (const auto& n : _nodes)
      if (n.parent_ == _parent) out.push_back(n.node_id_);
    return out;
  };

  // clean input
  const std::string s = [&]() {
    std::string out;
    out.resize(_s.size());
    for (size_t i = 0; i < _s.size(); ++i) out[i] = _s[i];
    return clean(out);
  }();

  // extract nodes
  const auto hh    = h_extract({s.data(), s.size()});

  const auto nodes = hh.value();

  //----------------------------------------------------
  // ...|header|node header|children|params|composite|...

  size_t out       = sizeof(States::Header);

  for (const Node& n : nodes) {
    const auto children = gather_children(n.node_id_, nodes);
    out += (uint32_t)sizeof(States::NodeHeader) + (uint16_t)children.size() * sizeof(int32_t) +
           (uint16_t)n.p_.size() * (1 + sizeof(int32_t)) + sizeof(States::Composite);
  }

  return out;
};  // compute_size_static

template <size_t S, class Variant>
consteval std::array<char, S> flatten_static(const std::string_view& _s) {
  F_SPLIT
  F_CLEAN
  F_CLAUSE
  F_PARAMS

  constexpr auto variant = variant_type_index_name_pairs<Variant>();

  H_EXTRACT

  const auto gather_children = [](const int32_t _parent, const std::vector<Node>& _nodes) -> std::vector<int32_t> {
    std::vector<int32_t> out;
    for (const auto& n : _nodes)
      if (n.parent_ == _parent) out.push_back(n.node_id_);
    return out;
  };

  // clean input
  const std::string s = [&]() {
    std::string out;
    out.resize(_s.size());
    for (size_t i = 0; i < _s.size(); ++i) out[i] = _s[i];
    return clean(out);
  }();

  // extract nodes
  const auto hh = h_extract({s.data(), s.size()});

  auto nodes    = hh.value();

  //----------------------------------------------------
  // ...|header|node header|children|params|composite|...

  std::array<char, S> out;
  for (size_t i = 0; i < S; ++i) out[i] = 0x0;

  States::Header header;
  header.node_count_ = nodes.size();
  header.ptr_        = 0;

  auto ha            = std::bit_cast<std::array<char, sizeof(States::Header)>>(header);
  for (size_t i = 0; i < sizeof(States::Header); ++i) out[i] = ha[i];

  uint32_t ptr = sizeof(States::Header);
  for (Node& n : nodes) {
    States::NodeHeader header;
    header.type_idx_        = n.type_idx_;
    header.parent_          = n.parent_;

    const auto children     = gather_children(n.node_id_, nodes);
    header.children_count_  = (uint16_t)children.size();
    header.children_offset_ = ptr + (uint32_t)sizeof(States::NodeHeader);

    header.params_count_    = (uint16_t)n.p_.size();
    header.params_offset_   = header.children_offset_ + header.children_count_ * sizeof(int32_t);

    header.comp_offset_     = header.params_offset_ + header.params_count_ * (1 + sizeof(int32_t));

    header.node_size_       = header.comp_offset_ + sizeof(States::Composite) - ptr;

    n.offset_               = ptr;
    n.size_                 = header.node_size_;

    // write header
    const auto nha          = std::bit_cast<std::array<char, sizeof(States::NodeHeader)>>(header);
    for (size_t i = 0; i < sizeof(States::NodeHeader); ++i) out[ptr + i] = nha[i];

    // write params (5 bytes per param)
    for (size_t i = 0; i < n.p_.size(); ++i) {
      const auto& p       = n.p_[i];
      const size_t offset = header.params_offset_ + i * (1 + sizeof(int32_t));
      std::visit(overloaded(
                     [&](const bool _p) {
                       const auto pa   = std::bit_cast<std::array<char, sizeof(int32_t)>>((int32_t)_p);
                       out[offset]     = pt_bool;
                       out[offset + 1] = pa[0];
                       for (int32_t i = 1; i < sizeof(int32_t); ++i) out[offset + 1 + i] = 0x0;
                     },
                     [&](const int32_t _p) {
                       const auto pa = std::bit_cast<std::array<char, sizeof(int32_t)>>(_p);
                       out[offset]   = pt_int;
                       for (int32_t i = 0; i < sizeof(int32_t); ++i) out[offset + 1 + i] = pa[i];
                     },
                     [&](const float _p) {
                       const auto pa = std::bit_cast<std::array<char, sizeof(float)>>(_p);
                       out[offset]   = pt_float;
                       for (int32_t i = 0; i < sizeof(float); ++i) out[offset + 1 + i] = pa[i];
                     },
                     [&](const uint32_t _p) {
                       const auto pa = std::bit_cast<std::array<char, sizeof(uint32_t)>>(_p);
                       out[offset]   = pt_dyn;
                       for (int32_t i = 0; i < sizeof(uint32_t); ++i) out[offset + 1 + i] = pa[i];
                     }),
                 p);
    }

    States::Composite cmp;
    cmp.cur_idx_ = 0;
    cmp.ptr_     = 0;

    auto cmpa    = std::bit_cast<std::array<char, sizeof(States::Composite)>>(cmp);
    for (size_t i = 0; i < sizeof(States::Composite); ++i) out[header.comp_offset_ + i] = cmpa[i];

    ptr += header.node_size_;
  }

  // link children
  for (const Node& n : nodes) {
    const uint32_t c_ptr = n.offset_ + (uint32_t)sizeof(States::NodeHeader);

    const auto children  = gather_children(n.node_id_, nodes);
    for (size_t i = 0; i < children.size(); ++i) {
      for (const auto& nc : nodes) {
        if (nc.node_id_ == children[i]) {
          // write children (4 bytes per child)
          for (size_t i = 0; i < children.size(); ++i) {
            const auto ca = std::bit_cast<std::array<char, sizeof(int32_t)>>(nc.offset_);
            for (int32_t i = 0; i < sizeof(int32_t); ++i) out[c_ptr + i * sizeof(int32_t)] = ca[i];
          }
          break;
        }
      }
    }
  }

  return out;
};

TEST_CASE("static extract node list", "[Hierarchy]") {
  using Variant                = std::variant<TaskA, TaskB, TaskC>;

  constexpr auto vr            = variant_type_index_name_pairs<Variant>();

  constexpr std::string_view s = "TaskA[TaskB(4.2), TaskC(true, $2)] TaskA";
  constexpr size_t r_size      = compute_size_static<Variant>(s);
  constexpr auto res           = flatten_static<r_size, Variant>(s);
}

// TEST_CASE("static extract node list", "[Hierarchy]") {
//   constexpr std::string_view input                                      = "Task";

//   constexpr std::array<std::pair<int32_t, std::string_view>, 4> variant = {
//       std::make_pair<int32_t, std::string_view>(1, "Task")};

//   constexpr auto build
// }