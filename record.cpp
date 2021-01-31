#include <string>
#include <iterator>
#include <array>
#include <optional>

#include "record.h"
#include "argparser.h"
#include "exceptions.h"

/* compiler-specific struct packing */
#ifndef PACK_1
# ifdef _MSC_VER
#  define PACK_1(decl)  __pragma(pack(push, 1)) decl __pragma(pack(pop))
# else
#  error please, implement compiler-specific pack
# endif
#endif

namespace dlt {
  /* compilance layer with the original DLT protocol */
  namespace conformance {
    constexpr size_t DLT_ID_SIZE = 4;
    using ID4 = uint8_t[DLT_ID_SIZE];

    /* htyp flags */
    enum class HtypMask : uint8_t {
      /**< use extended header */
      UEH = 0x01,
      /**< MSB first */
      MSBF = 0x02,
      /**< with ECU ID */
      WEID = 0x04,
      /**< with session ID */
      WSID = 0x08,
      /**< with timestamp */
      WTMS = 0x10,
      /**< version number, 0x1 */
      VERS = 0xe0
    };

    /* msin flags */
    enum class MsinMask : uint8_t {
      /**< verbose */
      VERB = 0x01,
      /**< message type */
      MSTP = 0x0e,
      /**< message type info */
      MTIN = 0xf0
    };

    /* verbose mode of the message */
    enum class Mode : int8_t {
      Unknown = -2,
      NonVerbose = 0,
      Verbose
    };

    /* type of the DLT message */
    enum class MsgType : int8_t {
      TypeUnknown = -2,
      TypeLog = 0,
      TypeAppTrace,
      TypeNwTrace,
      TypeControl
    };

    /* subtype of the DLT message, if type is TypeLog */
    enum class LogType : int8_t {
      LogUnknown = -2,
      LogDefault = -1,
      LogOff = 0,
      LogFatal,
      LogError,
      LogWarn,
      LogInfo,
      LogDebug,
      LogVerbose
    };

    /* subtype of the DLT message, if type is TypeAppTrace */
    enum class TraceType : int8_t {
      TraceUnknown = -2,
      TraceVariable = 1,
      TraceFunctionIn,
      TraceFunctionOut,
      TraceState,
      TraceVfb
    };

    /* subtype of the DLT message, if type is TypeNwTrace */
    enum class NetworkTraceType : int8_t {
      NetworkTraceUnknown = -2,
      NetworkTraceIpc = 1,
      NetworkTraceCan,
      NetworkTraceFlexray,
      NetworkTraceMost
    };

    /* subtype of the DLT message, if type is TypeControl */
    enum class ControlType : int8_t {
      ControlUnknown = -2,
      ControlRequest = 1,
      ControlResponse,
      ControlTime
    };

    /* definitions of DLT services */
    enum class CtrlServiceId : uint32_t {
      /**< Service ID: Set log level */
      SET_LOG_LEVEL = 1,
      /**< Service ID: Set trace status */
      SET_TRACE_STATUS,
      /**< Service ID: Get log info */
      GET_LOG_INFO,
      /**< Service ID: Get dafault log level */
      GET_DEFAULT_LOG_LEVEL,
      /**< Service ID: Store configuration */
      STORE_CONFIG,
      /**< Service ID: Reset to factory defaults */
      RESET_TO_FACTORY_DEFAULT,
      /**< Service ID: Set communication interface status */
      SET_COM_INTERFACE_STATUS,
      /**< Service ID: Set communication interface maximum bandwidth */
      SET_COM_INTERFACE_MAX_BANDWIDTH,
      /**< Service ID: Set verbose mode */
      SET_VERBOSE_MODE,
      /**< Service ID: Set message filtering */
      SET_MESSAGE_FILTERING,
      /**< Service ID: Set timing packets */
      SET_TIMING_PACKETS,
      /**< Service ID: Get local time */
      GET_LOCAL_TIME,
      /**< Service ID: Use ECU id */
      USE_ECU_ID,
      /**< Service ID: Use session id */
      USE_SESSION_ID,
      /**< Service ID: Use timestamp */
      USE_TIMESTAM,
      /**< Service ID: Use extended header */
      USE_EXTENDED_HEADER,
      /**< Service ID: Set default log level */
      SET_DEFAULT_LOG_LEVEL,
      /**< Service ID: Set default trace status */
      SET_DEFAULT_TRACE_STATUS,
      /**< Service ID: Get software version */
      GET_SOFTWARE_VERSION,
      /**< Service ID: Message buffer overflow */
      MESSAGE_BUFFER_OVERFLOW,
      /**< Service ID: Message unregister context */
      UNREGISTER_CONTEXT = 0xf01,
      /**< Service ID: Message connection info */
      CONNECTION_INFO = 0xf02,
      /**< Service ID: Timezone */
      TIMEZONE = 0xf03,
      /**< Service ID: Timezone */
      MARKER = 0xf04,
      /**< Service ID: Message Injection (minimal ID) */
      CALLSW_CINJECTION = 0xFFF
    };

