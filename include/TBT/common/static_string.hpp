#include <TBT/common/defines.hpp>
#include <TBT/strtonum/StrToNum.hpp>

namespace TBT {

  template <size_t N>
  struct sString {
    size_t size_ = 0;
    std::array<char, N> s_;

    constexpr sString() : size_(N) {
      for (size_t i = 0; i < N; ++i) s_[i] = '\0';
    }
  };  // sString

  using sStringView = std::pair<size_t, size_t>;

  template <size_t N1, size_t N2>
  constexpr bool operator==(const sString<N1>& _a, const sString<N2>& _b) {
    if (_a.size_ != _b.size_) return false;
    for (size_t i = 0; i < _b.size_; ++i) {
      if (_a.s_[i] != _b.s_[i]) return false;
    }
    return true;
  }  // operator==

  template <size_t N1, size_t N2>
  constexpr bool equals(const sStringView& _a_v, const sString<N1>& _a_s, const sString<N2>& _b) {
    if (_a_v.second - _a_v.first > _b.size_) return false;

    for (size_t i = _a_v.first, j = 0; i < _a_v.second; ++i, ++j)
      if (_a_s.s_[i] != _b.s_[j]) return false;

    return true;
  }  // operator==

  template <size_t N>
  [[nodiscard]] constexpr sString<N> ss_from_literal(char const (&_arr)[N]) {
    sString<N> out;
    out.size_ = N;
    for (size_t i = 0; i < N; ++i) out.s_[i] = _arr[i];
    return out;
  }  // ss_from_literal

  template <size_t N>
  [[nodiscard]] constexpr sString<N> ss_clean(const sString<N>& _in) noexcept {
    sString<N> out;
    size_t j = 0;
    for (size_t i = 0; i < _in.size_; ++i) {
      switch (_in.s_[i]) {
        case 0x20:  // space ' '
        case 0x0c:  // form feed '\f'
        case 0x0a:  // line feed '\n'
        case 0x0d:  // carriage return '\r'
        case 0x09:  // horizontal tab '\t'
        case 0x0b:  // vertical tab '\v'
          continue;
        default:
          out.s_[j++] = _in.s_[i];
      }
    }
    out.size_ = j;
    return out;
  }  // clean
}  // namespace TBT