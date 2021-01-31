#pragma once
#include <memory>
#include <string_view>
#include <optional>
#include <functional>

#include "filereader.h"

namespace dlt {
  class Record final {
    // copy&swap
    friend void swap(Record& r1, Record& r2);
    // actual implementation
    class Impl;

    using microseconds = uint64_t;
  public:
    Record();
    // special case. construct from Impl
    Record(const Record&) noexcept;
    Record(Record&&) noexcept;
    ~Record();
    
    // why the fuck we do not do this in the constructor?? because copying not fully-constructed
    // object will lead to heap corruption despite the fact all the allocations are done before construction
    // C++ I hate you                                                              just kidding, pls no bugs
    void parse(fs::reader& reader, std::optional<std::function<void(const Record&)>> parse_except_handler);

    // not a DLT field. used to flag corrupted message(s)
    [[nodiscard]] bool isCorrupted() const;
    [[nodiscard]] std::string_view getCorruptionCause() const;
    
    [[nodiscard]] const char* getMessage() const;
    [[nodiscard]] std::string_view getApid() const;
    [[nodiscard]] std::string_view getCtid() const;
    [[nodiscard]] microseconds getTimeStamp() const;
    [[nodiscard]] uint32_t getTimestampExtra() const;
    [[nodiscard]] uint32_t getSessionId() const;
    [[nodiscard]] uint8_t getMessageCounter() const;
    [[nodiscard]] int8_t getType() const;
    [[nodiscard]] int8_t getSubType() const;
    [[nodiscard]] std::string_view getEcu() const;

  private:
    /* TODO: (when arrive) std::propagate_const */
    std::unique_ptr<Impl> pImpl;
  };
}
