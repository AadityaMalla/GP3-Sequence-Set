/**
 * @file build_seqset.cpp
 * @brief Builds the GP 3.0 blocked sequence-set file from a GP 2.0 LI file
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

#include "ZipCodeRecord.h"
#include "RecordBuffer.h"
#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"

// -- File constants ------------------------------------------------------------
static const std::string INPUT_LI_FILE   = "data/us_postal_codes_li.txt";
static const std::string OUTPUT_SEQ_FILE = "data/us_postal_codes_seqset.txt";
static const std::string OUTPUT_IDX_FILE = "data/us_postal_codes_block_idx.txt";

// -- Forward declarations ------------------------------------------------------
BlockedFileHeader buildBlockedHeader();
bool buildSequenceSet(const std::string& inputLI,
                      const std::string& outputSeq,
                      int blockSize);
bool readNextLIRecord(std::ifstream& in,
                      RecordBuffer& recordBuffer,
                      ZipCodeRecord& record);
bool skipLIHeader(std::ifstream& in);
uint16_t readBigEndianLength(std::istream& in);

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=============================================================\n";
    std::cout << "   GP 3.0 -- Build Blocked Sequence-Set File\n";
    std::cout << "=============================================================\n";

    const int blockSize = 512;

    if (!buildSequenceSet(INPUT_LI_FILE, OUTPUT_SEQ_FILE, blockSize))
    {
        std::cerr << "\nBuild failed.\n";
        return 1;
    }

    std::cout << "\nBuild completed successfully.\n";
    return 0;
}

// =============================================================================
// buildBlockedHeader
// =============================================================================

BlockedFileHeader buildBlockedHeader()
{
    BlockedFileHeader header;

    header.fileStructureType       = "ZIPCODE_BSS_V1";
    header.version                 = 1;
    header.recordSizeIntegerBytes  = 2;
    header.sizeFormatType          = "binary";
    header.sizeOfSizes             = 2;
    header.sizeInclusionFlag       = false;

    header.blockSize               = 512;
    header.minBlockCapacity        = 0.50;
    header.recordCount             = 0;
    header.blockCount              = 0;
    header.activeListHead          = -1;
    header.availListHead           = -1;
    header.staleFlag               = false;

    header.primaryKeyIndexFileName = OUTPUT_IDX_FILE;
    header.indexFileStructureType  = "ZIPCODE_BLOCK_INDEX_V1";

    header.addField("zipCode",   "int",    true);
    header.addField("placeName", "string", false);
    header.addField("state",     "string", false);
    header.addField("county",    "string", false);
    header.addField("latitude",  "double", false);
    header.addField("longitude", "double", false);

    return header;
}

// =============================================================================
// readBigEndianLength
// =============================================================================

uint16_t readBigEndianLength(std::istream& in)
{
    uint8_t hi = 0;
    uint8_t lo = 0;

    in.read(reinterpret_cast<char*>(&hi), 1);
    in.read(reinterpret_cast<char*>(&lo), 1);

    if (!in.good())
        return 0;

    return static_cast<uint16_t>((hi << 8) | lo);
}

// =============================================================================
// skipLIHeader
// =============================================================================

bool skipLIHeader(std::ifstream& in)
{
    in.clear();
    in.seekg(0, std::ios::beg);

    uint16_t headerLen = readBigEndianLength(in);
    if (!in.good() || headerLen == 0)
    {
        std::cerr << "ERROR: could not read LI header length\n";
        return false;
    }

    in.seekg(headerLen, std::ios::cur);
    if (!in.good())
    {
        std::cerr << "ERROR: could not skip LI header payload\n";
        return false;
    }

    return true;
}

// =============================================================================
// readNextLIRecord
// =============================================================================

bool readNextLIRecord(std::ifstream& in,
                      RecordBuffer& recordBuffer,
                      ZipCodeRecord& record)
{
    if (!in.good())
        return false;

    // Try reading 2-byte record length
    char lenBytes[2];
    in.read(lenBytes, 2);

    if (in.eof())
        return false;

    if (!in.good())
        return false;

    uint16_t payloadLen =
        (static_cast<uint8_t>(lenBytes[0]) << 8) |
         static_cast<uint8_t>(lenBytes[1]);

    if (payloadLen == 0)
    {
        std::cerr << "ERROR: encountered zero-length LI record\n";
        return false;
    }

    std::vector<char> buffer(2 + payloadLen);
    buffer[0] = lenBytes[0];
    buffer[1] = lenBytes[1];

    in.read(buffer.data() + 2, payloadLen);
    if (!in.good())
    {
        std::cerr << "ERROR: truncated LI record payload\n";
        return false;
    }

    return recordBuffer.unpack(buffer.data(), record) > 0;
}

// =============================================================================
// buildSequenceSet
// =============================================================================

bool buildSequenceSet(const std::string& inputLI,
                      const std::string& outputSeq,
                      int blockSize)
{
    // -- Step 1: Open LI input file directly --------------------------------
    std::ifstream li(inputLI, std::ios::binary);
    if (!li.is_open())
    {
        std::cerr << "ERROR: cannot open LI input file: " << inputLI << "\n";
        return false;
    }

    if (!skipLIHeader(li))
    {
        li.close();
        return false;
    }

    std::cout << "\nInput LI file opened and header skipped successfully.\n";

    // -- Step 2: Open blocked output file -----------------------------------
    BlockFileBuffer out(outputSeq);
    if (!out.openForWrite())
    {
        std::cerr << "ERROR: cannot open output sequence-set file: "
                  << outputSeq << "\n";
        li.close();
        return false;
    }

    // -- Step 3: Prepare blocked header -------------------------------------
    BlockedFileHeader blockedHeader = buildBlockedHeader();
    blockedHeader.blockSize = blockSize;

    if (!out.writeHeader(blockedHeader))
    {
        std::cerr << "ERROR: cannot write initial blocked header\n";
        li.close();
        out.close();
        return false;
    }

    // -- Step 4: Read LI records and build blocks ---------------------------
    RecordBuffer recordBuffer;
    ZipCodeRecord rec;

    BlockBuffer currentBlock(blockSize);
    currentBlock.clearToEmptyActive();

    int currentRBN  = 0;
    int totalBlocks = 0;
    int totalRecs   = 0;
    bool hasAnyBlocks = false;

    while (readNextLIRecord(li, recordBuffer, rec))
    {
        if (!currentBlock.insertRecordSorted(rec))
        {
            // Current block full — write it, then start new block.
            if (currentBlock.getRecordCount() == 0)
            {
                std::cerr << "ERROR: single record too large for empty block\n";
                li.close();
                out.close();
                return false;
            }

            if (!out.writeBlock(currentRBN, currentBlock))
            {
                std::cerr << "ERROR: failed writing block RBN " << currentRBN << "\n";
                li.close();
                out.close();
                return false;
            }

            hasAnyBlocks = true;
            ++totalBlocks;

            std::cout << "Wrote block " << currentRBN
                      << " with " << currentBlock.getRecordCount()
                      << " records; highest key = "
                      << currentBlock.getHighestKey() << "\n";

            ++currentRBN;

            BlockBuffer nextBlock(blockSize);
            nextBlock.clearToEmptyActive();

            if (!nextBlock.insertRecordSorted(rec))
            {
                std::cerr << "ERROR: record does not fit even in fresh block\n";
                li.close();
                out.close();
                return false;
            }

            currentBlock = nextBlock;
        }

        ++totalRecs;
    }

    // -- Step 5: Write final block if needed --------------------------------
    if (currentBlock.getRecordCount() > 0)
    {
        if (!out.writeBlock(currentRBN, currentBlock))
        {
            std::cerr << "ERROR: failed writing final block RBN " << currentRBN << "\n";
            li.close();
            out.close();
            return false;
        }

        hasAnyBlocks = true;
        ++totalBlocks;

        std::cout << "Wrote block " << currentRBN
                  << " with " << currentBlock.getRecordCount()
                  << " records; highest key = "
                  << currentBlock.getHighestKey() << "\n";
    }

    // -- Step 6: Rewrite blocks with correct active-list links --------------
    for (int rbn = 0; rbn < totalBlocks; ++rbn)
    {
        BlockBuffer blk(blockSize);

        if (!out.readBlock(rbn, blk))
        {
            std::cerr << "ERROR: failed reading block back for link update: "
                      << rbn << "\n";
            li.close();
            out.close();
            return false;
        }

        blk.setPrevRBN((rbn == 0) ? -1 : rbn - 1);
        blk.setNextRBN((rbn == totalBlocks - 1) ? -1 : rbn + 1);

        if (!out.writeBlock(rbn, blk))
        {
            std::cerr << "ERROR: failed rewriting block with links: "
                      << rbn << "\n";
            li.close();
            out.close();
            return false;
        }
    }

    // -- Step 7: Finalize blocked header ------------------------------------
    blockedHeader.recordCount    = totalRecs;
    blockedHeader.blockCount     = totalBlocks;
    blockedHeader.activeListHead = hasAnyBlocks ? 0 : -1;
    blockedHeader.availListHead  = -1;
    blockedHeader.staleFlag      = false;

    if (!out.writeHeader(blockedHeader))
    {
        std::cerr << "ERROR: failed rewriting final blocked header\n";
        li.close();
        out.close();
        return false;
    }

    li.close();
    out.close();

    std::cout << "\nFinal blocked header:\n";
    blockedHeader.display();

    std::cout << "\nSummary:\n";
    std::cout << "  Input LI file     : " << inputLI << "\n";
    std::cout << "  Output seqset file: " << outputSeq << "\n";
    std::cout << "  Block size        : " << blockSize << "\n";
    std::cout << "  Total records     : " << totalRecs << "\n";
    std::cout << "  Total blocks      : " << totalBlocks << "\n";

    return true;
}