#pragma once
#include <string_view>
#include <list>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <memory>

namespace dlt::lazy {

  // this container uses list to 'lazy' evaluate the string in runtime
  // proxy design would technically do the same but requires recursion for evaluating leafs
  class string final {
  private:
    // string parts to lazy concat
    std::list<std::string_view> parts_;

    // if necessary we hold the pointers to release them at the moment of destruction
    std::vector<std::unique_ptr<char[]>> pointers_;

    bool is_evaluated_{false};

  public:
    string& operator+(const std::string_view str) {
      parts_.push_back(str);
      return *this;
    }

    template <typename T>
    string& operator+=(const T val) {
      return operator+(val);
    }

    string& append(std::unique_ptr<char[]> ptr, const size_t length) {
      const auto* buf_ptr = ptr.get();
      pointers_.push_back(std::move(ptr));
      return operator+(std::string_view(buf_ptr, length));
    }

    explicit operator std::string() {
      if (is_evaluated_) {
        // technically this is not error, but since we use it in terms of performance multiple evaluation likely is a mistake
        throw std::logic_error("this string is already evaluated");
      }
      is_evaluated_ = true;

      std::string evaluated_string;

      // 1st iteration: calc length
      const auto string_length = std::reduce(parts_.cbegin(), parts_.cend(), 0U,
                                             [](const decltype(parts_)::size_type& length,
                                                const std::string_view string) {
                                               return length + std::size<std::string_view>(string);
                                             });
      evaluated_string.reserve(string_length);

      // 2nd iteration: concat
      for (const auto& part : parts_) {
        evaluated_string.append(part);
      }

      return evaluated_string;
    }
  };
}
