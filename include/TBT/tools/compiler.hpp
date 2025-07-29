#pragma once

#include <TBT/common/defines.hpp>
#include <TBT/execute/helper.hpp>
#include <TBT/execute/states.hpp>

namespace TBT {

  // [header][states][children][decorators][payload]

  [[nodiscard]] inline std::string compile(const std::shared_ptr<Detail::Node>& _n) {
    // Step 1: linearize the tree using a df traversal
    std::deque<std::shared_ptr<Detail::Node>> line;

    std::stack<std::shared_ptr<Detail::Node>> stack;
    stack.push(_n);

    while (!stack.empty()) {
      std::shared_ptr<Detail::Node> node = stack.top();
      stack.pop();

      line.push_back(node);

      for (auto& c : node->c_) stack.push(c);
    }

    Execute::States::Header global_header;
    global_header.ptr_        = sizeof(Execute::States::Header);
    global_header.node_count_ = (uint32_t)line.size();

    std::vector<char> tt;
    tt.resize(10000);
    uint32_t ptr = sizeof(Execute::States::Header);

    Execute::write_global_node_header(global_header, tt.data());

    tsl::robin_map<uint64_t, uint32_t> ptr_to_idx;

    // step 2: write headers and payload
    for (auto& _node : line) {
      Execute::States::NodeHeader header;
      switch (_node->type_) {
        case Execute::vidx_root:
          header = Execute::create_node_header<Execute::States::NoState>(_node->type_, 1, 0, 0);
          break;
        case Execute::vidx_sequence:
          header = Execute::create_node_header<Execute::States::Composite>(
              _node->type_, (int16_t)_node->c_.size(), (int16_t)_node->d_.size(), (int16_t)_node->payloads_.size());
          break;
        case Execute::vidx_fallback:
          header = Execute::create_node_header<Execute::States::Composite>(
              _node->type_, (int16_t)_node->c_.size(), (int16_t)_node->d_.size(), (int16_t)_node->payloads_.size());
          break;
        case Execute::vidx_nloop:
          header = Execute::create_node_header<Execute::States::NLoop>(_node->type_, (int16_t)_node->c_.size(), 0,
                                                                       (int16_t)_node->payloads_.size());
          if (_node->payloads_.size() < 2) {  // TODO error
          }
          break;
        case Execute::vidx_cooldown:
          header = Execute::create_node_header<Execute::States::Cooldown>(_node->type_, (int16_t)_node->c_.size(), 0,
                                                                          (int16_t)_node->payloads_.size());
          {
            Execute::States::Cooldown state;
            state.is_first_ = true;
            Execute::write_node_state(state, &tt[ptr]);
          }
          break;
        case Execute::vidx_always_fail:
          header = Execute::create_node_header<Execute::States::NoState>(_node->type_, (int16_t)_node->c_.size(), 0,
                                                                         (int16_t)_node->payloads_.size());
          break;
        case Execute::vidx_always_succeed:
          header = Execute::create_node_header<Execute::States::NoState>(_node->type_, (int16_t)_node->c_.size(), 0,
                                                                         (int16_t)_node->payloads_.size());
          break;
        default:
          header = Execute::create_node_header<Execute::States::Task>(
              _node->type_, (int16_t)_node->c_.size(), (int16_t)_node->d_.size(), (int16_t)_node->payloads_.size());
      }
      // header
      Execute::write_node_header(header, &tt[ptr]);
      // write payloads
      for (int32_t i = 0; i < _node->payloads_.size(); ++i)
        Execute::write_payload(i, _node->payloads_[i], header, &tt[ptr]);

      ptr_to_idx[PtrToUlong(_node.get())] = ptr;
      ptr += header.node_size_;
    }

    // step 3: link nodes
    for (auto& _node : line) {
      const size_t n_ptr                 = ptr_to_idx.at(PtrToUlong(_node.get()));
      Execute::States::NodeHeader header = Execute::read_node_header(&tt[n_ptr]);
      if (_node->type_ == Execute::vidx_root)
        header.parent_ = 0;
      else
        header.parent_ = ptr_to_idx.at(PtrToUlong(_node->parent_.get()));
      Execute::write_node_header(header, &tt[n_ptr]);

      // write children
      for (int32_t i = 0; i < _node->c_.size(); ++i)
        Execute::write_child(i, ptr_to_idx.at(PtrToUlong(_node->c_[i].get())), header, &tt[n_ptr]);

      // write decorators
      for (int32_t i = 0; i < _node->d_.size(); ++i)
        Execute::write_decorator(i, ptr_to_idx.at(PtrToUlong(_node->d_[i].get())), header, &tt[n_ptr]);
    }

    // to check if this is a valid compiled string
    tt.push_back(Execute::is_comp);
    tt.push_back(Execute::is_comp);
    tt.push_back(Execute::is_comp);
    tt.push_back(Execute::is_comp);

    // resize to string
    std::string out;
    out.resize(ptr);
    std::memcpy(out.data(), tt.data(), ptr * sizeof(char));
    return out;
  }  // compile

}  // namespace TBT