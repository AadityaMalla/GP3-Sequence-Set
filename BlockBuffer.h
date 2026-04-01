/**
 * @file BlockBuffer.h
 * @brief Represents one fixed-size block in the blocked sequence-set file
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 *
 * @details
 * A block stores:
 *   - a block header
 *   - zero or more variable-length ZIP code records
 *
 * Active block layout:
 *   [int32 recordCount]
 *   [int32 prevRBN]
 *   [int32 nextRBN]
 *   [record bytes...]
 *   [unused space padded with blanks]
 *
 * Avail block layout:
 *   [int32 recordCount == 0]
 *   [int32 nextAvailRBN]
 *   [remaining bytes padded with blanks]
 *
 * For active blocks:
 *   - recordCount > 0
 *   - records are stored sorted by ZIP code
 *
 * For avail blocks:
 *   - recordCount == 0
 *   - prevRBN is not used
 *   - nextRBN is interpreted as nextAvailRBN
 */

#ifndef BLOCKBUFFER_H
#define BLOCKBUFFER_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include "ZipCodeRecord.h"
#include "RecordBuffer.h"

/**
 * @class BlockBuffer
 * @brief In-memory representation of one physical block.
 *
 * @details
 * Responsibilities:
 *   - store active-block metadata
 *   - store avail-block metadata
 *   - keep records sorted by ZIP code
 *   - pack/unpack the full fixed-size block image
 *   - calculate used/free bytes
 *
 * NOT responsible for:
 *   - file I/O
 *   - global sequence-set management
 *   - index creation
 */
class BlockBuffer
{
public:
    /**
     * @brief Construct a block with the given fixed block size.
     * @param blockSize  Number of bytes in the physical block.
     */
    explicit BlockBuffer(int blockSize = 512);

    /**
     * @brief Reset this block to an empty active block.
     */
    void clearToEmptyActive();

    /**
     * @brief Reset this block to an avail block.
     * @param nextAvailRBN  Link to next avail block; -1 if none.
     */
    void clearToAvail(int nextAvailRBN = -1);

    /**
     * @brief Return true if this block is an avail block.
     * @return true if recordCount == 0.
     */
    bool isAvailBlock() const;

    /**
     * @brief Return true if this block is an active block.
     * @return true if recordCount > 0.
     */
    bool isActiveBlock() const;

    /**
     * @brief Return the fixed physical block size.
     * @return Block size in bytes.
     */
    int getBlockSize() const;

    /**
     * @brief Return the number of records stored in this block.
     * @return Number of active records.
     */
    int getRecordCount() const;

    /**
     * @brief Set previous active-block RBN.
     * @param rbn  Previous active block number.
     */
    void setPrevRBN(int rbn);

    /**
     * @brief Set next active-block RBN.
     * @param rbn  Next active block number.
     */
    void setNextRBN(int rbn);

    /**
     * @brief Get previous active-block RBN.
     * @return Previous RBN.
     */
    int getPrevRBN() const;

    /**
     * @brief Get next active-block RBN.
     * @return Next RBN.
     */
    int getNextRBN() const;

    /**
     * @brief Return number of bytes used inside this block.
     * @return Used byte count.
     */
    size_t getUsedBytes() const;

    /**
     * @brief Return number of bytes still free in this block.
     * @return Free byte count.
     */
    size_t getFreeBytes() const;

    /**
     * @brief Return true if a record can fit into the block.
     * @param record  Record to test.
     * @return true if there is enough free space.
     */
    bool canFit(const ZipCodeRecord& record) const;

    /**
     * @brief Insert a record in ZIP-sorted order.
     * @param record  Record to insert.
     * @return true if inserted; false if duplicate ZIP or insufficient space.
     */
    bool insertRecordSorted(const ZipCodeRecord& record);

    /**
     * @brief Remove a record by ZIP code.
     * @param zipCode  ZIP code to remove.
     * @return true if removed; false if not found.
     */
    bool removeRecordByZip(int zipCode);

    /**
     * @brief Search for a record by ZIP code.
     * @param zipCode  ZIP code to find.
     * @param record   Output record if found.
     * @return true if found; false otherwise.
     */
    bool findRecord(int zipCode, ZipCodeRecord& record) const;

    /**
     * @brief Return the highest ZIP key in the block.
     * @return Highest ZIP code, or -1 if no active records.
     */
    int getHighestKey() const;

    /**
     * @brief Return all records in this block.
     * @return Const reference to sorted record vector.
     */
    const std::vector<ZipCodeRecord>& getRecords() const;

    /**
     * @brief Pack the full block image into a raw byte buffer.
     * @param buffer  Caller-allocated buffer of exactly blockSize bytes.
     * @return true on success; false if packed data exceeds block size.
     */
    bool pack(char* buffer) const;

    /**
     * @brief Unpack a full block image from a raw byte buffer.
     * @param buffer  Input buffer of exactly blockSize bytes.
     * @return true on success; false on malformed input.
     */
    bool unpack(const char* buffer);

    /**
     * @brief Print block contents for debugging.
     */
    void display() const;

private:
    int blockSize;                        /**< Fixed physical block size */
    int recordCount;                      /**< 0 for avail; >0 for active */
    int prevRBN;                          /**< Previous active block */
    int nextRBN;                          /**< Next active block or next avail */
    std::vector<ZipCodeRecord> records;   /**< Sorted active records */
    RecordBuffer recordBuffer;            /**< Packs individual records */

    /**
     * @brief Return the number of metadata bytes stored at block front.
     * @return Header byte count.
     */
    size_t getBlockHeaderSize() const;

    /**
     * @brief Sort records by ZIP code ascending.
     */
    void sortRecords();
};

#endif // BLOCKBUFFER_H