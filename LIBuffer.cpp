/**
 * @file LIBuffer.cpp
 * @brief Implementation of LIBuffer class
 * @author CSCI 331 Team 2
 * @date 03/13/2026
 *
 * @details Every record uses a 2-byte big-endian uint16_t length prefix.
 *          Data records are packed as raw binary (ints, doubles, c-strings).
 *          The header record is delegated to HeaderBuffer.
 */

#include "LIBuffer.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

// ── Constructor / Destructor ──────────────────────────────────────────────────

/**
 * @brief Construct an LIBuffer for the given filename.
 * @param filename  Path to the .li data file.
 * @pre  None.
 * @post Object ready; no file opened.
 */
LIBuffer::LIBuffer(const std::string& filename)
    : filename(filename)
    , readOpen(false)
    , writeOpen(false)
    , recordCount(0)
{
}

/**
 * @brief Destructor — delegates to close().
 * @pre  None.
 * @post All open streams closed.
 */
LIBuffer::~LIBuffer()
{
    close();
}

// ── File lifecycle ────────────────────────────────────────────────────────────

/**
 * @brief Open the file for sequential reading.
 * @return true on success; false if file cannot be opened.
 * @pre  None.
 * @post inStream open in binary mode at byte 0; readOpen == true.
 */
bool LIBuffer::openForRead()
{
    close();
    inStream.open(filename, std::ios::binary);
    if (!inStream.is_open())
    {
        std::cerr << "LIBuffer: cannot open \"" << filename << "\" for reading\n";
        return false;
    }
    readOpen    = true;
    recordCount = 0;
    return true;
}

/**
 * @brief Open (or create) the file for sequential writing, truncating content.
 * @return true on success; false if file cannot be opened.
 * @pre  None.
 * @post outStream open in binary mode; writeOpen == true.
 */
bool LIBuffer::openForWrite()
{
    close();
    outStream.open(filename, std::ios::binary | std::ios::trunc);
    if (!outStream.is_open())
    {
        std::cerr << "LIBuffer: cannot open \"" << filename << "\" for writing\n";
        return false;
    }
    writeOpen   = true;
    recordCount = 0;
    return true;
}

/**
 * @brief Close any open streams.
 * @pre  None.
 * @post Both streams closed; flags reset.
 */
void LIBuffer::close()
{
    if (readOpen  && inStream.is_open())  inStream.close();
    if (writeOpen && outStream.is_open()) outStream.close();
    readOpen  = false;
    writeOpen = false;
}

// ── Private helpers ───────────────────────────────────────────────────────────

/**
 * @brief Calculate packed byte size of a ZipCodeRecord.
 * @param record  Record to measure.
 * @pre  None.
 * @post No state change.
 * @return Required buffer size in bytes.
 */
size_t LIBuffer::getRecordPackedSize(const ZipCodeRecord& record) const
{
    // Build the CSV string and return its length
    // Format: ZZZZZ,placeName,state,county,lat,lon
    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << record.zipCode
        << "," << record.placeName
        << "," << record.state
        << "," << record.county
        << "," << std::fixed << std::setprecision(4) << record.latitude
        << "," << std::fixed << std::setprecision(4) << record.longitude;
    return oss.str().length();
}

/**
 * @brief Pack a ZipCodeRecord into a raw byte buffer.
 * @param record  Record to pack.
 * @param buffer  Caller-allocated buffer of at least getRecordPackedSize() bytes.
 * @pre  buffer is valid and large enough.
 * @post All fields written to buffer.
 * @return Number of bytes written.
 */
size_t LIBuffer::packRecord(const ZipCodeRecord& record, char* buffer) const
{
    // Serialise as comma-separated ASCII text
    // Format: ZZZZZ,placeName,state,county,lat,lon
    std::ostringstream oss;
    oss << std::setw(5) << std::setfill('0') << record.zipCode
        << "," << record.placeName
        << "," << record.state
        << "," << record.county
        << "," << std::fixed << std::setprecision(4) << record.latitude
        << "," << std::fixed << std::setprecision(4) << record.longitude;

    const std::string payload = oss.str();
    std::memcpy(buffer, payload.c_str(), payload.length());
    return payload.length();
}

