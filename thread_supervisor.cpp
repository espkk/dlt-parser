#include <thread>
#include <type_traits>
#include <cassert>

#include "thread_supervisor.h"
#include "exceptions.h"

namespace dlt {
  task::task(std::unique_ptr<fs::reader> reader) : reader_(std::move(reader)) {
  }

  const fs::reader& task::get_reader() const {
    return *reader_;
  }

  void task::execute() {
    // if parse exception occured push corrupted record if no others in a row
    // this is to be in compliance with DLT viewer
    const auto parse_exception_handler = [this](const Record& record) {
      // TODO: log
      if(records_.size() == 0 || !records_.back().isCorrupted()) {
        records_.push_back(record);
      }
    };

    while (true) {
      try {
        Record record;
        record.parse(*reader_, parse_exception_handler);
        // we do not emplace anymore because parse may throw
        records_.push_back(std::move(record));

        // check if we overruned current chunk
        if (reader_->get_overrun() > 0) {
          throw except::eof{};
        }

        // if exception was raised somewhere in another thread - cancel execution
        if (supervisor::exception_ptr_holder != nullptr) {
          break;
        }
      }
      catch (const except::eof&) {
        break;
      }
      catch(...) {
        supervisor::exception_ptr_holder = std::current_exception();
        break;
      }
    }
  }

  std::vector<Record>& task::result() {
    return records_;
  }

  supervisor::supervisor(fs::reader&& reader) {
    // threads_num_ should be configurable defaulting to the number of cores
    threads_num_ = cores_num;

    auto&& readers = reader.split(threads_num_);

    tasks_.reserve(threads_num_);
    for (size_t i = 0; i != threads_num_; ++i) {
      tasks_.emplace_back(std::move(readers[i]));
    }
  }

  void supervisor::execute(std::vector<Record>& records) {
    std::vector<std::thread> threads(threads_num_);

    for (size_t i = 0; i != threads_num_; ++i) {
      threads[i] = std::thread{[this, i]() { tasks_[i].execute(); }};
    }

    // firstly wait for all threads and store results
    std::vector<std::remove_reference_t<decltype(tasks_[0].result())>> results;
    results.reserve(threads_num_);
    for (size_t i = 0; i != threads_num_; ++i) {
      threads[i].join();
      results.push_back(std::move(tasks_[i].result()));
    }

    // secondly store results contiguously...
    records = std::move(results[0]);
    // reserve enough space and then just copy into the memory
    // this thing is a bit non-standard (UB)
    // actually, the standard forbids accessing reserved but non-resized
    // but since this memory is contigously allocated we are sure that everything goes well
    // otherwise the overhead zeroing would be performed
    std::size_t size{ 0 };
    for (size_t i = 0; i != threads_num_; ++i) {
      size += results[i].size();
    }
    records.reserve(size);

    for(size_t i=1; i != threads_num_; ++i) {
      // the whole point of this check is to determine wheter the corrupted record is
      // a result of splitting into chunks or not
      // if so, we just remove that corrupted record
      // we also remove it if overrun is because eof for two tasks in a row
      // this means that some chunks do not have any records at all. we do assert in such a case
      bool skip_first_record{ false };
      if (results[i].size() > 0 && results[i][0].isCorrupted()) {
        const auto prev_overrun = tasks_[i - 1].get_reader().get_overrun();
        const auto cur_overrun = tasks_[i].get_reader().get_overrun();
        const auto cur_valid_offset = tasks_[i].get_reader().get_first_valid_offset();

        if ((prev_overrun > 0 && prev_overrun == cur_valid_offset) 
          || (prev_overrun == fs::reader::OVERRUN_EOF && cur_overrun == fs::reader::OVERRUN_EOF && 
            (assert(results[i].size() == 1), true))) {
          skip_first_record = true;
        }
      }

      // if the first record is corrupted because it was splitted into chunks in the middle it will be skipped
      // such record itself is a part of previous chunk no matter if it is correct or not
      std::move(std::begin(results[i]) + (skip_first_record ? 1 : 0), std::end(results[i]), std::back_inserter(records));
    }
    
    // if exception was raised rethrow
    if (exception_ptr_holder != nullptr) {
      std::rethrow_exception(exception_ptr_holder);
    }
  }
}
