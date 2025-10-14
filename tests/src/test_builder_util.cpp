#include <TBT/TBT>
#include <TBT/common/defines.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace TBT;
using namespace Util;

TEST_CASE("Static string clean", "[util]") {
  constexpr sString input = ss_from_literal(R"(   apple,  
    
    banana  , orange)");
  constexpr sString res   = ss_clean(input);

  static_assert(res == ss_from_literal("apple,banana,orange"));
}

TEST_CASE("Dynamic clean", "[util]") {
  const sString input   = ss_from_literal(R"(   apple,  
    
    banana  , orange)");
  const sString cleaned = ss_clean(input);

  REQUIRE(cleaned == ss_from_literal("appleappleappleappleapple"));
}

TEST_CASE("Static string splitting", "[util]") {
  SECTION("no empty parts") {
    constexpr sString input = ss_from_literal("apple,banana,orange");
    constexpr char delim    = ',';

    constexpr size_t s      = c_count_elements(input, {0, input.size_}, delim);
    constexpr auto result   = c_split<s>(input, {0, input.size_}, delim);

    static_assert(s == 3);

    static_assert(result[0]);
    static_assert(result[0].value().first == 0);
    static_assert(result[0].value().second == 5);
    static_assert(equals(*result[0], input, ss_from_literal("apple")));

    static_assert(result[1]);
    static_assert(result[1].value().first == 6);
    static_assert(result[1].value().second == 12);
    static_assert(equals(*result[1], input, ss_from_literal("banana")));

    static_assert(result[2]);
    static_assert(result[2].value().first == 13);
    static_assert(result[2].value().second == 19);
    static_assert(equals(*result[2], input, ss_from_literal("orange")));
  }

  SECTION("string with empty parts") {
    constexpr sString input = ss_from_literal("|||apple|banana|||orange|||");
    constexpr char delim    = '|';

    constexpr size_t s      = c_count_elements(input, {0, input.size_}, delim);
    constexpr auto result   = c_split<s>(input, {0, input.size_}, delim);

    static_assert(s == 11);

    static_assert(!result[0]);
    static_assert(!result[1]);
    static_assert(!result[2]);

    static_assert(result[3]);
    static_assert(result[3].value().first == 3);
    static_assert(result[3].value().second == 8);
    static_assert(equals(*result[3], input, ss_from_literal("apple")));

    static_assert(result[4]);
    static_assert(result[4].value().first == 9);
    static_assert(result[4].value().second == 15);
    static_assert(equals(*result[4], input, ss_from_literal("banana")));

    static_assert(!result[5]);
    static_assert(!result[6]);

    static_assert(result[7]);
    static_assert(result[7].value().first == 18);
    static_assert(result[7].value().second == 24);
    static_assert(equals(*result[7], input, ss_from_literal("orange")));

    static_assert(!result[8]);
    static_assert(!result[9]);
    static_assert(!result[10]);
  }

  SECTION("Empty string") {
    constexpr sString input = ss_from_literal("");
    constexpr char delim    = '|';

    constexpr size_t s      = c_count_elements(input, {0, input.size_}, delim);
    constexpr auto result   = c_split<s>(input, {0, input.size_}, delim);

    static_assert(s == 1);
    static_assert(!result[0]);
  }

  SECTION("Single segment without delimiter") {
    constexpr sString input = ss_from_literal("apple");
    constexpr char delim    = '|';

    constexpr size_t s      = c_count_elements(input, {0, input.size_}, delim);
    constexpr auto result   = c_split<s>(input, {0, input.size_}, delim);

    static_assert(s == 1);

    static_assert(result[0]);
    static_assert(result[0].value().first == 0);
    static_assert(result[0].value().second == 5);
    static_assert(equals(*result[0], input, ss_from_literal("apple")));
  }

  SECTION("Only empty parts") {
    constexpr sString input = ss_from_literal("||");
    constexpr char delim    = '|';

    constexpr size_t s      = c_count_elements(input, {0, input.size_}, delim);
    constexpr auto result   = c_split<s>(input, {0, input.size_}, delim);

    static_assert(s == 3);
    static_assert(!result[0]);
    static_assert(!result[1]);
    static_assert(!result[2]);
  }

  SECTION("Sub string") {
    constexpr sString input = ss_from_literal("apple,banana,orange");
    constexpr char delim    = ',';

    constexpr size_t s      = c_count_elements(input, {0, 12}, delim);
    constexpr auto result   = c_split<s>(input, {0, 12}, delim);

    static_assert(s == 2);

    static_assert(result[0]);
    static_assert(result[0].value().first == 0);
    static_assert(result[0].value().second == 5);
    static_assert(equals(*result[0], input, ss_from_literal("apple")));

    static_assert(result[1]);
    static_assert(result[1].value().first == 6);
    static_assert(result[1].value().second == 11);
    static_assert(equals(*result[1], input, ss_from_literal("banana")));
  }
}

