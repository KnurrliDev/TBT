#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/glaze/to_tuple.hpp>

namespace TBT {

  namespace Execute {

    template <class T, class U>
    T read(const U* _ptr) {
      T result;
      static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
      memcpy((void*)&result, (void*)_ptr, sizeof(T));
      return result;
    }  // read

    template <class T, class U>
    void write(const T& _val, U* _ptr) {
      static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
      memcpy((void*)_ptr, (void*)&_val, sizeof(T));
    }  // read

    inline States::NodeHeader read_node_header(const char* _node) {
      return read<States::NodeHeader>(_node);
    }  // read_node_header

    inline void write_global_node_header(const States::Header& _val, char* _node) {
      write(_val, _node);
    }  // write_global_node_header

    inline States::Header read_global_node_header(const char* _node) {
      return read<States::Header>(_node);
    }  // read_global_node_header

    inline void write_node_header(const States::NodeHeader& _val, char* _node) {
      write(_val, _node);
    }  // read_node_header

    template <class T>
    T read_node_state(const char* _node) {
      return read<T>(_node + sizeof(States::NodeHeader));
    }  // read_states

    template <class T>
    void write_node_state(const T& _state, char* _node) {
      write<T>(_state, _node + sizeof(States::NodeHeader));
    }  // write_states

    inline uint32_t read_child(const int32_t& _i, const States::NodeHeader& _header, const char* _node) {
      return read<uint32_t>(_node + _header.children_offset_ + _i * sizeof(uint32_t));
    }  // read_child

    inline void write_child(const int32_t& _i, const uint32_t _val, const States::NodeHeader& _header, char* _node) {
      write<uint32_t>(_val, _node + _header.children_offset_ + _i * sizeof(uint32_t));
    }  // read_child

    inline uint32_t read_decorator(const int32_t& _i, const States::NodeHeader& _header, const char* _node) {
      return read<uint32_t>(_node + _header.decorator_offset_ + _i * sizeof(uint32_t));
    }  // read_decorator

    inline void write_decorator(const int32_t& _i, const uint32_t _val, const States::NodeHeader& _header,
                                char* _node) {
      write<uint32_t>(_val, _node + _header.decorator_offset_ + _i * sizeof(uint32_t));
    }  // write_decorator

    namespace Detail {
      union Payload {
        char v1[4];
        int32_t v2;
        float v3;
        uint32_t v4;
      };
    }  // namespace Detail

    [[nodiscard]] inline std::variant<bool, int32_t, float, uint32_t> read_payload(const int32_t& _i,
                                                                                   const States::NodeHeader& _header,
                                                                                   const char* _node) {
      constexpr int32_t size    = sizeof(char) + sizeof(int32_t);

      const uint8_t type        = read<uint8_t>(_node + _header.payload_offset_ + _i * size);
      const Detail::Payload val = read<Detail::Payload>(_node + _header.payload_offset_ + _i * size + sizeof(char));

      if (type == (pl_s | pt_bool))
        return val.v1[0] != 0;
      else if (type == (pl_s | pt_int))
        return val.v2;
      else if (type == (pl_s | pt_float))
        return val.v3;
      else if (type == (pl_s | pt_dyn))
        return val.v4;

      std::unreachable();
    }  // read_payload

    void write_payload(const int32_t& _i, const std::variant<bool, int32_t, float, uint32_t>& _val,
                       const States::NodeHeader& _header, char* _node) {
      constexpr int32_t size = sizeof(char) + sizeof(int32_t);
      Detail::Payload pl;
      char type = 0;
      switch (_val.index()) {
        case 0: {
          type  = (pl_s | pt_bool);
          pl.v2 = std::get<0>(_val) ? (int32_t)1 : int32_t(0);
          break;
        }
        case 1: {
          type  = (pl_s | pt_int);
          pl.v2 = std::get<1>(_val);
          break;
        }
        case 2: {
          type  = (pl_s | pt_float);
          pl.v3 = std::get<2>(_val);
          break;
        }
        case 3: {
          type  = (pl_s | pt_dyn);
          pl.v4 = std::get<3>(_val);
          break;
        }
      }
      write<char>(type, _node + _header.payload_offset_ + _i * size);
      write<Detail::Payload>(pl, _node + _header.payload_offset_ + _i * size + sizeof(char));
    }  // write_payload

    namespace Detail {

      template <typename... Ts>
      std::variant<std::remove_reference_t<Ts>...> tuple_element_to_variant(const std::tuple<Ts...>& tuple,
                                                                            const size_t _index) {
        std::variant<std::remove_reference_t<Ts>...> result;
        size_t current = 0;
        ((current++ == _index ? result = std::get<Ts>(std::move(tuple)) : result), ...);
        return result;
      }  // tuple_element_to_variant

