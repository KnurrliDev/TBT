#include <TBT/TBT>
#include <TBT/common/defines.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace TBT;
using namespace Util;

TEST_CASE("Util::split function tests", "[split]") {
  SECTION("Static string splitting with char delimiter 1") {
    constexpr std::string_view input = "apple,banana,orange";
    constexpr char delim             = ',';

    constexpr size_t s               = count_elements(input, delim);
    constexpr auto result            = csplit<s>(input, delim);

    static_assert(s == 3);
    static_assert(result[0] == "apple");
    static_assert(result[1] == "banana");
    static_assert(result[2] == "orange");
  }

  SECTION("Static string splitting with char delimiter 2") {
    constexpr std::string_view input = "|||apple|banana|||orange|||";
    // constexpr std::string_view input = "|apple|";
    constexpr char delim             = '|';

    constexpr size_t s               = count_elements(input, delim);
    constexpr auto result            = csplit<s>(input, delim);

    // static_assert(s == 11);
    //  static_assert(result[0] == "apple");
    //  static_assert(result[1] == "banana");
    //  static_assert(result[2] == "orange");
  }

  //   SECTION("Empty string") {
  //     std::string input = "";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 1);
  //     CHECK(part_to_string(result[0]) == "");
  //   }

  //   SECTION("Single segment without delimiter") {
  //     std::string input = "hello";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 1);
  //     CHECK(part_to_string(result[0]) == "hello");
  //   }

  //   SECTION("String with consecutive delimiters") {
  //     std::string input = "a,,b";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 3);
  //     CHECK(part_to_string(result[0]) == "a");
  //     CHECK(part_to_string(result[1]) == "");
  //     CHECK(part_to_string(result[2]) == "b");
  //   }

  //   SECTION("String starting with delimiter") {
  //     std::string input = ",start,middle,end";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 4);
  //     CHECK(part_to_string(result[0]) == "");
  //     CHECK(part_to_string(result[1]) == "start");
  //     CHECK(part_to_string(result[2]) == "middle");
  //     CHECK(part_to_string(result[3]) == "end");
  //   }

  //   SECTION("String ending with delimiter") {
  //     std::string input = "start,middle,end,";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 4);
  //     CHECK(part_to_string(result[0]) == "start");
  //     CHECK(part_to_string(result[1]) == "middle");
  //     CHECK(part_to_string(result[2]) == "end");
  //     CHECK(part_to_string(result[3]) == "");
  //   }

  //   SECTION("Dynamic string splitting with different input") {
  //     std::string input = "1|2|3|4";
  //     auto result       = Util::split(input.begin(), input.end(), '|');

  //     REQUIRE(result.size() == 4);
  //     CHECK(part_to_string(result[0]) == "1");
  //     CHECK(part_to_string(result[1]) == "2");
  //     CHECK(part_to_string(result[2]) == "3");
  //     CHECK(part_to_string(result[3]) == "4");
  //   }

  //   SECTION("Dynamic string with custom delimiter") {
  //     std::string input = "x;y;z";
  //     auto result       = Util::split(input.begin(), input.end(), ';');

  //     REQUIRE(result.size() == 3);
  //     CHECK(part_to_string(result[0]) == "x");
  //     CHECK(part_to_string(result[1]) == "y");
  //     CHECK(part_to_string(result[2]) == "z");
  //   }

  //   SECTION("String with single character") {
  //     std::string input = "a";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 1);
  //     CHECK(part_to_string(result[0]) == "a");
  //   }

  //   SECTION("String with only delimiters") {
  //     std::string input = ",,,";
  //     auto result       = Util::split(input.begin(), input.end(), ',');

  //     REQUIRE(result.size() == 4);
  //     CHECK(part_to_string(result[0]) == "");
  //     CHECK(part_to_string(result[1]) == "");
  //     CHECK(part_to_string(result[2]) == "");
  //     CHECK(part_to_string(result[3]) == "");
  //   }
}