    inline std::string getCtrlServiceIdName(CtrlServiceId id) {
      constexpr std::array CtrlServiceIdStrings{
        "", "set_log_level", "set_trace_status", "get_log_info", "get_default_log_level", "store_config",
        "reset_to_factory_default",
        "set_com_interface_status", "set_com_interface_max_bandwidth", "set_verbose_mode", "set_message_filtering",
        "set_timing_packets",
        "get_local_time", "use_ecu_id", "use_session_id", "use_timestamp", "use_extended_header",
        "set_default_log_level", "set_default_trace_status",
        "get_software_version", "message_buffer_overflow"
      };

      using underlying_t = std::underlying_type_t<decltype(id)>;

      const auto index = static_cast<underlying_t>(id);

      if (index < static_cast<underlying_t>(CtrlServiceId::SET_LOG_LEVEL) || index > static_cast<underlying_t>(
        CtrlServiceId::MESSAGE_BUFFER_OVERFLOW)) {
        return "service(" + std::to_string(index) + ")";
      }

      return CtrlServiceIdStrings[index];
    }

    /* definition of DLT ctrl return types*/
    enum class CtrlReturnType : uint8_t {
      OK,
      NOT_SUPPORTED,
      ERROR,
      _3,
      _4,
      _5,
      _6,
      _7,
      NO_MATCHING_CONTEXT_ID
    };

    constexpr auto getCtrlReturnTypeName(CtrlReturnType type) {
      constexpr std::array CtrlReturnTypeStrings{
        "ok", "not_supported", "error", "3", "4", "5", "6", "7", "no_matching_context_id"
      };

      const auto index = static_cast<std::underlying_type_t<decltype(type)>>(type);

      if (index >= CtrlReturnTypeStrings.size()) {
        throw except::parse_exception("invalid CtrlReturnType");
      }

      return CtrlReturnTypeStrings[index];
    }

    enum class ConnectionStatus : uint8_t {
      /**< Client is disconnected */
      DISCONNECTED = 1,
      /**< Client is connected */
      CONNECTED
    };

    constexpr auto getConnectionStatus(ConnectionStatus status) {
      if (status == ConnectionStatus::DISCONNECTED) {
        return "disconnected";
      }
      if (status == ConnectionStatus::CONNECTED) {
        return "connected";
      }
      return "unknown";
    }

    constexpr bool extractHtyp(uint8_t htyp, HtypMask mask) {
      return htyp & static_cast<uint8_t>(mask);
    }

    template <MsinMask mask>
    constexpr uint8_t extractMsin(uint8_t msin) {
      int shift{};
      if constexpr (mask == MsinMask::VERB) {
        shift = 0;
      }
      else if constexpr (mask == MsinMask::MSTP) {
        shift = 1;
      }
      else if constexpr (mask == MsinMask::MTIN) {
        shift = 4;
      }
      else {
        throw "unknown MsinMask"; // static exception, no need to use std::exception
      }

      const auto byte = static_cast<uint8_t>(mask);
      return (msin & byte) >> shift;
    }

