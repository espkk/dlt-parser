#include <fstream>
#include <filesystem>
#include <exception>

#include <boost/iostreams/device/mapped_file.hpp>

#include "filereader.h"
#include "exceptions.h"

namespace dlt::fs {
  std::vector<std::unique_ptr<reader>> reader::split(const uint8_t num) {
    if (len_ == 0) {
      // split of uninitialized reader? likely file is empty
      throw except::eof{};
    }

    std::vector<std::unique_ptr<reader>> readers;
    for (size_t i = 0; i != num; ++i) {
      const auto reader_begin = len_ / num * i;
      const auto reader_end = len_ / num * (i + 1) - 1;

      readers.emplace_back(clone());
      auto& back = readers.back();
      back->pos_ = reader_begin;
      back->chunk_len_ = reader_end;
    }

    return readers;
  }

  std::size_t reader::get_overrun() const {
    return overrun_;
  }

  std::size_t reader::get_first_valid_offset() const {
    return first_valid_offset_;
  }

  void reader::notify_success(const std::size_t offset) {
    if (first_valid_offset_ == 0) {
      first_valid_offset_ = offset;
    }
    if (pos_ == len_) {
      throw except::eof();
    }
  }

  class file_precache final : public reader {
  private:
    file_precache(const file_precache&) = default;

    std::shared_ptr<char[]> buffer_;

    [[nodiscard]]
    std::unique_ptr<reader> clone() override {
      return std::unique_ptr<reader>(new file_precache(*this));
    }

  public:
    file_precache(file_precache&&) = delete;

    explicit file_precache(const std::filesystem::path& path) {
      std::ifstream is(path, std::ios::binary);

      is.seekg(0, std::ios::end);
      len_ = is.tellg();
      is.seekg(0, std::ios::beg);

      buffer_.reset(new char[len_]);
      is.read(buffer_.get(), len_);
    }

    [[nodiscard]]
    const char* read(const std::size_t bytes_to_read) override {
      const auto new_pos = pos_ + bytes_to_read;

      // this is not ok. throw eof but with parse_exception ontop
      if (new_pos > len_) {
        overrun_ = OVERRUN_EOF;
        try {
          throw except::eof();
        }
        catch(...) {
          std::throw_with_nested(except::parse_exception("file ended with incomplete record"));
        }
      }

      if (new_pos > chunk_len_) {
        // store offset of overrun to keep it in mind while validating next chunk
        overrun_ = new_pos;
      }

      auto&& entry = buffer_.get() + pos_;
      pos_ = new_pos;
      return entry;
    }

    void set_pos(const std::size_t pos) override {
      assert(pos < len_);
      pos_ = pos;
    }

    [[nodiscard]]
    size_t get_pos() const noexcept override {
      return pos_;
    }
  };

  class file_map final : public reader {
  private:
    file_map(const file_map&) = default;

    std::shared_ptr<boost::iostreams::mapped_file> mapped_file_;

    [[nodiscard]]
    std::unique_ptr<reader> clone() override {
      return std::unique_ptr<reader>(new file_map(*this));
    }

  public:
    file_map(file_precache&&) = delete;

    explicit file_map(const std::filesystem::path& path) : mapped_file_{
      std::make_shared<boost::iostreams::mapped_file>()
    } {
      mapped_file_->open(path.string(), boost::iostreams::mapped_file::readonly);
      len_ = mapped_file_->size();
    }

    [[nodiscard]]
    const char* read(const std::size_t bytes_to_read) override {
      if (pos_ + bytes_to_read > len_) {
        throw except::eof{};
      }

      auto&& entry = mapped_file_->const_begin() + pos_;
      pos_ += bytes_to_read;
      return entry;
    }

    void set_pos(const std::size_t pos) override {
      assert(pos < len_);
      pos_ = pos;
    }

    [[nodiscard]]
    size_t get_pos() const noexcept override {
      return pos_;
    }
  };

  std::unique_ptr<reader> reader::factory(const reader_type type, const std::filesystem::path& path) {
    switch (type) {
    case reader_type::file_precache:
      return std::make_unique<file_precache>(path);
    case reader_type::file_map:
      return std::make_unique<file_map>(path);
    }
    throw std::runtime_error("unknown reader_type");
  }
}
