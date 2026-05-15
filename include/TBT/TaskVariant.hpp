#pragma once

#include <TBT/defines.hpp>
#include <TBT/ext/ctre/ctre.hpp>

namespace TBT {
  consteval inline std::meta::info create_task_variant() {
    const auto arg_types2 = std::meta::members_of(^^TBT, std::meta::access_context::unchecked());
    std::vector<std::meta::info> metas;
    for (const auto& t : arg_types2) {
      if (!std::meta::is_type(t)) continue;
      const std::string_view id = std::meta::identifier_of(t);
      if (ctre::starts_with<"^Task">(id)) { metas.push_back(t); }
    }
    if (metas.empty()) return ^^std::variant<std::monostate>;
    return std::meta::substitute(^^std::variant, metas);
  }  // create_task_variant
}  // namespace TBT

#define DefineTaskVariant using TaskVariant = [:TBT::create_task_variant():];