/**
 * @file HeaderBuffer.cpp
 * @brief Implementation of HeaderBuffer class
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details All records in the .li file use a 2-byte big-endian uint16_t
 *          length prefix.  The header record is no different — it is simply
 *          the first one in the file.
 *
 *          Write sequence:
 *            1. Call DataFileHeader::getPackedSize() to find payload length
 *            2. Allocate a buffer and call DataFileHeader::pack()
 *            3. Write 2-byte big-endian length prefix
 *            4. Write payload bytes
 *
 *          Read sequence:
 *            1. Read 2 bytes -> decode big-endian length
 *            2. Allocate buffer of that size, read payload bytes
 *            3. Call DataFileHeader::unpack()
 */

#include "HeaderBuffer.h"
#include <iostream>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// writeHeader
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Serialise header and write it as the first LI record.
 * @param out     Open binary ofstream at byte 0.
 * @param header  Header to write.
 * @return true on success; false on any error.
 * @pre  out is open in binary mode at position 0.
 * @post Header occupies first (2 + payloadLen) bytes; stream advances.
 */
bool HeaderBuffer::writeHeader(std::ofstream& out, DataFileHeader& header)
{
    if (!out.is_open())
    {
        std::cerr << "HeaderBuffer::writeHeader -- stream is not open\n";
        return false;
    }

    // ── Step 1: calculate packed size and update headerSize field ──────────
    size_t payloadLen = header.getPackedSize();
    header.headerSize = static_cast<int>(payloadLen);   // store for reference

    if (payloadLen > 0xFFFFu)
    {
        std::cerr << "HeaderBuffer::writeHeader -- payload too large ("
                  << payloadLen << " bytes)\n";
        return false;
    }

    // ── Step 2: pack into a buffer ─────────────────────────────────────────
    char* buffer = new char[payloadLen];
    header.pack(buffer);

    // ── Step 3: write 2-byte big-endian length prefix ─────────────────────
    const uint8_t hi = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);
    const uint8_t lo = static_cast<uint8_t>(payloadLen & 0xFF);
    out.put(static_cast<char>(hi));
    out.put(static_cast<char>(lo));

    // ── Step 4: write payload ─────────────────────────────────────────────
    out.write(buffer, static_cast<std::streamsize>(payloadLen));
    delete[] buffer;

    if (!out.good())
    {
        std::cerr << "HeaderBuffer::writeHeader -- write error\n";
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// readHeader
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Read first LI record and unpack it into a DataFileHeader.
 * @param in      Open binary ifstream at byte 0.
 * @param header  Reference to populate.
 * @return true on success; false on any error.
 * @pre  in is open in binary mode at position 0.
 * @post header is fully populated; stream advances past header record.
 */
bool HeaderBuffer::readHeader(std::ifstream& in, DataFileHeader& header)
{
    if (!in.is_open())
    {
        std::cerr << "HeaderBuffer::readHeader -- stream is not open\n";
        return false;
    }

    // ── Step 1: read 2-byte big-endian length prefix ──────────────────────
    uint8_t hi = 0, lo = 0;
    in.get(reinterpret_cast<char&>(hi));
    in.get(reinterpret_cast<char&>(lo));

    if (!in.good())
    {
        std::cerr << "HeaderBuffer::readHeader -- could not read length prefix\n";
        return false;
    }

    const uint16_t payloadLen = static_cast<uint16_t>((hi << 8) | lo);

    // ── Step 2: read payload bytes ────────────────────────────────────────
    char* buffer = new char[payloadLen];
    in.read(buffer, payloadLen);

    if (in.gcount() != static_cast<std::streamsize>(payloadLen))
    {
        std::cerr << "HeaderBuffer::readHeader -- truncated payload\n";
        delete[] buffer;
        return false;
    }

    // ── Step 3: unpack into header ────────────────────────────────────────
    header.unpack(buffer);
    delete[] buffer;

    return true;
}