#pragma once

#include <TBT/common/defines.hpp>

namespace TBT {

  namespace Execute {

    constexpr int16_t vidx_root           = -1;
    constexpr int16_t vidx_sequence       = -2;
    constexpr int16_t vidx_fallback       = -3;
    constexpr int16_t vidx_nloop          = -4;
    constexpr int16_t vidx_cooldown       = -5;
    constexpr int16_t vidx_always_fail    = -6;
    constexpr int16_t vidx_always_succeed = -7;

    constexpr uint8_t pt_bool             = 0b00000001;  // bool type
    constexpr uint8_t pt_int              = 0b00000010;  // int type
    constexpr uint8_t pt_float            = 0b00000100;  // float type
    constexpr uint8_t pt_dyn              = 0b00001000;  // dynamic type

    constexpr uint8_t pl_s                = 0b11100000;  // payload start

    constexpr uint8_t is_comp             = 0b10101010;  // is compiled

    namespace States {

      struct NoState {};

      struct Task {
        intptr_t ptr_ = 0;
      };

      struct Composite {
        int16_t cur_idx_;
      };  // Composite

      struct NLoop {
        int16_t cur_it_;
        uint32_t max_it_;
        bool break_on_fail_;
        bool inf_loop_;
      };  // NLoop

      struct Cooldown {
        bool is_first_;
        uint64_t start_;
        uint32_t dt_;
      };  // Cooldown

      struct Result {
        State state_;
        Direction dir_;
      };  // Result

      struct Header {
        uint32_t node_count_;
        uint32_t ptr_;
        Result last_result_;
      };  // Header

      struct NodeHeader {
        int16_t vidx_;
        uint32_t parent_;

        uint32_t decorator_offset_;
        uint16_t decorator_count_;
        uint32_t payload_offset_;
        uint16_t payload_count_;
        uint32_t children_offset_;
        uint16_t children_count_;

        uint32_t node_size_;
      };  // NodeHeader

    }  // namespace States

  }  // namespace Execute
}  // namespace TBT