    PACK_1(
      /**
       * The structure of the DLT file storage header. This header is used before each stored DLT message.
       */
      struct DltStorageHeader
    {
      /**< This pattern should be DLT0x01 */
      ID4 pattern;
      /**< seconds since 1.1.1970 */
      uint32_t seconds;
      /**< Microseconds */
      uint32_t microseconds;
      /**< The ECU id is added, if it is not already in the DLT message itself */
      ID4 ecu;
    };

    /**
     * The structure of the DLT standard header. This header is used in each DLT message.
     */
    struct DltStandardHeader
    {
      /**< This parameter contains several informations, see definitions below */
      uint8_t htyp;
      /**< The message counter is increased with each sent DLT message */
      uint8_t mcnt;
      /**< Length of the complete message, without storage header */
      uint16_t len;
    };

    /**
     * The structure of the DLT extra header parameters. Each parameter is sent only if enabled in htyp.
     */
    struct DltStandardHeaderExtra
    {
      /**< ECU id */
      ID4 ecu;
      /**< Session number */
      uint32_t seid;
      /**< Timestamp since system start in 0.1 milliseconds */
      uint32_t tmsp;
    };

    /**
     * The structure of the DLT extended header. This header is only sent if enabled in htyp parameter.
     */
    struct DltExtendedHeader
    {
      /**< messsage info */
      uint8_t msin;
      /**< number of arguments */
      uint8_t noar;
      /**< application id */
      ID4 apid;
      /**< context id */
      ID4 ctid;
    }
    );
  };

  class Record::Impl {
    using SubTypeVal = int8_t;
    const SubTypeVal SUBTYPE_UNKNOWN = -2;
  private:
    /* this is not related to DLT protocol
       and indicates that the stream related to the recor was corrupted
       this is used to use marked record as an indication of corrupted stream
       to be compliant with DLT viewer
       additionally, it stores the parsing error
    */
    std::optional<std::string> corruptionCause_{ std::nullopt };

    /* DLT headers & payload */
    conformance::DltStorageHeader storageHeader_{};
    conformance::DltStandardHeader standardHeader_{};
    conformance::DltStandardHeaderExtra standartHeaderExtra_{};
    conformance::DltExtendedHeader extendedHeader_{};

    /* options */
    bool isBigEndian_ = false;
    conformance::Mode mode_ = conformance::Mode::NonVerbose;
    conformance::MsgType type_ = conformance::MsgType::TypeUnknown;
    SubTypeVal subType_ = SUBTYPE_UNKNOWN;
    conformance::CtrlReturnType ctrlReturnType_{};
    conformance::CtrlServiceId ctrlServiceId_{};

    /* formatted output message */
    std::string message_;

    static std::string_view get_view(const conformance::ID4 field) {
      const size_t len = field[3] ? 4 : field[2] ? 3 : field[1] ? 2 : field[0] ? 1 : 0;
      return { reinterpret_cast<const char*>(&field[0]), len };
    }

