// This black macro magic was conjured up by https://github.com/JacksonAllan

#include <TBT/defines.hpp>

// Check for correct use.
#if !defined(TASK_TYPE)
#error Define TASK_TYPE before including magic.hpp
#endif

// If this is the first inclusion of the header, initialize the counter and define all API and helper macros.
#ifndef TASK_DIGIT_1

#define TASK_DIGIT_1 0
#define TASK_DIGIT_2 0
#define TASK_DIGIT_3 0

#define TASK_CAT_2_(a, b) a##b
#define TASK_CAT_2(a, b) TASK_CAT_2_(a, b)
#define TASK_CAT_3_(a, b, c) a##b##c
#define TASK_CAT_3(a, b, c) TASK_CAT_3_(a, b, c)
#define TASK_CAT_4_(a, b, c, d) a##b##c##d
#define TASK_CAT_4(a, b, c, d) TASK_CAT_4_(a, b, c, d)

#define TASK_COUNT TASK_CAT_4(0, TASK_DIGIT_3, TASK_DIGIT_2, TASK_DIGIT_1)

#define TASK_PROBE_0000 ,
#define TASK_ARG_2_(a, b, ...) b
#define TASK_ARG_2(...) TASK_ARG_2_(__VA_ARGS__)
#define TASK_PUT_COMMA_0
#define TASK_PUT_COMMA_1 ,
#define TASK_COMMA_IF_NOT_0000(slot) TASK_CAT_2(TASK_PUT_COMMA_, TASK_ARG_2(TASK_PROBE_##slot 0, 1, ))

#define TASK_SLOT(n) TASK_COMMA_IF_NOT_0000(n) TASK_CAT_3(task_type_, n, _tt)

#define TASK_R1_0(d3, d2)
#define TASK_R1_1(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 0))
#define TASK_R1_2(d3, d2) TASK_R1_1(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 1))
#define TASK_R1_3(d3, d2) TASK_R1_2(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 2))
#define TASK_R1_4(d3, d2) TASK_R1_3(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 3))
#define TASK_R1_5(d3, d2) TASK_R1_4(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 4))
#define TASK_R1_6(d3, d2) TASK_R1_5(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 5))
#define TASK_R1_7(d3, d2) TASK_R1_6(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 6))
#define TASK_R1_8(d3, d2) TASK_R1_7(d3, d2) TASK_SLOT(TASK_CAT_4(0, d3, d2, 7))

#define TASK_R2_0(d3)
#define TASK_R2_1(d3) TASK_R1_8(d3, 0)
#define TASK_R2_2(d3) TASK_R2_1(d3) TASK_R1_8(d3, 1)
#define TASK_R2_3(d3) TASK_R2_2(d3) TASK_R1_8(d3, 2)
#define TASK_R2_4(d3) TASK_R2_3(d3) TASK_R1_8(d3, 3)
#define TASK_R2_5(d3) TASK_R2_4(d3) TASK_R1_8(d3, 4)
#define TASK_R2_6(d3) TASK_R2_5(d3) TASK_R1_8(d3, 5)
#define TASK_R2_7(d3) TASK_R2_6(d3) TASK_R1_8(d3, 6)
#define TASK_R2_8(d3) TASK_R2_7(d3) TASK_R1_8(d3, 7)

#define TASK_R3_0()
#define TASK_R3_1() TASK_R2_8(0)
#define TASK_R3_2() TASK_R3_1() TASK_R2_8(1)
#define TASK_R3_3() TASK_R3_2() TASK_R2_8(2)
#define TASK_R3_4() TASK_R3_3() TASK_R2_8(3)
#define TASK_R3_5() TASK_R3_4() TASK_R2_8(4)
#define TASK_R3_6() TASK_R3_5() TASK_R2_8(5)
#define TASK_R3_7() TASK_R3_6() TASK_R2_8(6)
#define TASK_R3_8() TASK_R3_7() TASK_R2_8(7)

#define TASK_TYPES                                                                      \
  TASK_CAT_2(TASK_R3_, TASK_DIGIT_3)() TASK_CAT_2(TASK_R2_, TASK_DIGIT_2)(TASK_DIGIT_3) \
      TASK_CAT_2(TASK_R1_, TASK_DIGIT_1)(TASK_DIGIT_3, TASK_DIGIT_2)
#endif

// Registering user-provided type.
typedef TASK_TYPE TASK_CAT_3(task_type_, TASK_COUNT, _tt);

namespace TBT {
  ENABLE_TBT_TYPENAME(EEXPAND(TASK_TYPE), SEEXPAND(TASK_TYPE))
}

// For compatability with exotic types (function pointers etc.), instead do something like:
// typedef std::decay<std::remove_reference<decltype( std::declval<TASK_TYPE>() )>::type>::type TASK_CAT_3( task_type_,
// TASK_COUNT, _tt );
#undef TASK_TYPE

// Increment the counter.
#if TASK_DIGIT_1 == 0
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 1
#elif TASK_DIGIT_1 == 1
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 2
#elif TASK_DIGIT_1 == 2
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 3
#elif TASK_DIGIT_1 == 3
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 4
#elif TASK_DIGIT_1 == 4
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 5
#elif TASK_DIGIT_1 == 5
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 6
#elif TASK_DIGIT_1 == 6
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 7
#elif TASK_DIGIT_1 == 7
#undef TASK_DIGIT_1
#define TASK_DIGIT_1 0
#if TASK_DIGIT_2 == 0
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 1
#elif TASK_DIGIT_2 == 1
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 2
#elif TASK_DIGIT_2 == 2
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 3
#elif TASK_DIGIT_2 == 3
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 4
#elif TASK_DIGIT_2 == 4
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 5
#elif TASK_DIGIT_2 == 5
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 6
#elif TASK_DIGIT_2 == 6
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 7
#elif TASK_DIGIT_2 == 7
#undef TASK_DIGIT_2
#define TASK_DIGIT_2 0
#if TASK_DIGIT_3 == 0
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 1
#elif TASK_DIGIT_3 == 1
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 2
#elif TASK_DIGIT_3 == 2
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 3
#elif TASK_DIGIT_3 == 3
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 4
#elif TASK_DIGIT_3 == 4
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 5
#elif TASK_DIGIT_3 == 5
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 6
#elif TASK_DIGIT_3 == 6
#undef TASK_DIGIT_3
#define TASK_DIGIT_3 7
#elif TASK_DIGIT_3 == 7
#error Too many types for typelist!
#endif
#endif
#endif
