/**
 * @file BlockFileBuffer.h
 * @brief File I/O layer for blocked sequence-set files
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 *
 * @details
 * This class manages the physical blocked file on disk.
 *
 * File layout:
 *   [2-byte header length][header payload]
 *   [block 0 bytes]
 *   [block 1 bytes]
 *   [block 2 bytes]
 *   ...
 *
 * Responsibilities:
 *   - open/close the file
 *   - write/read the blocked file header
 *   - write/read one fixed-size block by RBN
 *   - compute byte offsets for blocks
 *
 * NOT responsible for:
 *   - sequence-set logic
 *   - block splitting/merging
 *   - indexing
 */

#ifndef BLOCKFILEBUFFER_H
#define BLOCKFILEBUFFER_H

#include <fstream>
#include <string>
#include <streambuf>
#include "BlockedFileHeader.h"
#include "BlockBuffer.h"

/**
 * @class BlockFileBuffer
 * @brief Provides physical file access for blocked sequence-set files.
 */
class BlockFileBuffer
{
public:
    /**
     * @brief Construct a BlockFileBuffer for a filename.
     * @param filename  Path to blocked data file.
     */
    explicit BlockFileBuffer(const std::string& filename);

    /**
     * @brief Destructor.
     */
    ~BlockFileBuffer();

    /**
     * @brief Open file for reading.
     * @return true on success; false otherwise.
     */
    bool openForRead();

    /**
     * @brief Open file for writing, truncating old contents.
     * @return true on success; false otherwise.
     */
    bool openForWrite();

    /**
     * @brief Open file for read/write update without truncating.
     * @return true on success; false otherwise.
     */
    bool openForUpdate();

    /**
     * @brief Close any open file streams.
     */
    void close();

    /**
     * @brief Write the blocked-file header at the start of the file.
     * @param header  Header to write.
     * @return true on success; false otherwise.
     *
     * @note This stores the end-of-header byte position internally so that
     *       future block offsets can be calculated correctly.
     */
    bool writeHeader(BlockedFileHeader& header);

    /**
     * @brief Read the blocked-file header from the start of the file.
     * @param header  Output header.
     * @return true on success; false otherwise.
     *
     * @note This stores the end-of-header byte position internally so that
     *       future block offsets can be calculated correctly.
     */
    bool readHeader(BlockedFileHeader& header);

    /**
     * @brief Write one physical block by RBN.
     * @param rbn    Relative block number (0-based).
     * @param block  Block to write.
     * @return true on success; false otherwise.
     */
    bool writeBlock(int rbn, const BlockBuffer& block);

    /**
     * @brief Read one physical block by RBN.
     * @param rbn    Relative block number (0-based).
     * @param block  Output block object.
     * @return true on success; false otherwise.
     */
    bool readBlock(int rbn, BlockBuffer& block);

    /**
     * @brief Return the byte offset of a given block.
     * @param rbn        Relative block number.
     * @param blockSize  Size of each physical block in bytes.
     * @return Absolute byte offset in the file.
     */
    std::streamoff getBlockOffset(int rbn, int blockSize) const;

    /**
     * @brief Return the filename associated with this object.
     * @return Filename string.
     */
    std::string getFilename() const;

    /**
     * @brief Return true if file is currently open for reading.
     * @return true if readable stream is open.
     */
    bool isReadOpen() const;

    /**
     * @brief Return true if file is currently open for writing/updating.
     * @return true if writable stream is open.
     */
    bool isWriteOpen() const;

private:
    std::string filename;        /**< Blocked data filename */
    std::ifstream inStream;      /**< Read stream */
    std::fstream ioStream;       /**< Read/write stream */
    bool readOpen;               /**< True if inStream open */
    bool writeOpen;              /**< True if ioStream open */
    std::streamoff headerEndPos; /**< Absolute offset of first block */

    /**
     * @brief Read a 2-byte big-endian header length from the stream.
     * @param in      Input stream.
     * @param length  Output payload length.
     * @return true on success; false otherwise.
     */
    bool readLength(std::istream& in, uint16_t& length);

    /**
     * @brief Write a 2-byte big-endian header length to the stream.
     * @param out     Output stream.
     * @param length  Payload length.
     * @return true on success; false otherwise.
     */
    bool writeLength(std::ostream& out, uint16_t length);
};

#endif // BLOCKFILEBUFFER_H