    void parse(fs::reader& reader) {
      using namespace conformance;

      decltype(standardHeader_.len) len = 0; /* length of payload + headers (except storage header) */

      /* read helper */
      auto readData = [&reader, &len](auto& field) {
#ifdef _BIT_
#pragma message("use std::bitcast")
#endif
        ::memcpy(&field, reader.read(sizeof(field)), sizeof(field));
        len += sizeof(field);
      };

      /* default headers */
      readData(storageHeader_.pattern);

      /* verify DLT signature. TODO: verify all fields */
      if (storageHeader_.pattern[0] != 'D' ||
        storageHeader_.pattern[1] != 'L' ||
        storageHeader_.pattern[2] != 'T' ||
        storageHeader_.pattern[3] != '\x1') {
        throw except::parse_exception("invalid DLT signature");
      }

      // nice order. hopefully this is not endianness-defined
      readData(storageHeader_.seconds);
      readData(storageHeader_.microseconds);
      readData(storageHeader_.ecu);
      readData(standardHeader_);

      /* extra flags */
      const auto htyp = standardHeader_.htyp;

      /* endianness */
      isBigEndian_ = extractHtyp(htyp, HtypMask::MSBF);

      /* extra fields */
      if (extractHtyp(htyp, HtypMask::WEID)) {
        readData(standartHeaderExtra_.ecu);
      }


      // protocol always contains these fields in big endian? it seems so
      constexpr bool isSwapNeeded = std::endian::native == std::endian::little;
      if (extractHtyp(htyp, HtypMask::WSID)) {
        uint8_t buf[sizeof(standartHeaderExtra_.seid)];
        readData(buf);
        standartHeaderExtra_.seid = endian::read<decltype(standartHeaderExtra_.seid)>(buf, isSwapNeeded);
      }
      if (extractHtyp(htyp, HtypMask::WTMS)) {
        uint8_t buf[sizeof(standartHeaderExtra_.tmsp)];
        readData(buf);
        standartHeaderExtra_.tmsp = endian::read<decltype(standartHeaderExtra_.tmsp)>(buf, isSwapNeeded);
      }

      /* extended header */
      if (extractHtyp(htyp, HtypMask::UEH)) {
        readData(extendedHeader_);

        /* message mode, type and subtype */
        if (extractMsin<MsinMask::VERB>(extendedHeader_.msin)) {
          mode_ = Mode::Verbose;
        }
        type_ = static_cast<MsgType>(extractMsin<MsinMask::MSTP>(extendedHeader_.msin));
        subType_ = extractMsin<MsinMask::MTIN>(extendedHeader_.msin);
      }

      len = endian::detail::read16_swap(standardHeader_.len, true) - (len - sizeof(DltStorageHeader));

      const auto& data = reader.read(len);
      assembleMessage(reinterpret_cast<const uint8_t*>(data));

      /* TODO: put some safety size checks here (and endianness swaps) */
    }

    void assembleMessage(const uint8_t* payload_) {
      using namespace conformance;
      if (type_ == MsgType::TypeControl) {
        if (mode_ == Mode::Verbose) {
          throw except::parse_exception("no support for verbose ctrl messages. is it required?");
        }

        ctrlServiceId_ = endian::extract<CtrlServiceId>(payload_, isBigEndian_);

        if (static_cast<ControlType>(subType_) == ControlType::ControlResponse) {
          ctrlReturnType_ = endian::extract<CtrlReturnType>(payload_);

          if (ctrlServiceId_ == CtrlServiceId::MARKER) {
            message_ = "MARKER";
          }
          else {
            message_ = std::string{ '[' } + getCtrlServiceIdName(ctrlServiceId_) + ' ' +
              getCtrlReturnTypeName(ctrlReturnType_) + "] "; // TODO: lazy evaluation?

            switch (ctrlServiceId_) {
            case CtrlServiceId::GET_SOFTWARE_VERSION:
            {
              const auto len = endian::extract<uint32_t>(payload_);
              std::copy(payload_, payload_ + len, std::back_inserter(message_));
              break;
            }
            case CtrlServiceId::CONNECTION_INFO:
            {
              const auto status = endian::extract<ConnectionStatus>(payload_);
              message_ = message_ + getConnectionStatus(status) + ' ';
              std::copy(payload_, payload_ + DLT_ID_SIZE, std::back_inserter(message_));
              break;
            }
            case CtrlServiceId::TIMEZONE:
            {
              const auto timezone = endian::extract<uint32_t>(payload_);
              message_ = std::to_string(timezone);
              const auto isDst = endian::extract<bool>(payload_);
              if (isDst) {
                message_ += "DST";
              }
            }
            default:
              break; /* no additional data in payload */
            }
          }
        }
        else {
          message_ = std::string{ '[' } + getCtrlServiceIdName(ctrlServiceId_) + ']';
        }
      }
      else if (mode_ == Mode::Verbose) {
        if (extendedHeader_.noar > 0) {
          if (isBigEndian_) {
            message_ = ArgParser<true>(payload_, extendedHeader_.noar);
          }
          else {
            message_ = ArgParser<false>(payload_, extendedHeader_.noar);
          }
        }
      }
      else {
        /* non-verbose by default */
        const auto id = endian::extract<uint32_t>(payload_);
        message_ = '[' + std::to_string(id) + ']';

        /* TODO: read message from payload (span?) */
      }
    }

  public:
    Impl() = default;
    explicit Impl(const Impl&) = default;

