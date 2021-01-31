#pragma once
#include <vector>
#include <exception>

#include "record.h"
#include "filereader.h"

namespace dlt {
  class task {
  public:
    explicit task(std::unique_ptr<fs::reader> reader);
    void execute();
    const fs::reader& get_reader() const;

    [[nodiscard]]
    std::vector<Record>& result();

  private:
    std::unique_ptr<fs::reader> reader_;
    std::vector<Record> records_;
  };

  class supervisor {
  public:
    inline static const auto cores_num = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;

    // we do not really care about how much exceptions or in what order they are
    // if any exception is detected it will be rethrown
    class : std::exception_ptr {
      friend class supervisor;
      friend class task;
      using std::exception_ptr::operator=;
    } inline static exception_ptr_holder{ };
    
    explicit supervisor(fs::reader&& reader);
    void execute(std::vector<Record>& records);

  private:
    uint8_t threads_num_{};
    std::vector<task> tasks_;
  };
}
