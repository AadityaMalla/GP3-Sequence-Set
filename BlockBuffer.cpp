/**
 * @file BlockBuffer.cpp
 * @brief Implementation of BlockBuffer
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "BlockBuffer.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

BlockBuffer::BlockBuffer(int blockSize)
    : blockSize(blockSize)
    , recordCount(0)
    , prevRBN(-1)
    , nextRBN(-1)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic state resets
// ─────────────────────────────────────────────────────────────────────────────

void BlockBuffer::clearToEmptyActive()
{
    recordCount = 0;
    prevRBN = -1;
    nextRBN = -1;
    records.clear();
}

void BlockBuffer::clearToAvail(int nextAvailRBN)
{
    recordCount = 0;
    prevRBN = -1;
    nextRBN = nextAvailRBN;
    records.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// State checks
// ─────────────────────────────────────────────────────────────────────────────

bool BlockBuffer::isAvailBlock() const
{
    return recordCount == 0 && records.empty();
}

bool BlockBuffer::isActiveBlock() const
{
    return !records.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

int BlockBuffer::getBlockSize() const
{
    return blockSize;
}

int BlockBuffer::getRecordCount() const
{
    return recordCount;
}

void BlockBuffer::setPrevRBN(int rbn)
{
    prevRBN = rbn;
}

void BlockBuffer::setNextRBN(int rbn)
{
    nextRBN = rbn;
}

int BlockBuffer::getPrevRBN() const
{
    return prevRBN;
}

int BlockBuffer::getNextRBN() const
{
    return nextRBN;
}

const std::vector<ZipCodeRecord>& BlockBuffer::getRecords() const
{
    return records;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

size_t BlockBuffer::getBlockHeaderSize() const
{
    // recordCount + prevRBN + nextRBN
    return sizeof(int32_t) * 3;
}

void BlockBuffer::sortRecords()
{
    std::sort(records.begin(), records.end(),
              [](const ZipCodeRecord& a, const ZipCodeRecord& b)
              {
                  return a.zipCode < b.zipCode;
              });
}

// ─────────────────────────────────────────────────────────────────────────────
// Space calculations
// ─────────────────────────────────────────────────────────────────────────────

size_t BlockBuffer::getUsedBytes() const
{
    size_t used = getBlockHeaderSize();

    for (const auto& rec : records)
        used += recordBuffer.getPackedSize(rec);

    return used;
}

size_t BlockBuffer::getFreeBytes() const
{
    const size_t used = getUsedBytes();
    return (used <= static_cast<size_t>(blockSize))
        ? static_cast<size_t>(blockSize) - used
        : 0;
}

bool BlockBuffer::canFit(const ZipCodeRecord& record) const
{
    return recordBuffer.getPackedSize(record) <= getFreeBytes();
}

// ─────────────────────────────────────────────────────────────────────────────
// Record operations
// ─────────────────────────────────────────────────────────────────────────────

bool BlockBuffer::insertRecordSorted(const ZipCodeRecord& record)
{
    // Reject duplicate ZIPs
    for (const auto& rec : records)
    {
        if (rec.zipCode == record.zipCode)
            return false;
    }

    if (!canFit(record))
        return false;

    records.push_back(record);
    sortRecords();
    recordCount = static_cast<int>(records.size());
    return true;
}

bool BlockBuffer::removeRecordByZip(int zipCode)
{
    auto it = std::find_if(records.begin(), records.end(),
                           [zipCode](const ZipCodeRecord& rec)
                           {
                               return rec.zipCode == zipCode;
                           });

    if (it == records.end())
        return false;

    records.erase(it);
    recordCount = static_cast<int>(records.size());
    return true;
}

bool BlockBuffer::findRecord(int zipCode, ZipCodeRecord& record) const
{
    auto it = std::find_if(records.begin(), records.end(),
                           [zipCode](const ZipCodeRecord& rec)
                           {
                               return rec.zipCode == zipCode;
                           });

    if (it == records.end())
        return false;

    record = *it;
    return true;
}

int BlockBuffer::getHighestKey() const
{
    if (records.empty())
        return -1;

    return records.back().zipCode;
}

// ─────────────────────────────────────────────────────────────────────────────
// pack
// ─────────────────────────────────────────────────────────────────────────────

bool BlockBuffer::pack(char* buffer) const
{
    std::memset(buffer, ' ', blockSize);

    char* p = buffer;

    int32_t rc   = static_cast<int32_t>(recordCount);
    int32_t prev = static_cast<int32_t>(prevRBN);
    int32_t next = static_cast<int32_t>(nextRBN);

    std::memcpy(p, &rc, sizeof(rc));
    p += sizeof(rc);

    std::memcpy(p, &prev, sizeof(prev));
    p += sizeof(prev);

    std::memcpy(p, &next, sizeof(next));
    p += sizeof(next);

    // Avail block: no records written
    if (recordCount == 0 && records.empty())
        return true;

    for (const auto& rec : records)
    {
        const size_t packedSize = recordBuffer.getPackedSize(rec);

        if ((p - buffer) + static_cast<std::ptrdiff_t>(packedSize) > blockSize)
            return false;

        recordBuffer.pack(rec, p);
        p += packedSize;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// unpack
// ─────────────────────────────────────────────────────────────────────────────

bool BlockBuffer::unpack(const char* buffer)
{
    records.clear();

    const char* p = buffer;

    int32_t rc = 0;
    int32_t prev = -1;
    int32_t next = -1;

    std::memcpy(&rc, p, sizeof(rc));
    p += sizeof(rc);

    std::memcpy(&prev, p, sizeof(prev));
    p += sizeof(prev);

    std::memcpy(&next, p, sizeof(next));
    p += sizeof(next);

    recordCount = static_cast<int>(rc);
    prevRBN = static_cast<int>(prev);
    nextRBN = static_cast<int>(next);

    // Avail block
    if (recordCount == 0)
        return true;

    for (int i = 0; i < recordCount; ++i)
    {
        if ((p - buffer) + 2 > blockSize)
            return false;

        const uint16_t payloadLen = recordBuffer.readLength(p);
        const size_t packedSize = 2 + payloadLen;

        if ((p - buffer) + static_cast<std::ptrdiff_t>(packedSize) > blockSize)
            return false;

        ZipCodeRecord rec;
        if (recordBuffer.unpack(p, rec) == 0)
            return false;

        records.push_back(rec);
        p += packedSize;
    }

    sortRecords();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// display
// ─────────────────────────────────────────────────────────────────────────────

void BlockBuffer::display() const
{
    std::cout << "==========================================\n";
    std::cout << "BlockBuffer\n";
    std::cout << "==========================================\n";
    std::cout << "Block Size   : " << blockSize << "\n";
    std::cout << "Record Count : " << recordCount << "\n";
    std::cout << "Prev RBN     : " << prevRBN << "\n";
    std::cout << "Next RBN     : " << nextRBN << "\n";
    std::cout << "Used Bytes   : " << getUsedBytes() << "\n";
    std::cout << "Free Bytes   : " << getFreeBytes() << "\n";

    if (records.empty())
    {
        std::cout << "Records      : (none)\n";
    }
    else
    {
        std::cout << "Records:\n";
        for (size_t i = 0; i < records.size(); ++i)
        {
            std::cout << "  [" << i << "] ";
            records[i].displayInline();
            std::cout << "\n";
        }
    }

    std::cout << "==========================================\n";
}