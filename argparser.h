#pragma once
#include <string>
#include <type_traits>
#include <utility>
#include <fmt/format.h>
#include "endian.h"
#include "exceptions.h"

namespace dlt {
  /* compilance layer with original DLT */
  namespace conformance {
    /*
     * Definitions of types of arguments in payload.
     */
    enum class ArgType : uint32_t {
      INFO_BOOL = 0x00000010,
      INFO_SINT = 0x00000020,
      INFO_UINT = 0x00000040,
      INFO_FLOA = 0x00000080,
      INFO_ARAY = 0x00000100,
      INFO_STRG = 0x00000200,
      INFO_RAWD = 0x00000400,
      INFO_VARI = 0x00000800,
      INFO_FIXP = 0x00001000,
      INFO_TRAI = 0x00002000,
      INFO_STRU = 0x00004000,
    };

    /* Mask of variable tyle */
    using arg_tyle_mask_t = std::integral_constant<uint32_t, 0x0000000f>;
    inline arg_tyle_mask_t ArgTyleMask;

    /* Mask of string/uint coding */
    using arg_coding_mask_t = std::integral_constant<uint32_t, 0x00038000>;
    inline arg_coding_mask_t ArgCodingMask;

    enum class TyleType : uint8_t {
      TYLE_8BIT = 1,
      TYLE_16BIT,
      TYLE_32BIT,
      TYLE_64BIT,
      TYLE_128BIT
    };

    enum class CodingType : uint32_t {
      SCOD_ASCII = 0x00000000,
      SCOD_UTF8 = 0x00008000,
      SCOD_HEX = 0x00010000,
      SCOD_BIN = 0x00018000
    };
  }

  namespace detail {
    /* conversion operators for masks */
    template <typename T, typename U = std::underlying_type_t<T>>
    constexpr auto operator &(T type, T mask) {
      return static_cast<U>(type) & static_cast<U>(mask);
    }

    template <typename T, typename U = std::underlying_type_t<T>>
    constexpr auto operator &(T type, conformance::arg_coding_mask_t mask) {
      auto result = static_cast<U>(type) & static_cast<U>(mask);
      return static_cast<conformance::CodingType>(result);
    }

    template <typename T, typename U = std::underlying_type_t<T>>
    constexpr auto operator &(T type, conformance::arg_tyle_mask_t mask) {
      auto result = static_cast<U>(type) & static_cast<U>(mask);
      return static_cast<conformance::TyleType>(result);
    }
  }

  template <bool BigEndian>
  class ArgParser {
  public:
    ArgParser(const uint8_t* payload, uint8_t count) : payload_(payload) {
      while (count > 1) {
        parse();
        // this is odd but conform to DLT viewer
        output_.push_back(' ');
        count--;
      }
      // what if we have 0 args monkaHmm
      if (count == 1) {
        // without trailing space
        parse();
      }
    }

    operator std::string() && {
      return std::move(output_);
    }

    void parse() {
      using namespace detail;
      using conformance::ArgType;
      using conformance::ArgTyleMask;
      using conformance::ArgCodingMask;

      const auto type = static_cast<ArgType>(endian::read<uint32_t>(payload_, BigEndian));
      payload_ += sizeof(ArgType);

      if (type & ArgType::INFO_STRG) {
        if (type & ArgType::INFO_VARI) {
          throw except::parse_exception("how could string be variable?");
        }
        return parseStr(type & ArgCodingMask);
      }
      if (type & ArgType::INFO_UINT) {
        return parseUInt(type & ArgTyleMask, type & ArgCodingMask);
      }
      if (type & ArgType::INFO_SINT) {
        return parseSInt(type & ArgTyleMask);
      }
      if (type & ArgType::INFO_FLOA) {
        return parseFloat(type & ArgTyleMask);
      }
      if (type & ArgType::INFO_BOOL) {
        return parseBool();
      }
      if (type & ArgType::INFO_RAWD) {
        return parseRaw();
      }
      if (type & ArgType::INFO_FIXP || type & ArgType::INFO_TRAI || type & ArgType::INFO_STRU) {
        throw except::parse_exception("not supported yet");
      }
      throw except::parse_exception("unknown argument type");
    }

