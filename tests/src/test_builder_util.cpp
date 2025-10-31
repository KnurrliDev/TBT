#include <TBT/TBT>
#include <TBT/common/defines.hpp>
#include <catch2/catch_test_macros.hpp>

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

  const std::array<std::pair<int32_t, std::string_view>, 1> variant = {
      std::make_pair<int32_t, std::string_view>(1, "Task")};

  H_EXTRACT

  const std::string input   = "Task(true), Task(true, true)[ Task[ Task(4.2)], Task, Task(true)] Task";
  const std::string res     = clean(input);

  const std::vector<Node> h = h_extract({res.data(), res.size()});

  REQUIRE(h.size() == 7);

  REQUIRE(h[0].id_ == 1);
  REQUIRE(h[0].idx_ == 1);
  REQUIRE(h[0].level_ == 0);
  REQUIRE(h[0].parent_ == 0);
  REQUIRE(h[0].p_.size() == 1);

  REQUIRE(h[1].id_ == 2);
  REQUIRE(h[1].idx_ == 1);
  REQUIRE(h[1].level_ == 0);
  REQUIRE(h[1].parent_ == 0);
  REQUIRE(h[1].p_.size() == 2);

  REQUIRE(h[2].id_ == 3);
  REQUIRE(h[2].idx_ == 1);
  REQUIRE(h[2].level_ == 1);
  REQUIRE(h[2].parent_ == 2);
  REQUIRE(h[2].p_.size() == 0);

  REQUIRE(h[3].id_ == 4);
  REQUIRE(h[3].idx_ == 1);
  REQUIRE(h[3].level_ == 2);
  REQUIRE(h[3].parent_ == 3);
  REQUIRE(h[3].p_.size() == 1);

  REQUIRE(h[4].id_ == 5);
  REQUIRE(h[4].idx_ == 1);
  REQUIRE(h[4].level_ == 1);
  REQUIRE(h[4].parent_ == 2);
  REQUIRE(h[4].p_.size() == 0);

  REQUIRE(h[5].id_ == 6);
  REQUIRE(h[5].idx_ == 1);
  REQUIRE(h[5].level_ == 1);
  REQUIRE(h[5].parent_ == 2);
  REQUIRE(h[5].p_.size() == 1);

  REQUIRE(h[6].id_ == 7);
  REQUIRE(h[6].idx_ == 1);
  REQUIRE(h[6].level_ == 0);
  REQUIRE(h[6].parent_ == 0);
  REQUIRE(h[6].p_.size() == 0);
}

// struct TBT_Test {
//   constexpr static auto tree_ = build<estimate_size("sss")>("sss");
// };  // TBT_Test

/*

  #define Compile(id, tree)
    struct TBT_##id {
      constexpr static auto func_ = TBT::build<TBT::estimate_size(tree)>(tree);
    };

*/
