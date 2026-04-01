/**
 * @file dump_seqset.cpp
 * @brief Dumps a GP 3.0 blocked sequence-set file
 */

#include <iostream>
#include <string>

#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"

static const std::string INPUT_SEQ_FILE = "data/us_postal_codes_seqset.txt";

// ─────────────────────────────────────────────────────────────────────────────

void dumpPhysical(BlockFileBuffer& file,
                  const BlockedFileHeader& header)
{
    std::cout << "\n=============================================\n";
    std::cout << "PHYSICAL ORDER DUMP\n";
    std::cout << "=============================================\n";

    for (int rbn = 0; rbn < header.blockCount; ++rbn)
    {
        BlockBuffer block(header.blockSize);

        if (!file.readBlock(rbn, block))
        {
            std::cout << "Failed to read block " << rbn << "\n";
            continue;
        }

        std::cout << "\n--- BLOCK RBN " << rbn << " ---\n";
        block.display();
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void dumpLogical(BlockFileBuffer& file,
                 const BlockedFileHeader& header)
{
    std::cout << "\n=============================================\n";
    std::cout << "LOGICAL ORDER DUMP\n";
    std::cout << "=============================================\n";

    int rbn = header.activeListHead;

    while (rbn != -1)
    {
        BlockBuffer block(header.blockSize);

        if (!file.readBlock(rbn, block))
        {
            std::cout << "Failed to read block " << rbn << "\n";
            break;
        }

        std::cout << "\n--- BLOCK RBN " << rbn << " ---\n";
        block.display();

        rbn = block.getNextRBN();
    }
}

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "=============================================\n";
    std::cout << "   GP 3.0 -- Dump Sequence Set\n";
    std::cout << "=============================================\n";

    BlockFileBuffer file(INPUT_SEQ_FILE);

    if (!file.openForRead())
    {
        std::cerr << "ERROR: cannot open sequence-set file\n";
        return 1;
    }

    BlockedFileHeader header;

    if (!file.readHeader(header))
    {
        std::cerr << "ERROR: cannot read header\n";
        return 1;
    }

    std::cout << "\nHEADER:\n";
    header.display();

    dumpPhysical(file, header);
    dumpLogical(file, header);

    file.close();

    return 0;
}