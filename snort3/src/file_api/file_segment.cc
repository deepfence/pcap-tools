//--------------------------------------------------------------------------
// Copyright (C) 2015-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
//  file_segment.cc author Hui Cao <huica@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "file_segment.h"

#include "file_lib.h"

using namespace snort;

FileSegment::~FileSegment ()
{
    if (data)
        delete data;
}

FileSegments::FileSegments (FileContext* ctx)
{
    head = nullptr;
    current_offset = 0;
    context =  ctx;
}

FileSegments::~FileSegments ()
{
    clear();
}

void FileSegments::clear()
{
    FileSegment* current_segment = head;

    while (current_segment)
    {
        FileSegment* previous_segment = current_segment;
        current_segment = current_segment->next;
        delete previous_segment;
    }

    head = nullptr;
    current_offset = 0;
}

// Update the segment list based on new data
void FileSegments::add(const uint8_t* file_data, uint64_t data_size, uint64_t offset)
{
    FileSegment* new_segment = new FileSegment();
    new_segment->offset = offset;
    new_segment->data = new std::string((const char*)file_data, data_size);

    if (!head)
    {
        head = new_segment;
        return;
    }

    FileSegment* current_segment = head;
    uint64_t start = offset;
    uint64_t end = offset + data_size;
    // left points to segment that "next" pointer needs to be updated
    FileSegment* left = nullptr;
    FileSegment* previous = nullptr;
    bool find_left = false;
    bool is_overlap = false;

    // Find left boundary, left points to segment that needs update
    while (current_segment)
    {
        if (current_segment->offset > start)
        {
            find_left = true;
            left = previous;
            break;
        }

        previous = current_segment;
        current_segment = current_segment->next;
    }

    // New segment should be at the end of link list
    if (!find_left)
    {
        previous->next = new_segment;
    }
    // New segment should be at the start of link list
    else if (!left)
    {
        if (end <= head->offset)
        {
            new_segment->next = head;
            head = new_segment;
        }
        else
        {
            is_overlap = true;
        }
    }
    else
    {
        if ((left->offset + left->data->size() > start) ||
            (left->next->offset < end))
        {
            is_overlap = true;
        }
        else
        {
            new_segment->next = left->next;
            left->next = new_segment;
        }
    }

    // ignore overlap case
    if (is_overlap)
    {
        delete new_segment;
        return;
    }
}

FilePosition FileSegments::get_file_position(uint64_t data_size, uint64_t file_size)
{
    if (current_offset == 0)
    {
        if (file_size == data_size)
            return SNORT_FILE_FULL;
        else
            return SNORT_FILE_START;
    }

    if (file_size <= data_size + current_offset)
        return SNORT_FILE_END;

    return SNORT_FILE_MIDDLE;
}

int FileSegments::process_one(Packet* p, const uint8_t* file_data, int data_size,
    FilePolicyBase* policy, FilePosition position)
{
    if (position == SNORT_FILE_POSITION_UNKNOWN)
        position = get_file_position(data_size, context->get_file_size());

    return context->process(p, file_data, data_size, position, policy);
}

int FileSegments::process_all(Packet* p, FilePolicyBase* policy)
{
    int ret = 1;

    FileSegment* current_segment = head;
    while (current_segment && (current_offset == current_segment->offset))
    {
        ret = process_one(p, (const uint8_t*)current_segment->data->data(),
            current_segment->data->size(), policy);

        if (!ret)
        {
            clear();
            break;
        }

        current_offset += current_segment->data->size();
        head = current_segment->next;
        delete(current_segment);
        current_segment = head;
    }

    return ret;
}

/*
 * Process file segment, do file segment reassemble if the file segment is
 * out of order.
 * Return:
 *    1: continue processing/log/block this file
 *    0: ignore this file
 */
int FileSegments::process(Packet* p, const uint8_t* file_data, uint64_t data_size,
    uint64_t offset, FilePolicyBase* policy, FilePosition position)
{
    int ret = 0;

    if (offset < current_offset)
    {
        if (offset + data_size > current_offset)
        {
            file_data += (current_offset - offset);
            data_size = (offset + data_size) - current_offset;
            offset = current_offset;
        }
        else
        {
            return 1;
        }
    }

    // Walk through the segments that can be flushed
    if (current_offset == offset)
    {
        ret =  process_one(p, file_data, data_size, policy, position);
        current_offset += data_size;
        if (!ret)
        {
            clear();
            return 0;
        }

        ret = process_all(p, policy);
    }
    else if ((current_offset < context->get_file_size()) && (current_offset < offset))
    {
        add(file_data, data_size, offset);
        return 1;
    }

    return ret;
}

