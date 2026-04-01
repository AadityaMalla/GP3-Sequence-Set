/**
 * @file RecordBuffer.h
 * @brief Packs and unpacks one ZIP code record for blocked sequence-set storage
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 *
 * @details
 * This class is responsible for converting a ZipCodeRecord to and from
 * a raw byte representation stored inside a block.
 *
 * GP 3.0 still expects the record fields to remain comma-separated in
 * logical form, so this buffer stores each record as:
 *
 *   [2-byte big-endian payload length]
 *   [ASCII payload: ZZZZZ,place,state,county,lat,lon]
 *
 * This allows records to remain variable-length while still being easy
 * to pack into a fixed-size block.
 */

#ifndef RECORDBUFFER_H
#define RECORDBUFFER_H

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include "ZipCodeRecord.h"

/**
 * @class RecordBuffer
 * @brief Converts ZipCodeRecord objects to/from raw bytes for block storage.
 *
 * @details
 * Responsibilities:
 *   - compute packed size of one record
 *   - pack one record into a caller-provided buffer
 *   - unpack one record from a raw buffer
 *   - encode/decode the 2-byte big-endian length prefix
 *
 * NOT responsible for:
 *   - file I/O
 *   - whole-block management
 *   - indexing
 */
class RecordBuffer
{
public:
    /**
     * @brief Default constructor.
     */
    RecordBuffer() = default;

    /**
     * @brief Default destructor.
     */
    ~RecordBuffer() = default;

    /**
     * @brief Build the ASCII payload for a record.
     * @param record  Input ZIP code record.
     * @return Comma-separated ASCII payload string.
     */
    std::string buildPayload(const ZipCodeRecord& record) const;

    /**
     * @brief Return the number of bytes needed to store this record,
     *        including the 2-byte length prefix.
     * @param record  Input ZIP code record.
     * @return Total packed byte count.
     */
    size_t getPackedSize(const ZipCodeRecord& record) const;

    /**
     * @brief Pack a record into a raw byte buffer.
     * @param record  Input ZIP code record.
     * @param buffer  Caller-allocated output buffer.
     * @return Number of bytes written.
     *
     * @pre buffer must have at least getPackedSize(record) bytes.
     * @post buffer contains [2-byte len][ASCII payload].
     */
    size_t pack(const ZipCodeRecord& record, char* buffer) const;

    /**
     * @brief Unpack a record from a raw byte buffer.
     * @param buffer  Input raw bytes beginning at a length prefix.
     * @param record  Output record.
     * @return Number of bytes consumed, or 0 on error.
     *
     * @pre buffer points to a valid packed record.
     * @post record populated if return value > 0.
     */
    size_t unpack(const char* buffer, ZipCodeRecord& record) const;

    /**
     * @brief Read only the payload length from a packed record.
     * @param buffer  Input raw bytes beginning at a length prefix.
     * @return Payload length in bytes.
     */
    uint16_t readLength(const char* buffer) const;

    /**
     * @brief Write a 2-byte big-endian length prefix.
     * @param length  Payload length.
     * @param buffer  Output buffer with space for at least 2 bytes.
     */
    void writeLength(uint16_t length, char* buffer) const;

private:
    /**
     * @brief Parse a comma-separated payload into a ZipCodeRecord.
     * @param payload  ASCII payload string.
     * @param record   Output record.
     * @return true on success; false on malformed input.
     */
    bool parsePayload(const std::string& payload, ZipCodeRecord& record) const;
};

#endif // RECORDBUFFER_H