TEST_CASE("Static clause extraction", "[util]") {
  SECTION("single clause") {
    constexpr auto input       = ss_from_literal("<apple>");
    constexpr char open_delim  = '<';
    constexpr char close_delim = '>';

    constexpr auto result      = c_clause(input, {0, input.size_}, open_delim, close_delim);

    static_assert(result);
    static_assert(result.value().first == 1);
    static_assert(result.value().second == 6);
    static_assert(equals(*result, input, ss_from_literal("apple")));
  }

  SECTION("multi clause") {
    constexpr auto input       = ss_from_literal("<orange<apple>banana>");
    constexpr char open_delim  = '<';
    constexpr char close_delim = '>';

    constexpr auto result      = c_clause(input, {0, input.size_}, open_delim, close_delim);

    static_assert(result);
    static_assert(result.value().first == 1);
    static_assert(result.value().second == 20);
    static_assert(equals(*result, input, ss_from_literal("orange<apple>banana")));
  }

  SECTION("multi brackets") {
    constexpr auto input       = ss_from_literal("<<orange<<<apple>>>banana>>");
    constexpr char open_delim  = '<';
    constexpr char close_delim = '>';

    constexpr auto result      = c_clause(input, {0, input.size_}, open_delim, close_delim);

    static_assert(result);
    static_assert(result.value().first == 1);
    static_assert(result.value().second == 26);
    static_assert(equals(*result, input, ss_from_literal("<orange<<<apple>>>banana>")));
  }

  SECTION("missing brackets") {
    constexpr auto input       = ss_from_literal("<<<orange<<<apple>>>banana>>");
    constexpr char open_delim  = '<';
    constexpr char close_delim = '>';

    constexpr auto result      = c_clause(input, {0, input.size_}, open_delim, close_delim);

    static_assert(result);
    static_assert(result.value().first == 1);
    static_assert(result.value().second == 29);
    static_assert(equals(*result, input, ss_from_literal("<<orange<<<apple>>>banana>>")));
  }
}

TEST_CASE("Static parameter extraction", "[util]") {
  SECTION("simple pack") {
    constexpr auto input                      = ss_from_literal("4.3,-12,false,$2,true");

    constexpr size_t s                        = c_count_elements(input, {0, input.size_}, ',');
    constexpr std::array<Parameter, s> result = c_extract_parameters<s>(input, {0, input.size_});

    static_assert(s == 5);
    static_assert(std::get<float>(result[0]) == 4.3f);
    static_assert(std::get<int32_t>(result[1]) == -12);
    static_assert(std::get<bool>(result[2]) == false);
    static_assert(std::get<uint32_t>(result[3]) == 2);
    static_assert(std::get<bool>(result[4]) == true);
  }
}

TEST_CASE("node list", "[Hierarchy]") {
  SECTION("count with ]") {
    constexpr auto input  = ss_from_literal("Task(true), Task(true, true)[Task, Task, Task(true)] Task");
    constexpr sString res = ss_clean(input);

    constexpr size_t s    = H::c_count(res, {0, res.size_});

    static_assert(s == 6);
  }

  SECTION("count with ],") {
    constexpr auto input  = ss_from_literal("Task(true), Task(true, true)[Task, Task, Task(true)], Task");
    constexpr sString res = ss_clean(input);

    constexpr size_t s    = H::c_count(res, {0, res.size_});

    static_assert(s == 6);
  }

  SECTION("count params") {
    constexpr auto input  = ss_from_literal("Task(true), Task(true, true)[Task, Task, Task(true)], Task");
    constexpr sString res = ss_clean(input);

    constexpr size_t p    = H::c_count_params(res, {0, res.size_});

    static_assert(p == 4);
  }

  SECTION("count params empty") {
    constexpr auto input  = ss_from_literal("Task(), Task[Task(), Task, Task], Task()");
    constexpr sString res = ss_clean(input);

    constexpr size_t p    = H::c_count_params(res, {0, res.size_});

    static_assert(p == 0);
  }

  SECTION("extract") {
    constexpr auto input  = ss_from_literal("Task(true), Task(true, true)[Task, Task, Task(true)], Task");
    constexpr sString res = ss_clean(input);

    constexpr size_t s    = H::c_count(res, {0, res.size_});
    constexpr size_t p    = H::c_count_params(res, {0, res.size_});

    constexpr auto list   = H::c_extract<s, p>(res, {0, res.size_});
  }
}