/**
 * @brief Unpack a ZipCodeRecord from a raw byte buffer.
 * @param buffer  Buffer produced by packRecord().
 * @param record  Output: populated on success.
 * @pre  buffer contains a valid packed record.
 * @post record fully populated.
 * @return Number of bytes consumed.
 */
size_t LIBuffer::unpackRecord(const char* buffer, ZipCodeRecord& record, size_t len) const
{
    // Parse comma-separated ASCII text payload
    // Format: ZZZZZ,placeName,state,county,lat,lon
    std::string payload(buffer, len);
    std::istringstream iss(payload);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ','))
        tokens.push_back(token);

    if (tokens.size() < 6)
    {
        std::cerr << "LIBuffer::unpackRecord -- malformed payload: "
                  << payload.substr(0, 40) << "\n";
        return 0;
    }

    record.zipCode   = std::stoi(tokens[0]);
    record.placeName = tokens[1];
    record.state     = tokens[2];
    record.county    = tokens[3];
    record.latitude  = std::stod(tokens[4]);
    record.longitude = std::stod(tokens[5]);

    return payload.length();
}

/**
 * @brief Read a 2-byte big-endian length prefix from the read stream.
 * @param length  Output: decoded length.
 * @return true on success; false on EOF or I/O error.
 * @pre  inStream open and positioned at a length prefix.
 * @post Stream advances 2 bytes.
 */
bool LIBuffer::readLength(uint16_t& length)
{
    uint8_t hi = 0, lo = 0;
    inStream.get(reinterpret_cast<char&>(hi));
    inStream.get(reinterpret_cast<char&>(lo));
    if (!inStream.good()) return false;
    length = static_cast<uint16_t>((hi << 8) | lo);
    return true;
}

/**
 * @brief Write a 2-byte big-endian length prefix to the write stream.
 * @param length  Value to encode.
 * @return true on success; false on I/O error.
 * @pre  outStream open in binary write mode.
 * @post Stream advances 2 bytes.
 */
bool LIBuffer::writeLength(uint16_t length)
{
    const uint8_t hi = static_cast<uint8_t>((length >> 8) & 0xFF);
    const uint8_t lo = static_cast<uint8_t>(length & 0xFF);
    outStream.put(static_cast<char>(hi));
    outStream.put(static_cast<char>(lo));
    return outStream.good();
}

// ── Read interface ────────────────────────────────────────────────────────────

/**
 * @brief Read and unpack the header record.
 * @param header  Output: populated with file metadata.
 * @return true on success; false on error.
 * @pre  openForRead() returned true; stream at byte 0.
 * @post header populated; stream advances past header record.
 */
bool LIBuffer::readHeader(DataFileHeader& header)
{
    if (!readOpen)
    {
        std::cerr << "LIBuffer::readHeader -- not open for reading\n";
        return false;
    }
    return headerBuf.readHeader(inStream, header);
}

/**
 * @brief Read and unpack the next data record.
 * @param record  Output: populated on success.
 * @return true on success; false at EOF or on error.
 * @pre  openForRead() and readHeader() called.
 * @post record populated; recordCount incremented; stream advances.
 */
bool LIBuffer::getNextRecord(ZipCodeRecord& record)
{
    if (!readOpen || !inStream.is_open()) return false;

    uint16_t len = 0;
    if (!readLength(len)) return false;   // EOF

    char* buffer = new char[len];
    inStream.read(buffer, len);

    if (inStream.gcount() != static_cast<std::streamsize>(len))
    {
        std::cerr << "LIBuffer::getNextRecord -- truncated record\n";
        delete[] buffer;
        return false;
    }

    unpackRecord(buffer, record, static_cast<size_t>(len));
    delete[] buffer;
    recordCount++;
    return true;
}