      template <class Task, class... Ts>
      Task construct_task(const std::vector<uint32_t>& _idxs,
                          const std::vector<std::variant<bool, int32_t, float, uint32_t>>& _pl,
                          const std::tuple<Ts...>& _params) {
        static_assert(std::is_class_v<Task>, "Task must be a class/struct");

        // this creates a copy of all params. maybe not the best way
        const auto& args         = _params;
        constexpr auto args_size = std::tuple_size_v<std::decay_t<decltype(_params)>>;
        constexpr auto N         = glz::reflect<Task>::size;

        Task out;

        // default constructed
        if (args_size + _pl.size() == 0) return out;

        // if not default constructed the params will be matched in order
        if constexpr (N > 0) {
          auto tie = glz::to_tie(out);

          for (size_t i = 0; i < N; ++i) {
            // more members than payloads
            if (i >= _idxs.size()) break;

            const auto field = glz::detail::get_runtime(tie, i);

            // dynamic payload
            if (_idxs[i] >= _pl.size()) {
              if constexpr (args_size > 0) {
                const auto val = tuple_element_to_variant(args, _idxs[i] - _pl.size());

                std::visit(
                    [&](auto p, auto& v) {
                      using ValueType   = std::decay_t<decltype(v)>;
                      using PointerType = std::remove_pointer_t<std::decay_t<decltype(p)>>;
                      if constexpr (std::is_same_v<PointerType, ValueType>) *p = std::move(v);
                    },
                    field, val);
              }
            }

            // static payload
            else {
              std::visit(
                  [&](auto ptr) {
                    if constexpr (std::is_same_v<bool, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                      if (_pl[_idxs[i]].index() == 0) *ptr = std::get<0>(_pl[_idxs[i]]);
                    } else if constexpr (std::is_same_v<int32_t, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                      if (_pl[_idxs[i]].index() == 1) *ptr = std::get<1>(_pl[_idxs[i]]);
                    } else if constexpr (std::is_same_v<float, std::decay_t<std::remove_pointer_t<decltype(ptr)>>>) {
                      if (_pl[_idxs[i]].index() == 2) *ptr = std::get<2>(_pl[_idxs[i]]);
                    } else {
                      std::unreachable();
                    }
                  },
                  field);
            }
          }
        }

        return out;
      }  // construct_task

    }  // namespace Detail

    template <class Variant, class... Ts>
    [[nodiscard]] Variant* alloc_task(uint16_t _idx, const std::vector<uint32_t>& _idxs,
                                      const std::vector<std::variant<bool, int32_t, float, uint32_t>>& _pl,
                                      const std::tuple<Ts...>& _params) {
      constexpr size_t variant_size = std::variant_size_v<Variant>;
      assert(_idx < variant_size);
      Variant* v            = new Variant();

      auto construct_params = [&]<size_t... Is>(std::index_sequence<Is...>) {
        ((Is == _idx
              ? ((*v = Detail::construct_task<std::variant_alternative_t<Is, Variant>>(_idxs, _pl, _params)), true)
              : false),
         ...);
      };

      auto construct = [v, _idx]<size_t... Is>(std::index_sequence<Is...>) {
        ((Is == _idx ? (v->template emplace<Is>(), true) : false), ...);
      };

      if (_idxs.empty())
        construct(std::make_index_sequence<variant_size>{});
      else
        construct_params(std::make_index_sequence<variant_size>{});

      return v;
    }  // alloc_task

    template <class State>
    Execute::States::NodeHeader create_node_header(const int16_t _vidx, const int16_t _children_count,
                                                   const int16_t _decorator_count, const int16_t _payload_count) {
      Execute::States::NodeHeader header;

      header.vidx_             = _vidx;
      header.parent_           = 0;

      // children
      header.children_count_   = _children_count;
      header.children_offset_  = sizeof(Execute::States::NodeHeader) + sizeof(State);

      // decorators
      header.decorator_count_  = _decorator_count;
      header.decorator_offset_ = header.children_offset_ + header.children_count_ * sizeof(uint32_t);

      // payload
      header.payload_count_    = _payload_count;
      header.payload_offset_   = header.decorator_offset_ + header.decorator_count_ * sizeof(uint32_t);

      header.node_size_        = header.payload_offset_ + header.payload_count_ * (sizeof(uint8_t) + sizeof(uint32_t));

      return header;
    }  // create_node_header

    uint32_t ptr(const char* _base, const char* _p) { return (uint32_t)(_p - _base); }  // ptr

  }  // namespace Execute

}  // namespace TBT