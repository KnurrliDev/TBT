#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/execute/states.hpp>
#include <TBT/glaze/to_tuple.hpp>

namespace TBT {

  namespace Execute {

    // constexpr void write_payload(const int32_t& _i, const std::variant<bool, int32_t, float, uint32_t>& _val,
    //                              const States::NodeHeader& _header, char* _node) {
    //   constexpr int32_t size = sizeof(char) + sizeof(int32_t);
    //   Detail::Payload pl;
    //   char type = 0;
    //   switch (_val.index()) {
    //     case 0: {
    //       type  = (pl_s | pt_bool);
    //       pl.v2 = std::get<0>(_val) ? (int32_t)1 : int32_t(0);
    //       break;
    //     }
    //     case 1: {
    //       type  = (pl_s | pt_int);
    //       pl.v2 = std::get<1>(_val);
    //       break;
    //     }
    //     case 2: {
    //       type  = (pl_s | pt_float);
    //       pl.v3 = std::get<2>(_val);
    //       break;
    //     }
    //     case 3: {
    //       type  = (pl_s | pt_dyn);
    //       pl.v4 = std::get<3>(_val);
    //       break;
    //     }
    //   }
    //   write<char>(type, _node + _header.payload_offset_ + _i * size);
    //   write<Detail::Payload>(pl, _node + _header.payload_offset_ + _i * size + sizeof(char));
    // }  // write_payload

    // namespace Detail {

    // }  // namespace Detail

    // template <class State>
    // States::NodeHeader create_node_header(const int16_t _vidx, const int16_t _children_count,
    //                                       const int16_t _decorator_count, const int16_t _payload_count) {
    //   Execute::States::NodeHeader header;

    //   header.vidx_             = _vidx;
    //   header.parent_           = 0;

    //   // children
    //   header.children_count_   = _children_count;
    //   header.children_offset_  = sizeof(Execute::States::NodeHeader) + sizeof(State);

    //   // decorators
    //   header.decorator_count_  = _decorator_count;
    //   header.decorator_offset_ = header.children_offset_ + header.children_count_ * sizeof(uint32_t);

    //   // payload
    //   header.payload_count_    = _payload_count;
    //   header.payload_offset_   = header.decorator_offset_ + header.decorator_count_ * sizeof(uint32_t);

    //   header.node_size_        = header.payload_offset_ + header.payload_count_ * (sizeof(uint8_t) +
    //   sizeof(uint32_t));

    //   return header;
    // }  // create_node_header

    // uint32_t ptr(const char* _base, const char* _p) { return (uint32_t)(_p - _base); }  // ptr

  }  // namespace Execute

}  // namespace TBT