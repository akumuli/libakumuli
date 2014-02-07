/**
 * PRIVATE HEADER
 *
 * Descriptions of internal data structures used to store data in memory mappaed files.
 * All data are in host byte order.
 *
 * Copyright (c) 2013 Eugene Lazin <4lazin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#pragma once
#include <cstdint>
#include "akumuli.h"
#include "util.h"

const int64_t AKU_MAX_PAGE_SIZE   = 0x100000000;
const int64_t AKU_MAX_PAGE_OFFSET =  0xFFFFFFFF;

namespace Akumuli {

typedef uint32_t EntryOffset;
typedef uint32_t ParamId;


/** Time duration.
 *  Difference between two timestamps.
 */
struct TimeDuration {
    int64_t value;

    static inline TimeDuration make(int64_t t) noexcept {
        TimeDuration td = {t};
        return td;
    }
};


/** Timestamp. Can be treated as
 *  single 64-bit value or two
 *  consequtive 32-bit values.
 */
struct TimeStamp {
    /** Number of microseconds since 00:00:00 january 1, 1970 UTC.
     *  Some kind of monotone sequence id can be used instead of
     *  real timestamps. Akumuli doesn't uses any kind of calendar
     *  or timezone conversion.
     */
    int64_t value;

    /** UTC timestamp of the current instant */
    static TimeStamp utc_now() noexcept;

    bool         operator  < (TimeStamp other) const noexcept;
    bool         operator  > (TimeStamp other) const noexcept;
    bool         operator == (TimeStamp other) const noexcept;
    bool         operator <= (TimeStamp other) const noexcept;
    bool         operator >= (TimeStamp other) const noexcept;
    TimeDuration operator  - (TimeStamp other) const noexcept;

    //! Maximum possible timestamp
    static const TimeStamp MAX_TIMESTAMP;

    //! Minimum possible timestamp
    static const TimeStamp MIN_TIMESTAMP;

    static inline TimeStamp make(int64_t t) noexcept {
        TimeStamp ts = {t};
        return ts;
    }
};


/** Data entry. Sensor measurement, single click from
 *  clickstream and so on. Data can be variable length,
 *  timestamp can be treated as 64-bit value (high precise
 *  timestam) or as pair of two 32-bit values: object
 *  timestamp - timestamp generated by source of data (
 *  sensor or program); server timestamp - generated by
 *  Recorder by itself, this is a time of data reception.
 */
struct Entry {
    ParamId      param_id;  //< Parameter ID
    TimeStamp        time;  //< Entry timestamp
    uint32_t       length;  //< Entry length: constant + variable sized parts
    uint32_t     value[];   //< Data begining

    //! C-tor
    Entry(uint32_t length);

    //! Extended c-tor
    Entry(ParamId param_id, TimeStamp time, uint32_t length);

    //! Calculate size needed to store data
    static uint32_t get_size(uint32_t load_size) noexcept;

    //! Return pointer to storage
    aku_MemRange get_storage() const noexcept;
};

struct Entry2 {
    uint32_t     param_id;  //< Parameter ID
    TimeStamp        time;  //< Entry timestamp
    aku_MemRange    range;  //< Data

    Entry2(ParamId param_id, TimeStamp time, aku_MemRange range);
};


//! Page types
enum PageType {
    Metadata,  //< Page with metadata used by Spatium itself
    Index      //< Index page
};


/** Page bounding box.
 *  All data is two dimentional: param-timestamp.
 */
struct PageBoundingBox {
    ParamId min_id;
    ParamId max_id;
    TimeStamp min_timestamp;
    TimeStamp max_timestamp;

    PageBoundingBox();
};


/** Page cursor
 *  used by different search methods.
 */
struct PageCursor {
    // user data
    EntryOffset*    results;        //< resulting offsets array
    uint64_t        results_cap;    //< capacity of the array
    uint64_t        results_num;    //< number of results in array
    uint32_t        done;           //< is done reading (last data writen to results array)
    // page search data
    uint32_t        start_index;    //< starting index of the traversal
    uint32_t        probe_index;    //< current index of the traversal
    uint32_t        state;          //< FSM state
    uint32_t        error_code;     //< Search error code
    // cache search data
    int64_t         cache_start_id; //< Cache generation id to start from
    uint32_t        cache_index;    //< Searched generation
    uint32_t        cache_init;     //< Initialization flag
    // generation data
    uint32_t        skip;           //< Num of elements to skip

