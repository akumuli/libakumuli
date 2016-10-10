#include "ingestion_pipeline.h"
#include "logger.h"
#include "utility.h"

#include <thread>

#include <boost/exception/all.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace Akumuli
{

static Logger db_logger_ = Logger("akumuli-storage", 32);

static void db_logger(aku_LogLevel tag, const char *msg) {
    db_logger_.error() << "(" << tag << ") " << msg;
}

//! Abstraction layer above aku_Cursor
struct AkumuliCursor : DbCursor {
    aku_Cursor* cursor_;

    AkumuliCursor(aku_Cursor* cur) : cursor_(cur) { }

    virtual size_t read(void *dest, size_t dest_size) {
        return aku_cursor_read(cursor_, dest, dest_size);
    }

    virtual int is_done() {
        return aku_cursor_is_done(cursor_);
    }

    virtual bool is_error(aku_Status *out_error_code_or_null) {
        return aku_cursor_is_error(cursor_, out_error_code_or_null);
    }

    virtual void close() {
        aku_cursor_close(cursor_);
    }
};

AkumuliConnection::AkumuliConnection(const char *path)
    : dbpath_(path)
{
    aku_FineTuneParams params = {};
    params.debug_mode = 0;
    params.durability = (u32)durability;
    params.enable_huge_tlb = hugetlb ? 1u : 0u;
    params.logger = &db_logger;
    params.compression_threshold = compression_threshold;
    params.window_size = window_width;
    params.max_cache_size = cache_size;
    db_ = aku_open_database(dbpath_.c_str(), params);
}

void AkumuliConnection::close() {
    aku_close_database(db_);
}

aku_Status AkumuliConnection::write(aku_Sample const& sample) {
    // FIXME: api was changed
    //return aku_write(db_, &sample);
    throw "not implemented";
}

std::shared_ptr<DbCursor> AkumuliConnection::search(std::string query) {
    aku_Cursor* cursor = aku_query(db_, query.c_str());
    return std::make_shared<AkumuliCursor>(cursor);
}

int AkumuliConnection::param_id_to_series(aku_ParamId id, char* buffer, size_t buffer_size) {
    //return aku_param_id_to_series(db_, id, buffer, buffer_size);
    // FIXME: api was changed
    throw "not implemented";
}

aku_Status AkumuliConnection::series_to_param_id(const char *name, size_t size, aku_Sample *sample) {
    // FIXME: api changed
    //return aku_series_to_param_id(db_, name, name + size, sample);
    throw "not implemented";
}

std::string AkumuliConnection::get_all_stats() {
    //std::vector<char> buffer;
    //buffer.resize(0x1000);
    //int nbytes = aku_json_stats(db_, buffer.data(), buffer.size());
    //if (nbytes > 0) {
    //    return std::string(buffer.data(), buffer.data() + nbytes);
    //}
    // FIXME: aku_json_stats fn
    return "nope!";
}

}
