#pragma once
#include <memory>
#include <filesystem>

namespace dlt::fs {
  enum class reader_type {
    file_precache,
    file_map
  };

  class reader {
  public:
    // special case of overrun
    static constexpr std::size_t OVERRUN_EOF = std::numeric_limits<std::size_t>::max();

    virtual ~reader() = default;

    [[nodiscard]]
    virtual const char* read(std::size_t bytes_to_read) = 0;

    virtual void set_pos(std::size_t pos) = 0;

    [[nodiscard]]
    virtual std::size_t get_pos() const noexcept = 0;

    [[nodiscard]]
    static std::unique_ptr<reader> factory(reader_type type, const std::filesystem::path& path);

    // split reader into several parallel readers
    std::vector<std::unique_ptr<reader>> split(uint8_t num);

    std::size_t get_overrun() const;
    std::size_t get_first_valid_offset() const;

    // this is called after successful read
    // we use this only to capture first valid offset
    void notify_success(std::size_t offset);

  protected:
    std::size_t pos_{};
    std::size_t len_{};
    // for chunks which are being parsed in different threads overrun should be possible to handle borderline message
    // hence we introduce separate chunk fence instead of general len to let a reader know it may continue once
    // we set it to max value to get rid of !=0 check for last/single thread
    std::size_t chunk_len_{std::numeric_limits<decltype(chunk_len_)>::max()};
    // overrun is set if chunk_len_ exceeded and should erase eof by manual check-call
    // together with first_valid_offset it is used to validate 
    // the beginning of the next chunk and check whether it is corrupted
    std::size_t overrun_ = 0;
    std::size_t current_offset_ = 0;
    std::size_t first_valid_offset_ = 0;

  private:
    virtual std::unique_ptr<reader> clone() = 0;
  };
}