    /* construct record */
    void parse(const Record& self, fs::reader& reader, std::optional<std::function<void(const Record&)>> parse_except_handler) {
      //explicit Impl(fs::reader& reader) {
      auto successfully_read{ false };
      size_t current_pos{};
      while (!successfully_read) {
        try {
          current_pos = reader.get_pos();
          parse(reader);
          reader.notify_success(current_pos);
          successfully_read = true;
        }
        catch (const except::parse_exception& e) {
          // notify upper layer to handle this
          if (parse_except_handler.has_value()) {
            corruptionCause_ = std::make_optional(e.what());
            (*parse_except_handler)(self);
            corruptionCause_.reset();
          }
          
          // eof can be reason of parse_exception
          std::rethrow_if_nested(e);

          // ill-formed; continue parsing from pos+1
          // well, this may slow things a lot but is only truly reliable way to ensure we read EVERYTHING
          reader.set_pos(current_pos + 1);
        }
        catch (...) {
          throw;
        }
      }
    }

    [[nodiscard]]
    bool isCorrupted() const {
      return corruptionCause_.has_value();
    }

    [[nodiscard]]
    std::string_view getCorruptionCause() const {
      return corruptionCause_.value();
    }

    [[nodiscard]]
    const auto& getMessage() const {
      return message_;
    }

    [[nodiscard]]
    auto getApid() const {
      return get_view(extendedHeader_.apid);
    }

    [[nodiscard]]
    auto getCtid() const {
      return get_view(extendedHeader_.ctid);
    }

    [[nodiscard]]
    auto getTimestamp() const {
      return static_cast<microseconds>(storageHeader_.seconds) * 1'000'000 + storageHeader_.microseconds;
    }

    [[nodiscard]]
    auto getTimestampExtra() const {
      return standartHeaderExtra_.tmsp;
    }

    [[nodiscard]]
    auto getMessageCounter() const {
      return standardHeader_.mcnt;
    }

    [[nodiscard]]
    auto getSessionId() const {
      return standartHeaderExtra_.seid;
    }

    [[nodiscard]]
    auto getType() {
      return type_;
    }

    [[nodiscard]]
    auto getSubType() {
      return subType_;
    }

    [[nodiscard]]
    auto getEcu() {
      return get_view(storageHeader_.ecu);
    }

    [[nodiscard]]
    auto getEcuExtra() {
      return get_view(standartHeaderExtra_.ecu);
    }
  };

  // copy&swap
  void swap(Record& r1, Record& r2) {
    using namespace std;
    swap(r1.pImpl, r2.pImpl);
  }

  /* Record proxy implementation */
  Record::Record() : pImpl(std::make_unique<Impl>()) {
  }

  Record::Record(const Record& record) noexcept : pImpl(std::make_unique<Impl>(*record.pImpl)) {
  }

  Record::Record(Record&& record) noexcept {
    swap(record, *this);
  }

  Record::~Record() = default;

  void Record::parse(fs::reader& reader, std::optional<std::function<void(const Record&)>> parse_except_handler) {
    pImpl->parse(*this, reader, parse_except_handler);
  }

  bool Record::isCorrupted() const {
    return pImpl->isCorrupted();
  }

  std::string_view Record::getCorruptionCause() const {
    return pImpl->getCorruptionCause();
  }

  const char * Record::getMessage() const {
    return pImpl->getMessage().c_str();
  }

  std::string_view Record::getApid() const {
    return pImpl->getApid();
  }

  std::string_view Record::getCtid() const {
    return pImpl->getCtid();
  }

  Record::microseconds Record::getTimeStamp() const {
    return pImpl->getTimestamp();
  }

  uint32_t Record::getTimestampExtra() const {
    return pImpl->getTimestampExtra();
  }
  
  uint32_t Record::getSessionId() const {
    return pImpl->getSessionId();
  }

  uint8_t Record::getMessageCounter() const {
    return pImpl->getMessageCounter();
  }

  int8_t Record::getType() const {
    // let's just output int
    return static_cast<int8_t>(pImpl->getType());
  }

  int8_t Record::getSubType() const {
    return pImpl->getSubType();
  }

  std::string_view Record::getEcu() const {
    return pImpl->getEcu();
  }
}
