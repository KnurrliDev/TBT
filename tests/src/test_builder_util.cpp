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

  const std::array<std::pair<int32_t, std::string_view>, 4> variant = {
      std::make_pair<int32_t, std::string_view>(-1, "root"), std::make_pair<int32_t, std::string_view>(-2, "sequence"),
      std::make_pair<int32_t, std::string_view>(-3, "fallback"), std::make_pair<int32_t, std::string_view>(1, "Task")};

  const auto h_extract = [&](const std::string_view& _s) -> std::vector<Node> {
    std::vector<int32_t> dd;

    int32_t temp_task_id = 1;

    int32_t level        = 0;
    bool in_param        = false;

    std::vector<std::pair<int32_t, int32_t>> stack;
    stack.push_back({0, 0});

    const auto get_idx_for_type = [&](const std::string_view& _ss) -> std::optional<int32_t> {
      const auto t = _ss.substr(0, _ss.find_first_of('('));
      for (size_t i = 0; i < variant.size(); ++i)
        if (variant[i].second == t) return variant[i].first;
      return std::nullopt;
    };

    /*
      Grammer:
        Hierarchy: Task($n, ...)[Task($n, ...), ...]
          special node: root if no custom root node
                        root[Task($n, ...)[Task($n, ...), ...], Task($n, ...)]

        Behaviour Tree: sequence[Task($n, ...), Task($n, ...)]

        State Machine: $[$n: Task($n, ...), ..., $n0->$n1:Task($n, ...), ...]
          $n:     State
          $n->$n: transition

        $[State Machine]
        sequence[BT]

    */

    /* valid ends: 'Task,', 'Task[' 'Task]', 'Task],' */
    std::vector<Node> nodes;
    int32_t idd = 1;
    for (size_t cur_s = 0; cur_s < _s.size(); ++cur_s) { /* find end of current node */
      /* special case: sm */
      if (_s[cur_s] == '$' && _s[cur_s + 1] == '[') {
        const auto st_cl = clause({&_s[cur_s + 1], _s.size() - cur_s - 1}, '[', ']');
        nodes.emplace_back(idd++, stack.back().first, stack.back().second,
                           std::string_view{&_s[cur_s + 2], st_cl.size()}, 2);
        cur_s = cur_s + 2 + st_cl.size();
        if (cur_s + 1 < _s.size() && _s[cur_s + 1] == ',') { ++cur_s; }
        continue;
      }
      for (size_t cur_e = cur_s + 1; cur_e < _s.size(); ++cur_e) { /* if we have reached the end */
        bool next = false;
        if (cur_e == _s.size() - 1 && _s[cur_e] != ']') {
          nodes.emplace_back(idd++, stack.back().first, stack.back().second,
                             std::string_view{&_s[cur_s], cur_e - cur_s + 1});
          cur_s = _s.size();
          next  = true;
        }
        if (!next) {
          switch (_s[cur_e]) {
            case '[':
              nodes.emplace_back(idd++, stack.back().first, stack.back().second,
                                 std::string_view{&_s[cur_s], cur_e - cur_s});
              stack.push_back({stack.back().first + 1, idd - 1});
              next = true;
              break;
            case ']':
              if (cur_e - 1 >= 0 && _s[cur_e - 1] != ']')
                nodes.emplace_back(idd++, stack.back().first, stack.back().second,
                                   std::string_view{&_s[cur_s], cur_e - cur_s});
              stack.pop_back();
              next = true;
              if (cur_e + 1 < _s.size() && _s[cur_e + 1] == ',') { ++cur_e; }
              break;
            case ',': /* set level and parent */
              if (!in_param) {
                nodes.emplace_back(idd++, stack.back().first, stack.back().second,
                                   std::string_view{&_s[cur_s], cur_e - cur_s});
                next = true;
              }
              break;
            case '(':
              in_param = true;
              break;
            case ')':
              in_param = false;
              break;
          }
        }
        if (next) { /* last node is a bt */
          if (nodes.back().n_ == "root" || nodes.back().n_ == "sequence" ||
              nodes.back().n_ == "fallback") { /* bt isnt a single node */
            nodes.back().n_type = 1;
            stack.pop_back();
            nodes.back().idx_ = get_idx_for_type(nodes.back().n_).value();
            if (_s[cur_e] == '[') {
              nodes.back().n_ = clause({&_s[cur_e], _s.size() - cur_e}, '[', ']');
              cur_e           = cur_e + nodes.back().n_.size() + 1;
              if (cur_e + 1 < _s.size() && _s[cur_e + 1] == ',') { ++cur_e; }
            }
          }
          cur_s = cur_e;
          break;
        }
      }
    }

    std::vector<Node> out;
    for (const auto& n : nodes) {
      Node nn;
      nn.id_     = n.id_;
      nn.level_  = n.level_;
      nn.parent_ = n.parent_;
      nn.cl_     = n.n_;
      nn.type_   = n.n_type == 1 ? NodeType::BT : n.n_type == 2 ? NodeType::SM : NodeType::H;
      if (nn.type_ == NodeType::H) {
        const auto idx = get_idx_for_type(n.n_);
        nn.idx_        = idx.value(); /* todo assert */
        const auto cl  = clause(n.n_, '(', ')');
        if (!cl.empty()) { nn.p_ = extract_params(cl); }
      } else if (nn.type_ == NodeType::BT) {
        nn.idx_ = n.idx_;
      }
      out.push_back(std::move(nn));
    }

    return out;
  };

  SECTION("Pure hierarchy") {
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

  SECTION("two children nested hierarchy") {
    const std::string input   = "Task[Task[Task[Task]]]";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 4);

    for (int32_t i = 0; i < 4; ++i) {
      REQUIRE(h[i].id_ == i + 1);
      REQUIRE(h[i].idx_ == 1);
      REQUIRE(h[i].level_ == i);
      REQUIRE(h[i].parent_ == i);
      REQUIRE(h[i].p_.empty());
      REQUIRE(h[i].cl_ == "Task");
    }
  }

  SECTION("Deep hierarchy") {
    const std::string input =
        "Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task[Task]]]]]]]]]]]]]]]]";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 17);

    for (int32_t i = 0; i < 16; ++i) {
      REQUIRE(h[i].id_ == i + 1);
      REQUIRE(h[i].idx_ == 1);
      REQUIRE(h[i].level_ == i);
      REQUIRE(h[i].parent_ == i);
      REQUIRE(h[i].p_.empty());
      REQUIRE(h[i].cl_ == "Task");
    }
  }

  SECTION("ST at start hierarchy") {
    const std::string input   = "$[...], Task";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 2);

    REQUIRE(h[0].cl_ == "...");
    REQUIRE(h[0].type_ == NodeType::SM);
  }

  SECTION("ST mixed hierarchy") {
    const std::string input   = "Task, $[...] Task";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 3);

    REQUIRE(h[1].cl_ == "...");
    REQUIRE(h[1].type_ == NodeType::SM);
  }

  SECTION("BT at start hierarchy") {
    const std::string input   = "sequence[...], root[...] fallback[...]";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 3);

    REQUIRE(h[0].idx_ == -2);
    REQUIRE(h[0].cl_ == "...");
    REQUIRE(h[0].type_ == NodeType::BT);

    REQUIRE(h[1].idx_ == -1);
    REQUIRE(h[1].cl_ == "...");
    REQUIRE(h[1].type_ == NodeType::BT);

    REQUIRE(h[2].idx_ == -3);
    REQUIRE(h[2].cl_ == "...");
    REQUIRE(h[2].type_ == NodeType::BT);
  }

  SECTION("Mixed hierarchy") {
    const std::string input =
        "Task(true), sequence[...] Task(true, true)[ Task[ Task(4.2)], Task, fallback[...], Task(true)] $[...] Task, "
        "root";
    const std::string res     = clean(input);

    const std::vector<Node> h = h_extract({res.data(), res.size()});

    REQUIRE(h.size() == 11);

    // sequence
    REQUIRE(h[1].id_ == 2);
    REQUIRE(h[1].type_ == NodeType::BT);
    REQUIRE(h[1].idx_ == -2);
    REQUIRE(h[1].cl_ == "...");

    // fallback
    REQUIRE(h[6].id_ == 7);
    REQUIRE(h[6].type_ == NodeType::BT);
    REQUIRE(h[6].idx_ == -3);
    REQUIRE(h[6].cl_ == "...");

    // sm
    REQUIRE(h[8].id_ == 9);
    REQUIRE(h[8].type_ == NodeType::SM);
    REQUIRE(h[8].cl_ == "...");

    // root
    REQUIRE(h[10].id_ == 11);
    REQUIRE(h[10].type_ == NodeType::BT);
    REQUIRE(h[10].idx_ == -1);
  }
}

// TEST_CASE("extract", "[Behaviour Tree]") {
//   F_SPLIT
//   F_CLEAN
//   F_CLAUSE
//   F_PARAMS
//   H_COUNT_NODES

//   const std::array<std::pair<int32_t, std::string_view>, 4> variant = {
//       std::make_pair<int32_t, std::string_view>(-1, "root"), std::make_pair<int32_t, std::string_view>(-2,
//       "sequence"), std::make_pair<int32_t, std::string_view>(-3, "fallback"), std::make_pair<int32_t,
//       std::string_view>(1, "Task")};

//   H_EXTRACT

//   const auto bt_extract = [&](const std::string_view& _s) -> std::vector<Node> {
//     //
//   };
// }

// struct TBT_Test {
//   constexpr static auto tree_ = build<estimate_size("sss")>("sss");
// };  // TBT_Test

/*

  #define Compile(id, tree)
    struct TBT_##id {
      constexpr static auto func_ = TBT::build<TBT::estimate_size(tree)>(tree);
    };

*/
