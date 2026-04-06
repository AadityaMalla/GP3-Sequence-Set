/**
 * @file RecordBuffer.cpp
 * @brief Implementation of RecordBuffer
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "RecordBuffer.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <iostream>

// -----------------------------------------------------------------------------
// buildPayload
// -----------------------------------------------------------------------------

std::string RecordBuffer::buildPayload(const ZipCodeRecord& record) const
{
    std::ostringstream oss;

    oss << std::setw(5) << std::setfill('0') << record.zipCode
        << "," << record.placeName
        << "," << record.state
        << "," << record.county
        << "," << std::fixed << std::setprecision(4) << record.latitude
        << "," << std::fixed << std::setprecision(4) << record.longitude;

    return oss.str();
}

// -----------------------------------------------------------------------------
// getPackedSize
// -----------------------------------------------------------------------------

size_t RecordBuffer::getPackedSize(const ZipCodeRecord& record) const
{
    const std::string payload = buildPayload(record);
    return 2 + payload.length(); // 2-byte length prefix + payload
}

// -----------------------------------------------------------------------------
// writeLength
// -----------------------------------------------------------------------------

void RecordBuffer::writeLength(uint16_t length, char* buffer) const
{
    buffer[0] = static_cast<char>((length >> 8) & 0xFF);
    buffer[1] = static_cast<char>(length & 0xFF);
}

// -----------------------------------------------------------------------------
// readLength
// -----------------------------------------------------------------------------

uint16_t RecordBuffer::readLength(const char* buffer) const
{
    const uint8_t hi = static_cast<uint8_t>(buffer[0]);
    const uint8_t lo = static_cast<uint8_t>(buffer[1]);
    return static_cast<uint16_t>((hi << 8) | lo);
}

// -----------------------------------------------------------------------------
// pack
// -----------------------------------------------------------------------------

size_t RecordBuffer::pack(const ZipCodeRecord& record, char* buffer) const
{
    const std::string payload = buildPayload(record);
    const uint16_t payloadLen = static_cast<uint16_t>(payload.length());

    writeLength(payloadLen, buffer);
    std::memcpy(buffer + 2, payload.c_str(), payloadLen);

    return 2 + payloadLen;
}

// -----------------------------------------------------------------------------
// parsePayload
// -----------------------------------------------------------------------------

bool RecordBuffer::parsePayload(const std::string& payload,
                                ZipCodeRecord& record) const
{
    std::istringstream iss(payload);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ','))
        tokens.push_back(token);

    if (tokens.size() < 6)
    {
        std::cerr << "RecordBuffer::parsePayload -- malformed payload: "
                  << payload << "\n";
        return false;
    }

    try
    {
        record.zipCode   = std::stoi(tokens[0]);
        record.placeName = tokens[1];
        record.state     = tokens[2];
        record.county    = tokens[3];
        record.latitude  = std::stod(tokens[4]);
        record.longitude = std::stod(tokens[5]);
    }
    catch (const std::exception&)
    {
        std::cerr << "RecordBuffer::parsePayload -- conversion error: "
                  << payload << "\n";
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// unpack
// -----------------------------------------------------------------------------

size_t RecordBuffer::unpack(const char* buffer, ZipCodeRecord& record) const
{
    const uint16_t payloadLen = readLength(buffer);

    if (payloadLen == 0)
    {
        std::cerr << "RecordBuffer::unpack -- zero-length payload\n";
        return 0;
    }

    const std::string payload(buffer + 2, buffer + 2 + payloadLen);

    if (!parsePayload(payload, record))
        return 0;

    return 2 + payloadLen;
}