    /** Page cursor c-tor.
     *  @param buffer receiving buffer
     *  @param buffer_size capacity of the receiving buffer
     */
    PageCursor(EntryOffset* buffer, uint64_t buffer_size) noexcept;
};


/** Cursor for single parameter time-range query */
struct SingleParameterCursor : PageCursor {
    // search query
    TimeStamp       lowerbound;     //< begining of the time interval (0 for -inf)
    TimeStamp       upperbound;     //< end of the time interval (0 for inf)
    ParamId         param;          //< parameter id
    uint32_t        direction;      //< scan direction

    /** Cursor c-tor
     *  @param pid parameter id
     *  @param low time lowerbound (0 for -inf)
     *  @param upp time upperbound (MAX_TIMESTAMP for inf)
     *  @param scan_dir scan direction
     *  @param buffer receiving buffer
     *  @param buffer_size capacity of the receiving buffer
     */
    SingleParameterCursor( ParamId      pid
                         , TimeStamp    low
                         , TimeStamp    upp
                         , uint32_t     scan_dir
                         , EntryOffset* buffer
                         , uint64_t     buffer_size ) noexcept;
};


/**
 * In-memory page representation.
 * PageHeader represents begining of the page.
 * Entry indexes grows from low to high adresses.
 * Entries placed in the bottom of the page.
 * This class must be nonvirtual.
 */
struct PageHeader {
    // metadata
    PageType type;              //< page type
    uint32_t count;             //< number of elements stored
    uint32_t last_offset;       //< index of the last added record
    uint64_t length;            //< page size
    uint32_t overwrites_count;  //< how many times page was overwriten
    uint32_t page_id;           //< page index in storage
    // NOTE: maybe it is possible to get this data from page_index?
    PageBoundingBox bbox;       //< page data limits
    EntryOffset page_index[];   //< page index

    //! Get const pointer to the begining of the page
    const char* cdata() const noexcept;

    //! Get pointer to the begining of the page
    char* data() noexcept;

    void update_bounding_box(ParamId param, TimeStamp time) noexcept;

    //! C-tor
    PageHeader(PageType type, uint32_t count, uint64_t length, uint32_t page_id);

    //! Clear all page conent (overwrite count += 1)
    void clear() noexcept;

    //! Return number of entries stored in page
    int get_entries_count() const noexcept;

    //! Returns amount of free space in bytes
    int get_free_space() const noexcept;

    bool inside_bbox(ParamId param, TimeStamp time) const noexcept;

    /**
     * Add new entry to page data.
     * @param entry entry
     * @returns operation status
     */
    int add_entry(Entry const& entry) noexcept;

    /**
     * Add new entry to page data.
     * @param entry entry
     * @returns operation status
     */
    int add_entry(Entry2 const& entry) noexcept;

    /**
     * Get length of the entry.
     * @param entry_index index of the entry.
     * @returns 0 if index is out of range, entry length otherwise.
     */
    int get_entry_length_at(int entry_index) const noexcept;

    /**
     * Get length of the entry.
     * @param offset offset of the entry.
     * @returns 0 if index is out of range, entry length otherwise.
     */
    int get_entry_length(EntryOffset offset) const noexcept;

    /**
     * Copy entry from page to receiving buffer using index.
     * @param receiver receiving buffer
     * receiver.length must contain correct buffer length
     * buffer length can be larger than sizeof(Entry)
     * @returns 0 if index out of range, -1*entry[index].length
     * if buffer is to small, entry[index].length on success.
     */
    int copy_entry_at(int index, Entry* receiver) const noexcept;

    /**
     * Copy entry from page to receiving buffer using offset.
     * @param receiver receiving buffer
     * receiver.length must contain correct buffer length
     * buffer length can be larger than sizeof(Entry)
     * @returns 0 if index out of range, -1*entry[index].length
     * if buffer is to small, entry[index].length on success.
     */
    int copy_entry(EntryOffset offset, Entry* receiver) const noexcept;

    /**
     * Get pointer to entry without copying using index
     * @param index entry index
     * @returns pointer to entry or NULL
     */
    const Entry* read_entry_at(int index) const noexcept;

    /**
     * Get pointer to entry without copying using offset
     * @param index entry index
     * @returns pointer to entry or NULL
     */
    const Entry* read_entry(EntryOffset offset) const noexcept;

    /**
     * Sort page content
     */
    void sort() noexcept;

    /**
     * TODO: remove
     */
    void partial_sort() noexcept;

    /**
     *  Binary search for entry
     *  @returns true on success
     */
    void search(SingleParameterCursor* traversal) const noexcept;

};

}  // namespaces