/**
 * @brief Seek to a byte offset and read exactly one data record.
 * @param offset  Byte offset of the record's 2-byte length prefix.
 * @param record  Output: populated on success.
 * @return true on success; false on seek/read/unpack error.
 * @pre  openForRead() returned true.
 * @post record populated; stream positioned after the record.
 */
bool LIBuffer::readRecordAt(std::streamoff offset, ZipCodeRecord& record)
{
    if (!readOpen || !inStream.is_open())
    {
        std::cerr << "LIBuffer::readRecordAt -- not open for reading\n";
        return false;
    }

    inStream.clear();
    inStream.seekg(offset, std::ios::beg);
    if (!inStream.good())
    {
        std::cerr << "LIBuffer::readRecordAt -- seek failed to offset " << offset << "\n";
        return false;
    }

    uint16_t len = 0;
    if (!readLength(len))
    {
        std::cerr << "LIBuffer::readRecordAt -- cannot read length at offset " << offset << "\n";
        return false;
    }

    char* buffer = new char[len];
    inStream.read(buffer, len);

    if (inStream.gcount() != static_cast<std::streamsize>(len))
    {
        std::cerr << "LIBuffer::readRecordAt -- truncated payload\n";
        delete[] buffer;
        return false;
    }

    unpackRecord(buffer, record, static_cast<size_t>(len));
    delete[] buffer;
    return true;
}

// ── Write interface ───────────────────────────────────────────────────────────

/**
 * @brief Write the header record as the first LI record.
 * @param header  Metadata to write.
 * @return true on success; false on I/O error.
 * @pre  openForWrite() returned true; must be first write call.
 * @post Header written; stream advances.
 */
bool LIBuffer::writeHeader(DataFileHeader& header)
{
    if (!writeOpen)
    {
        std::cerr << "LIBuffer::writeHeader -- not open for writing\n";
        return false;
    }
    return headerBuf.writeHeader(outStream, header);
}

/**
 * @brief Serialise and append one ZipCodeRecord.
 * @param record  ZIP code data to write.
 * @return true on success; false on I/O error.
 * @pre  openForWrite() and writeHeader() called.
 * @post One LI data record appended; recordCount incremented.
 */
bool LIBuffer::writeRecord(const ZipCodeRecord& record)
{
    if (!writeOpen || !outStream.is_open())
    {
        std::cerr << "LIBuffer::writeRecord -- not open for writing\n";
        return false;
    }

    size_t payloadLen = getRecordPackedSize(record);
    if (payloadLen > 0xFFFFu)
    {
        std::cerr << "LIBuffer::writeRecord -- payload too large\n";
        return false;
    }

    char* buffer = new char[payloadLen];
    packRecord(record, buffer);

    if (!writeLength(static_cast<uint16_t>(payloadLen)))
    {
        delete[] buffer;
        return false;
    }

    outStream.write(buffer, static_cast<std::streamsize>(payloadLen));
    delete[] buffer;

    if (!outStream.good())
    {
        std::cerr << "LIBuffer::writeRecord -- write error\n";
        return false;
    }

    recordCount++;
    return true;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

/**
 * @brief Return the current byte offset of the read stream.
 * @pre  openForRead() called.
 * @post No state change.
 * @return Byte offset; -1 if not open.
 */
std::streamoff LIBuffer::getCurrentOffset() const
{
    if (readOpen && inStream.is_open())
        return const_cast<std::ifstream&>(inStream).tellg();
    return -1;
}

/**
 * @brief Return the cumulative data record count.
 * @pre  None.
 * @post No state change.
 * @return recordCount.
 */
long LIBuffer::getRecordCount() const { return recordCount; }

/**
 * @brief Return the filename.
 * @pre  None.
 * @post No state change.
 * @return filename string.
 */
std::string LIBuffer::getFilename() const { return filename; }