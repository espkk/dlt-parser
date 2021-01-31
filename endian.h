#pragma once
#include <cstdint>
#include <stdexcept>

#if _HAS_CXX20
#include <bit> // std::endian, std::bitcast
#else
#include <type_traits> // std::endian
#include <algorithm> // std::copy
#endif
static_assert(std::endian::native == std::endian::little, "whoops. big-endian host not supported yet");

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace dlt::endian {
  namespace detail {
    constexpr uint16_t read16_swap(const uint16_t val, const bool big_endian) {
      return !big_endian
               ? val
               :
#ifdef _MSC_VER
               _byteswap_ushort(val)
#elif defined __GNUG__
				__builtin_bswap16(val)
#else
				((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00)
#endif
        ;
    }

    constexpr uint32_t read32_swap(const uint32_t val, const bool big_endian) {
      return !big_endian
               ? val
               :
#ifdef _MSC_VER
               _byteswap_ulong(val)
#elif defined __GNUG__
				__builtin_bswap32(val)
#else
				((val >> 24) & 0x000000FF) | ((val >> 8) & 0x0000FF00) |
				((val << 8) & 0x00FF0000) | ((val << 24) & 0xFF000000)
#endif
        ;
    }

    constexpr uint64_t read64_swap(const uint64_t val, const bool big_endian) {
      return !big_endian
               ? val
               :
#ifdef _MSC_VER
               _byteswap_uint64(val)
#elif defined __GNUG__
				__builtin_bswap64(val)
#else
				((val >> 56) & 0x00000000000000FF) | ((val >> 40) & 0x000000000000FF00) |
				((val >> 24) & 0x0000000000FF0000) | ((val >> 8) & 0x00000000FF000000) |
				((val << 8) & 0x000000FF00000000) | ((val << 24) & 0x0000FF0000000000) |
				((val << 40 & 0x00FF000000000000) | ((val << 56) & 0xFF00000000000000)
#endif
        ;
    }

    /* we assume buf containes enough bytes and properly aligned */
    inline uint8_t read8(const uint8_t* buf, const bool) { return *buf; }

    inline uint16_t read16(const uint8_t* buf, const bool big_endian) {
      return read16_swap(*reinterpret_cast<const uint16_t*>(buf), big_endian);
    }

    inline uint32_t read32(const uint8_t* buf, const bool big_endian) {
      return read32_swap(*reinterpret_cast<const uint32_t*>(buf), big_endian);
    }

    inline uint64_t read64(const uint8_t* buf, const bool big_endian) {
      return read64_swap(*reinterpret_cast<const uint64_t*>(buf), big_endian);
    }

    /* constexpr read helper */
    template <typename T>
    constexpr auto read_helper() {
      if constexpr (sizeof(T) == sizeof(uint8_t)) {
        return read8;
      }
      else if constexpr (sizeof(T) == sizeof(uint16_t)) {
        return read16;
      }
      else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return read32;
      }
      else if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return read64;
      }
      throw std::logic_error("this should never happen");
    }

    /* strict aliasing-friendly convert */
    template <typename T, typename U>
    constexpr auto reinterpret(U val) -> T {
      static_assert(sizeof(T) == sizeof(U));

      std::remove_cv_t<T> ret;
#if defined _BIT_ && 0
			ret = std::bit_cast<decltype(ret)>(val);
#else
      std::copy(reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val) + sizeof(T),
                reinterpret_cast<uint8_t*>(&ret));
#endif
      return ret;
    }

    template <typename T>
    struct readable_t : std::enable_if_t<std::disjunction_v<
        std::is_integral<T>, std::is_floating_point<T>, std::is_enum<T>> && sizeof(T) <= 8> {
    };
  }

  /* read<type> from payload */
  template <typename T, typename = detail::readable_t<T>>
  constexpr auto read(const uint8_t* payload, bool big_endian = false) -> T {
    using namespace detail;
    const auto val = read_helper<T>()(payload, big_endian);
    if constexpr (std::is_same_v<T, decltype(val)>) {
      return val;
    }
    else {
      return reinterpret<T>(val);
    }
  }

  /* extract<type> from payload */
  template <typename T, typename = detail::readable_t<T>>
  constexpr auto extract(const uint8_t*& payload, bool big_endian = false) {
    const auto val = read<T>(payload, big_endian);
    payload += sizeof(val);
    return val;
  }
}