  private:
    /* parse string */
    void parseStr(conformance::CodingType scod_type) {
      using conformance::CodingType;

      const auto len = endian::read<uint16_t>(payload_, BigEndian);
      // this is a definite parse exception
      // handle this separetely because we are doing allocation below 
      // it may occur that output+len-1 == -1
      if (len == 0) {
        throw except::parse_exception("INFO_STRG len is 0");
      }
      payload_ += sizeof(len);

      // remember offset and increase output size: len + space (instead of null-byte)
      const auto offset = std::size(output_);
      //including ' ' after arg
      output_.reserve(std::size(output_) + len);
      output_.resize(std::size(output_) + len - 1);

      switch (scod_type) {
      case CodingType::SCOD_ASCII:
        // check for null-character
        if (payload_[len - 1] != '\0') {
          throw except::parse_exception("string is not null-terminated");
        }
        std::copy_n(payload_, len - 1, std::data(output_) + offset);
        // space after string argument. otherwise multiple string args will be concatenated
        payload_ += len;
        break;
      case CodingType::SCOD_UTF8:
        throw except::parse_exception("SCOD_UTF8 is not supported yet");
      default:
        throw except::parse_exception("incorrect CodingType of string");
      }
    }

    /* parse raw data */
    void parseRaw() {
      constexpr char hexLiterals[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

      const auto len = endian::read<uint16_t>(payload_, BigEndian);
      payload_ += sizeof(len);

      // remember offset and increase output size
      const auto offset = std::size(output_);
      //including ' ' after arg
      output_.resize(std::size(output_) + len * 2 + 1);

      /* convert raw data to hex */
      for (size_t i = 0; i < len; ++i) {
        output_[offset + i * 2] = hexLiterals[payload_[i] >> 4];
        output_[offset + i * 2 + 1] = hexLiterals[payload_[i] & 0x0F];
      }

      payload_ += len;
    }

    /* parse tyle */
    enum class Radix : uint8_t {
      DEC,
      HEX,
      BIN
    };

    template <typename T, Radix R = Radix::DEC>
    void parseTyle() {
      const auto val = endian::extract<T>(payload_, BigEndian);
      // constexpr optimization is still poor... let's help our compiler
      if constexpr (R == Radix::HEX) {
        fmt::format_to(std::back_inserter(output_), "{:#x}", val);
      }
      else if constexpr (R == Radix::BIN) {
        fmt::format_to(std::back_inserter(output_), "{:#b}", val);
      }
      // unlike std iostream, fmt consider uint8_t as integer and bool as actual bool, hence there is no need to introduce different specifiers here
      fmt::format_to(std::back_inserter(output_), "{}", val);
    }

    /* parse tyle helper func */
    template <typename T>
    void parseTyle(conformance::CodingType scod_type) {
      using conformance::CodingType;

      if (scod_type == CodingType::SCOD_HEX) {
        return parseTyle<T, Radix::HEX>();
      }
      if (scod_type == CodingType::SCOD_BIN) {
        return parseTyle<T, Radix::BIN>();
      }
      return parseTyle<T>();
    }

    /* parse unsigned int */
    void parseUInt(conformance::TyleType tyle_type, conformance::CodingType scod_type) {
      using conformance::TyleType;

      switch (tyle_type) {
      case TyleType::TYLE_8BIT:
        return parseTyle<uint8_t>(scod_type);
      case TyleType::TYLE_16BIT:
        return parseTyle<uint16_t>(scod_type);
      case TyleType::TYLE_32BIT:
        return parseTyle<uint32_t>(scod_type);
      case TyleType::TYLE_64BIT:
        return parseTyle<uint64_t>(scod_type);
      case TyleType::TYLE_128BIT:
        throw except::parse_exception("not supported yet");
      default:
        throw except::parse_exception("unknown tyle type");
      }
    }

    /* parse signed int */
    void parseSInt(conformance::TyleType tyle_type) {
      using conformance::TyleType;

      switch (tyle_type) {
      case TyleType::TYLE_8BIT:
        return parseTyle<int8_t>();
      case TyleType::TYLE_16BIT:
        return parseTyle<int16_t>();
      case TyleType::TYLE_32BIT:
        return parseTyle<int32_t>();
      case TyleType::TYLE_64BIT:
        return parseTyle<int64_t>();
      case TyleType::TYLE_128BIT:
        throw except::parse_exception("not supported yet");
      default:
        throw except::parse_exception("unknown tyle type");
      }
    }

    void parseFloat(conformance::TyleType tyle_type) {
      using conformance::TyleType;

      switch (tyle_type) {
      case TyleType::TYLE_32BIT:
        return parseTyle<float>();
      case TyleType::TYLE_64BIT:
        return parseTyle<double>();
      default:
        throw except::parse_exception("unknown tyle type");
      }
    }

    void parseBool() {
      parseTyle<bool>();
    }

  private:
    const uint8_t* payload_;
    std::string output_;
  };
}
