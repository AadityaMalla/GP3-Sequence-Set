/**
 * @file BlockFileBuffer.cpp
 * @brief Implementation of BlockFileBuffer
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "BlockFileBuffer.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

BlockFileBuffer::BlockFileBuffer(const std::string& filename)
    : filename(filename)
    , readOpen(false)
    , writeOpen(false)
    , headerEndPos(0)
{
}

BlockFileBuffer::~BlockFileBuffer()
{
    close();
}

// -----------------------------------------------------------------------------
// File lifecycle
// -----------------------------------------------------------------------------

bool BlockFileBuffer::openForRead()
{
    close();
    inStream.open(filename, std::ios::binary);
    if (!inStream.is_open())
    {
        std::cerr << "BlockFileBuffer::openForRead -- cannot open \""
                  << filename << "\"\n";
        return false;
    }

    readOpen = true;
    return true;
}

bool BlockFileBuffer::openForWrite()
{
    close();
    ioStream.open(filename,
                  std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);

    if (!ioStream.is_open())
    {
        // Create file first if needed, then reopen read/write
        std::ofstream create(filename, std::ios::binary | std::ios::trunc);
        create.close();

        ioStream.open(filename,
                      std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    }

    if (!ioStream.is_open())
    {
        std::cerr << "BlockFileBuffer::openForWrite -- cannot open \""
                  << filename << "\"\n";
        return false;
    }

    writeOpen = true;
    return true;
}

bool BlockFileBuffer::openForUpdate()
{
    close();
    ioStream.open(filename, std::ios::binary | std::ios::in | std::ios::out);

    if (!ioStream.is_open())
    {
        std::cerr << "BlockFileBuffer::openForUpdate -- cannot open \""
                  << filename << "\"\n";
        return false;
    }

    writeOpen = true;
    return true;
}

void BlockFileBuffer::close()
{
    if (readOpen && inStream.is_open())
        inStream.close();

    if (writeOpen && ioStream.is_open())
        ioStream.close();

    readOpen = false;
    writeOpen = false;
    headerEndPos = 0;
}

// -----------------------------------------------------------------------------
// Length helpers
// -----------------------------------------------------------------------------

bool BlockFileBuffer::readLength(std::istream& in, uint16_t& length)
{
    uint8_t hi = 0;
    uint8_t lo = 0;

    in.read(reinterpret_cast<char*>(&hi), 1);
    in.read(reinterpret_cast<char*>(&lo), 1);

    if (!in.good())
        return false;

    length = static_cast<uint16_t>((hi << 8) | lo);
    return true;
}

bool BlockFileBuffer::writeLength(std::ostream& out, uint16_t length)
{
    const uint8_t hi = static_cast<uint8_t>((length >> 8) & 0xFF);
    const uint8_t lo = static_cast<uint8_t>(length & 0xFF);

    out.write(reinterpret_cast<const char*>(&hi), 1);
    out.write(reinterpret_cast<const char*>(&lo), 1);

    return out.good();
}

// -----------------------------------------------------------------------------
// Header I/O
// -----------------------------------------------------------------------------

bool BlockFileBuffer::writeHeader(BlockedFileHeader& header)
{
    if (!writeOpen)
    {
        std::cerr << "BlockFileBuffer::writeHeader -- file not open for writing\n";
        return false;
    }

    const size_t payloadLen = header.getPackedSize();
    if (payloadLen > 0xFFFFu)
    {
        std::cerr << "BlockFileBuffer::writeHeader -- header too large\n";
        return false;
    }

    header.headerSize = static_cast<int>(payloadLen);

    std::vector<char> buffer(payloadLen);
    header.pack(buffer.data());

    ioStream.seekp(0, std::ios::beg);

    if (!writeLength(ioStream, static_cast<uint16_t>(payloadLen)))
    {
        std::cerr << "BlockFileBuffer::writeHeader -- failed writing header length\n";
        return false;
    }

    ioStream.write(buffer.data(), static_cast<std::streamsize>(payloadLen));
    if (!ioStream.good())
    {
        std::cerr << "BlockFileBuffer::writeHeader -- failed writing header payload\n";
        return false;
    }

    headerEndPos = static_cast<std::streamoff>(2 + payloadLen);
    ioStream.flush();
    return true;
}

bool BlockFileBuffer::readHeader(BlockedFileHeader& header)
{
    std::istream* in = nullptr;

    if (readOpen)
    {
        in = &inStream;
    }
    else if (writeOpen)
    {
        ioStream.seekg(0, std::ios::beg);
        in = &ioStream;
    }
    else
    {
        std::cerr << "BlockFileBuffer::readHeader -- file not open\n";
        return false;
    }

    in->clear();
    in->seekg(0, std::ios::beg);

    uint16_t payloadLen = 0;
    if (!readLength(*in, payloadLen))
    {
        std::cerr << "BlockFileBuffer::readHeader -- failed reading header length\n";
        return false;
    }

    std::vector<char> buffer(payloadLen);
    in->read(buffer.data(), static_cast<std::streamsize>(payloadLen));

    if (in->gcount() != static_cast<std::streamsize>(payloadLen))
    {
        std::cerr << "BlockFileBuffer::readHeader -- truncated header payload\n";
        return false;
    }

    header.unpack(buffer.data());
    headerEndPos = static_cast<std::streamoff>(2 + payloadLen);
    return true;
}

// -----------------------------------------------------------------------------
// Block offsets
// -----------------------------------------------------------------------------

std::streamoff BlockFileBuffer::getBlockOffset(int rbn, int blockSize) const
{
    return headerEndPos + static_cast<std::streamoff>(rbn) * blockSize;
}

// -----------------------------------------------------------------------------
// Block I/O
// -----------------------------------------------------------------------------

bool BlockFileBuffer::writeBlock(int rbn, const BlockBuffer& block)
{
    if (!writeOpen)
    {
        std::cerr << "BlockFileBuffer::writeBlock -- file not open for writing\n";
        return false;
    }

    std::vector<char> buffer(block.getBlockSize());
    if (!block.pack(buffer.data()))
    {
        std::cerr << "BlockFileBuffer::writeBlock -- block packing failed\n";
        return false;
    }

    const std::streamoff offset = getBlockOffset(rbn, block.getBlockSize());

    ioStream.clear();
    ioStream.seekp(offset, std::ios::beg);
    ioStream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    if (!ioStream.good())
    {
        std::cerr << "BlockFileBuffer::writeBlock -- write failed at RBN "
                  << rbn << "\n";
        return false;
    }

    ioStream.flush();
    return true;
}

bool BlockFileBuffer::readBlock(int rbn, BlockBuffer& block)
{
    std::istream* in = nullptr;

    if (readOpen)
    {
        in = &inStream;
    }
    else if (writeOpen)
    {
        in = &ioStream;
    }
    else
    {
        std::cerr << "BlockFileBuffer::readBlock -- file not open\n";
        return false;
    }

    const std::streamoff offset = getBlockOffset(rbn, block.getBlockSize());

    std::vector<char> buffer(block.getBlockSize());

    in->clear();
    in->seekg(offset, std::ios::beg);
    in->read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    if (in->gcount() != static_cast<std::streamsize>(buffer.size()))
    {
        std::cerr << "BlockFileBuffer::readBlock -- failed reading block at RBN "
                  << rbn << "\n";
        return false;
    }

    return block.unpack(buffer.data());
}

// -----------------------------------------------------------------------------
// Accessors
// -----------------------------------------------------------------------------

std::string BlockFileBuffer::getFilename() const
{
    return filename;
}

bool BlockFileBuffer::isReadOpen() const
{
    return readOpen;
}

bool BlockFileBuffer::isWriteOpen() const
{
    return writeOpen;
}