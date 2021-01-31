#pragma once
#include <stdexcept>

namespace dlt::except {
  class eof : public std::exception {
  };

  class parse_exception : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;

    explicit parse_exception(const char* msg = "unable to parse") : std::runtime_error(msg) {
    }
  };
}
