/**
 * @file BlockIndex.h
 * @brief Simple primary-key block index for GP 3.0
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 *
 * @details
 * This index stores one entry per active block:
 *
 *   [highest key in block] -> [RBN]
 *
 * The entries are sorted by highest key so binary search can be used.
 */

#ifndef BLOCKINDEX_H
#define BLOCKINDEX_H

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * @struct BlockIndexEntry
 * @brief One simple block-index entry.
 */
struct BlockIndexEntry
{
    int32_t maxKey;   /**< Highest ZIP key stored in the block */
    int32_t rbn;      /**< Relative block number */

    BlockIndexEntry() : maxKey(0), rbn(-1) {}
    BlockIndexEntry(int key, int blockNum) : maxKey(key), rbn(blockNum) {}

    bool operator<(const BlockIndexEntry& other) const
    {
        return maxKey < other.maxKey;
    }
};

/**
 * @class BlockIndex
 * @brief In-memory simple index for the blocked sequence-set file.
 *
 * @details
 * Responsibilities:
 *   - store block index entries
 *   - sort entries
 *   - write/read binary index file
 *   - find candidate block for a ZIP code via binary search
 */
class BlockIndex
{
public:
    /**
     * @brief Construct an index associated with a filename.
     * @param indexFilename  Path to binary block-index file.
     */
    explicit BlockIndex(const std::string& indexFilename);

    /**
     * @brief Clear all in-memory entries.
     */
    void clear();

    /**
     * @brief Add one entry to the in-memory index.
     * @param maxKey  Highest ZIP in the block.
     * @param rbn     Relative block number.
     */
    void addEntry(int maxKey, int rbn);

    /**
     * @brief Sort entries by maxKey ascending.
     */
    void sortEntries();

    /**
     * @brief Write index entries to the binary index file.
     * @return true on success; false otherwise.
     */
    bool writeIndex() const;

    /**
     * @brief Load index entries from the binary index file.
     * @return true on success; false otherwise.
     */
    bool loadIndex();

    /**
     * @brief Find the candidate block for a target ZIP code.
     * @param zipCode  ZIP code being searched.
     * @param rbn      Output candidate RBN.
     * @return true if a candidate block exists; false otherwise.
     *
     * @details
     * Uses lower_bound on maxKey values to find the first block whose
     * maxKey >= zipCode.
     */
    bool findCandidateBlock(int zipCode, int& rbn) const;

    /**
     * @brief Return the number of entries in memory.
     * @return entries.size()
     */
    size_t size() const;

    /**
     * @brief Return the associated index filename.
     * @return Filename string.
     */
    std::string getFilename() const;

    /**
     * @brief Return const reference to all entries.
     * @return Const vector reference.
     */
    const std::vector<BlockIndexEntry>& getEntries() const;

    /**
     * @brief Print the index in readable form.
     */
    void display() const;

private:
    std::string indexFilename;               /**< Binary index filename */
    std::vector<BlockIndexEntry> entries;    /**< Sorted block-index entries */
};

#endif // BLOCKINDEX_H