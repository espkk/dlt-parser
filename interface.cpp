#include <filesystem>
#include <boost/dll.hpp>
#include <rostrum/api.hpp>

#include "filereader.h"
#include "exceptions.h"
#include "record.h"
#include "thread_supervisor.h"

/* for lld */
#ifdef __llvm__
# pragma comment(lib, "lua")
# pragma comment(lib, "fmt")
#	pragma comment(lib, "boost_iostreams-vc140-mt")
#endif

class dlt_file_adapter {
private:
  std::vector<dlt::Record> records;
public:
  void parse(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
      throw std::runtime_error("DLT file not found");
    }
    auto&& reader = dlt::fs::reader::factory(dlt::fs::reader_type::file_precache, filename);
    //if (dlt::supervisor::cores_num > 1) 
    {
      try {
        dlt::supervisor supervisor(std::move(*reader));
        supervisor.execute(records);
      }
      catch(const dlt::except::eof&) {
        // file len is 0
        // TODO: log this
      }
    }
    /*else {
      // single thread
      while (true) {
        try {
          // TODO: callback for exceptions
          records.emplace_back(*reader, std::nullopt);
        }
        catch (const dlt::except::eof&) {
          break;
        }
      }
    }*/
  }

  [[nodiscard]] auto records_num() const {
    return records.size();
  }

  [[nodiscard]] const auto & get_record(const size_t index) const {
    return records[index];
  }
};


sol::table imbue_lua(sol::state_view& lua) {
  auto table = lua.create_table();

  static_cast<void>(table.new_usertype<dlt_file_adapter>("dlt_file",
    "parse", &dlt_file_adapter::parse,
    "records_num", &dlt_file_adapter::records_num,
    "get_record", &dlt_file_adapter::get_record));

  static_cast<void>(table.new_usertype<dlt::Record>("",
    "is_corrupted", &dlt::Record::isCorrupted,
    "get_corruption_cause", &dlt::Record::getCorruptionCause,
    "get_message", &dlt::Record::getMessage,
    "get_apid", &dlt::Record::getApid,
    "get_ctid", &dlt::Record::getCtid,
    "get_timestamp", &dlt::Record::getTimeStamp,
    "get_timestamp_extra", &dlt::Record::getTimestampExtra,
    "get_sessionid", &dlt::Record::getSessionId,
    "get_counter", &dlt::Record::getMessageCounter,
    "get_type", &dlt::Record::getType,
    "get_subtype", &dlt::Record::getSubType,
    "get_ecu", &dlt::Record::getEcu
    ));

  return table;
}

void query_info(rostrum::api::module_info& module_info_ptr) noexcept {
  module_info_ptr = rostrum::api::module_info{
    {"dlt-parser"}, {"dlt-parser"}, rostrum::api::module_version{1, 0}, imbue_lua
  };
}

BOOST_DLL_ALIAS(query_info, __rostrum_query_info);
