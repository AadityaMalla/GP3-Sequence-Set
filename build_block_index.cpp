/**
 * @file build_block_index.cpp
 * @brief Builds the GP 3.0 simple block index from the sequence-set file
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include <iostream>
#include <string>

#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"
#include "BlockIndex.h"

// -- File constants ------------------------------------------------------------
static const std::string INPUT_SEQ_FILE = "data/us_postal_codes_seqset.txt";
static const std::string OUTPUT_IDX_FILE = "data/us_postal_codes_block_idx.txt";

// -- Forward declarations ------------------------------------------------------
bool buildBlockIndex(const std::string& seqFilename,
                     const std::string& idxFilename);

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=============================================================\n";
    std::cout << "   GP 3.0 -- Build Simple Block Index\n";
    std::cout << "=============================================================\n";

    if (!buildBlockIndex(INPUT_SEQ_FILE, OUTPUT_IDX_FILE))
    {
        std::cerr << "\nIndex build failed.\n";
        return 1;
    }

    std::cout << "\nIndex build completed successfully.\n";
    return 0;
}

// =============================================================================
// buildBlockIndex
// =============================================================================

bool buildBlockIndex(const std::string& seqFilename,
                     const std::string& idxFilename)
{
    BlockFileBuffer file(seqFilename);

    if (!file.openForRead())
    {
        std::cerr << "ERROR: cannot open sequence-set file: "
                  << seqFilename << "\n";
        return false;
    }

    BlockedFileHeader header;
    if (!file.readHeader(header))
    {
        std::cerr << "ERROR: cannot read blocked-file header\n";
        file.close();
        return false;
    }

    std::cout << "\nSequence-set header loaded.\n";
    std::cout << "Block count      : " << header.blockCount << "\n";
    std::cout << "Record count     : " << header.recordCount << "\n";
    std::cout << "Active list head : " << header.activeListHead << "\n";

    BlockIndex index(idxFilename);

    int rbn = header.activeListHead;
    int entryCount = 0;

    while (rbn != -1)
    {
        BlockBuffer block(header.blockSize);

        if (!file.readBlock(rbn, block))
        {
            std::cerr << "ERROR: failed reading block RBN " << rbn << "\n";
            file.close();
            return false;
        }

        if (block.getRecordCount() > 0)
        {
            int maxKey = block.getHighestKey();
            index.addEntry(maxKey, rbn);
            ++entryCount;

            std::cout << "Indexed block RBN " << rbn
                      << " with maxKey " << maxKey << "\n";
        }

        rbn = block.getNextRBN();
    }

    index.sortEntries();

    if (!index.writeIndex())
    {
        std::cerr << "ERROR: failed writing block index file\n";
        file.close();
        return false;
    }

    file.close();

    std::cout << "\nReadable index dump:\n";
    index.display();

    std::cout << "\nSummary:\n";
    std::cout << "  Sequence-set file : " << seqFilename << "\n";
    std::cout << "  Index file        : " << idxFilename << "\n";
    std::cout << "  Index entries     : " << entryCount << "\n";

    return true;
}