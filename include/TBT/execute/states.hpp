#pragma once

#include <TBT/common/defines.hpp>

namespace TBT {

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

  // constexpr uint8_t pl_s                = 0b11100000;  // payload start

  constexpr uint8_t is_comp             = 0b10101010;  // is compiled

  using Parameter                       = std::variant<bool, int32_t, float, uint32_t>;

  namespace States {

    struct NoState {};

#pragma pack(push, 1)
    struct Composite {
      uintptr_t ptr_ = 0;
      int16_t cur_idx_;
    };  // Composite

    struct Result {
      State state_;
      Direction dir_;
    };  // Result

    struct Header {
      uint32_t node_count_;
      uint32_t ptr_;

      uint16_t children_count_    = 0;
      uint32_t first_node_offset_ = 0;
      // Result last_result_;
    };  // Header

    struct NodeHeader {
      int16_t type_idx_         = 0;
      int32_t parent_           = -1;

      uint32_t children_offset_ = 0;
      uint16_t children_count_  = 0;

      uint32_t params_offset_   = 0;
      uint16_t params_count_    = 0;

      uint32_t comp_offset_     = 0;

      uint32_t node_size_       = 0;
    };  // NodeHeader

#pragma pack(pop)

  }  // namespace States

}  // namespace TBT