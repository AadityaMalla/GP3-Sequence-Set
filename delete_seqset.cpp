/**
 * @file delete_seqset.cpp
 * @brief Deletes ZIP code records from the GP 3.0 sequence-set file
 *        (supports simple delete, redistribution, and merge)
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>

#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"
#include "BlockIndex.h"
#include "ZipCodeRecord.h"

// -- File constants ------------------------------------------------------------
static const std::string SEQ_FILE = "data/us_postal_codes_seqset.txt";
static const std::string IDX_FILE = "data/us_postal_codes_block_idx.txt";
static const std::string INPUT_DELETE_FILE = "data/delete_records.txt";

// -- Forward declarations ------------------------------------------------------
bool loadDeleteZips(const std::string& filename, std::vector<int>& zips);
bool parseDeleteLine(const std::string& line, int& zipCode);
bool rebuildBlockIndex(const std::string& seqFilename, const std::string& idxFilename);
bool deleteOneZip(int zipCode);

size_t computeMaxRecordsPerBlock(int blockSize);
size_t computeMinRecordsRequired(int blockSize, double minCapacity);

bool redistributeFromLeft(BlockFileBuffer& file,
                          const BlockedFileHeader& header,
                          int leftRBN,
                          BlockBuffer& left,
                          int targetRBN,
                          BlockBuffer& target);

bool redistributeFromRight(BlockFileBuffer& file,
                           const BlockedFileHeader& header,
                           int rightRBN,
                           BlockBuffer& right,
                           int targetRBN,
                           BlockBuffer& target);

bool mergeWithLeft(BlockFileBuffer& file,
                   BlockedFileHeader& header,
                   int leftRBN,
                   BlockBuffer& left,
                   int targetRBN,
                   BlockBuffer& target);

bool mergeWithRight(BlockFileBuffer& file,
                    BlockedFileHeader& header,
                    int targetRBN,
                    BlockBuffer& target,
                    int rightRBN,
                    BlockBuffer& right);

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=============================================================\n";
    std::cout << "   GP 3.0 -- Delete Records from Sequence Set (Merge Version)\n";
    std::cout << "=============================================================\n";

    std::vector<int> zips;

    if (!loadDeleteZips(INPUT_DELETE_FILE, zips))
    {
        std::cerr << "ERROR: could not load delete-record input file\n";
        return 1;
    }

    std::cout << "Loaded " << zips.size() << " ZIP code(s) from "
              << INPUT_DELETE_FILE << "\n\n";

    int deleted = 0;
    int failed = 0;

    for (int zip : zips)
    {
        std::cout << "Attempting delete of ZIP: "
                  << std::setw(5) << std::setfill('0') << zip << "\n";
        std::cout << std::setfill(' ');

        if (deleteOneZip(zip))
        {
            std::cout << "Delete succeeded.\n\n";
            ++deleted;
        }
        else
        {
            std::cout << "Delete failed.\n\n";
            ++failed;
        }
    }

    std::cout << "Summary:\n";
    std::cout << "  Deleted: " << deleted << "\n";
    std::cout << "  Failed : " << failed << "\n";

    return 0;
}

// =============================================================================
// loadDeleteZips
// =============================================================================

bool loadDeleteZips(const std::string& filename, std::vector<int>& zips)
{
    std::ifstream in(filename);
    if (!in.is_open())
    {
        std::cerr << "ERROR: cannot open delete-record file: " << filename << "\n";
        return false;
    }

    zips.clear();

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;

        int zipCode = 0;
        if (!parseDeleteLine(line, zipCode))
        {
            std::cerr << "WARNING: malformed delete line skipped: " << line << "\n";
            continue;
        }

        zips.push_back(zipCode);
    }

    return true;
}

// =============================================================================
// parseDeleteLine
// =============================================================================

bool parseDeleteLine(const std::string& line, int& zipCode)
{
    std::string digits = line;

    while (!digits.empty() &&
           (digits.back() == '\r' || digits.back() == '\n' || digits.back() == ' '))
    {
        digits.pop_back();
    }

    if (digits.empty())
        return false;

    for (char ch : digits)
    {
        if (!std::isdigit(static_cast<unsigned char>(ch)))
            return false;
    }

    try
    {
        zipCode = std::stoi(digits);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

// =============================================================================
// rebuildBlockIndex
// =============================================================================

bool rebuildBlockIndex(const std::string& seqFilename, const std::string& idxFilename)
{
    BlockFileBuffer file(seqFilename);

    if (!file.openForRead())
    {
        std::cerr << "ERROR: cannot open sequence-set file for index rebuild\n";
        return false;
    }

    BlockedFileHeader header;
    if (!file.readHeader(header))
    {
        std::cerr << "ERROR: cannot read blocked header during index rebuild\n";
        file.close();
        return false;
    }

    BlockIndex index(idxFilename);

    int rbn = header.activeListHead;
    while (rbn != -1)
    {
        BlockBuffer block(header.blockSize);

        if (!file.readBlock(rbn, block))
        {
            std::cerr << "ERROR: failed reading block " << rbn
                      << " during index rebuild\n";
            file.close();
            return false;
        }

        if (block.getRecordCount() > 0)
            index.addEntry(block.getHighestKey(), rbn);

        rbn = block.getNextRBN();
    }

    index.sortEntries();
    file.close();

    return index.writeIndex();
}

// =============================================================================
// Capacity helpers
// =============================================================================

size_t computeMaxRecordsPerBlock(int blockSize)
{
    BlockBuffer probe(blockSize);
    size_t count = 0;

    while (true)
    {
        ZipCodeRecord rec(
            static_cast<int>(10000 + count),
            "X",
            "XX",
            "Y",
            0.0,
            0.0
        );

        if (!probe.canFit(rec))
            break;

        if (!probe.insertRecordSorted(rec))
            break;

        ++count;
    }

    return count;
}

size_t computeMinRecordsRequired(int blockSize, double minCapacity)
{
    const size_t maxRecords = computeMaxRecordsPerBlock(blockSize);
    size_t minRequired = static_cast<size_t>(std::ceil(maxRecords * minCapacity));

    if (minRequired < 1)
        minRequired = 1;

    return minRequired;
}

// =============================================================================
// Redistribution helpers
// =============================================================================

bool redistributeFromLeft(BlockFileBuffer& file,
                          const BlockedFileHeader& header,
                          int leftRBN,
                          BlockBuffer& left,
                          int targetRBN,
                          BlockBuffer& target)
{
    const std::vector<ZipCodeRecord>& leftRecords = left.getRecords();
    if (leftRecords.empty())
        return false;

    ZipCodeRecord moved = leftRecords.back();

    if (!left.removeRecordByZip(moved.zipCode))
        return false;

    if (!target.insertRecordSorted(moved))
        return false;

    if (!file.writeBlock(leftRBN, left))
        return false;

    if (!file.writeBlock(targetRBN, target))
        return false;

    std::cout << "Redistribution used LEFT neighbor.\n";
    return true;
}

bool redistributeFromRight(BlockFileBuffer& file,
                           const BlockedFileHeader& header,
                           int rightRBN,
                           BlockBuffer& right,
                           int targetRBN,
                           BlockBuffer& target)
{
    const std::vector<ZipCodeRecord>& rightRecords = right.getRecords();
    if (rightRecords.empty())
        return false;

    ZipCodeRecord moved = rightRecords.front();

    if (!right.removeRecordByZip(moved.zipCode))
        return false;

    if (!target.insertRecordSorted(moved))
        return false;

    if (!file.writeBlock(rightRBN, right))
        return false;

    if (!file.writeBlock(targetRBN, target))
        return false;

    std::cout << "Redistribution used RIGHT neighbor.\n";
    return true;
}

// =============================================================================
// Merge helpers
// =============================================================================

bool mergeWithLeft(BlockFileBuffer& file,
                   BlockedFileHeader& header,
                   int leftRBN,
                   BlockBuffer& left,
                   int targetRBN,
                   BlockBuffer& target)
{
    for (const auto& rec : target.getRecords())
    {
        if (!left.insertRecordSorted(rec))
        {
            std::cerr << "ERROR: left merge failed due to capacity\n";
            return false;
        }
    }

    const int targetNext = target.getNextRBN();
    left.setNextRBN(targetNext);

    if (!file.writeBlock(leftRBN, left))
        return false;

    if (targetNext != -1)
    {
        BlockBuffer nextBlock(header.blockSize);
        if (!file.readBlock(targetNext, nextBlock))
            return false;

        nextBlock.setPrevRBN(leftRBN);

        if (!file.writeBlock(targetNext, nextBlock))
            return false;
    }

    // Convert target block to avail block
    target.clearToAvail(header.availListHead);
    if (!file.writeBlock(targetRBN, target))
        return false;

    header.availListHead = targetRBN;
    header.blockCount -= 1;

    std::cout << "Merge used LEFT neighbor; block " << targetRBN
              << " moved to avail list.\n";
    return true;
}

bool mergeWithRight(BlockFileBuffer& file,
                    BlockedFileHeader& header,
                    int targetRBN,
                    BlockBuffer& target,
                    int rightRBN,
                    BlockBuffer& right)
{
    for (const auto& rec : right.getRecords())
    {
        if (!target.insertRecordSorted(rec))
        {
            std::cerr << "ERROR: right merge failed due to capacity\n";
            return false;
        }
    }

    const int rightNext = right.getNextRBN();
    target.setNextRBN(rightNext);

    if (!file.writeBlock(targetRBN, target))
        return false;

    if (rightNext != -1)
    {
        BlockBuffer nextBlock(header.blockSize);
        if (!file.readBlock(rightNext, nextBlock))
            return false;

        nextBlock.setPrevRBN(targetRBN);

        if (!file.writeBlock(rightNext, nextBlock))
            return false;
    }

    // Convert right block to avail block
    right.clearToAvail(header.availListHead);
    if (!file.writeBlock(rightRBN, right))
        return false;

    header.availListHead = rightRBN;
    header.blockCount -= 1;

    std::cout << "Merge used RIGHT neighbor; block " << rightRBN
              << " moved to avail list.\n";
    return true;
}

// =============================================================================
// deleteOneZip
// =============================================================================

bool deleteOneZip(int zipCode)
{
    BlockIndex index(IDX_FILE);
    if (!index.loadIndex())
    {
        std::cerr << "ERROR: cannot load block index\n";
        return false;
    }

    int candidateRBN = -1;
    if (!index.findCandidateBlock(zipCode, candidateRBN))
    {
        std::cerr << "ERROR: no candidate block found for ZIP "
                  << std::setw(5) << std::setfill('0') << zipCode << "\n";
        std::cout << std::setfill(' ');
        return false;
    }

    BlockFileBuffer file(SEQ_FILE);
    if (!file.openForUpdate())
    {
        std::cerr << "ERROR: cannot open sequence-set file for update\n";
        return false;
    }

    BlockedFileHeader header;
    if (!file.readHeader(header))
    {
        std::cerr << "ERROR: cannot read sequence-set header\n";
        file.close();
        return false;
    }

    BlockBuffer target(header.blockSize);
    if (!file.readBlock(candidateRBN, target))
    {
        std::cerr << "ERROR: cannot read candidate block RBN "
                  << candidateRBN << "\n";
        file.close();
        return false;
    }

    if (!target.removeRecordByZip(zipCode))
    {
        std::cerr << "ERROR: ZIP not found inside candidate block: "
                  << std::setw(5) << std::setfill('0') << zipCode << "\n";
        std::cout << std::setfill(' ');
        file.close();
        return false;
    }

    header.recordCount -= 1;

    // If target became empty, we still handle it through merge logic if possible.
    const size_t minRequired =
        computeMinRecordsRequired(header.blockSize, header.minBlockCapacity);

    const int leftRBN = target.getPrevRBN();
    const int rightRBN = target.getNextRBN();

    // Case 1: last/only block or still above minimum
    if (target.getRecordCount() == 0)
    {
        std::cout << "Target block became empty after delete.\n";
    }

    if (target.getRecordCount() >= static_cast<int>(minRequired) ||
        (leftRBN == -1 && rightRBN == -1))
    {
        if (!file.writeBlock(candidateRBN, target))
        {
            std::cerr << "ERROR: failed writing updated block after delete\n";
            file.close();
            return false;
        }

        header.staleFlag = false;

        if (!file.writeHeader(header))
        {
            std::cerr << "ERROR: failed updating blocked header after delete\n";
            file.close();
            return false;
        }

        file.close();

        if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
        {
            std::cerr << "ERROR: failed rebuilding block index after delete\n";
            return false;
        }

        std::cout << "Delete completed with NO rebalance.\n";
        return true;
    }

    // Load neighbors if present
    BlockBuffer left(header.blockSize);
    BlockBuffer right(header.blockSize);
    bool hasLeft = false;
    bool hasRight = false;

    if (leftRBN != -1)
    {
        if (!file.readBlock(leftRBN, left))
        {
            std::cerr << "ERROR: failed reading left neighbor block\n";
            file.close();
            return false;
        }
        hasLeft = true;
    }

    if (rightRBN != -1)
    {
        if (!file.readBlock(rightRBN, right))
        {
            std::cerr << "ERROR: failed reading right neighbor block\n";
            file.close();
            return false;
        }
        hasRight = true;
    }

    // Case 2: redistribution from left
    if (hasLeft && left.getRecordCount() > static_cast<int>(minRequired))
    {
        if (!redistributeFromLeft(file, header, leftRBN, left, candidateRBN, target))
        {
            std::cerr << "ERROR: redistribution from left failed\n";
            file.close();
            return false;
        }

        header.staleFlag = false;
        if (!file.writeHeader(header))
        {
            std::cerr << "ERROR: failed updating header after redistribution\n";
            file.close();
            return false;
        }

        file.close();

        if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
        {
            std::cerr << "ERROR: failed rebuilding index after redistribution\n";
            return false;
        }

        return true;
    }

    // Case 3: redistribution from right
    if (hasRight && right.getRecordCount() > static_cast<int>(minRequired))
    {
        if (!redistributeFromRight(file, header, rightRBN, right, candidateRBN, target))
        {
            std::cerr << "ERROR: redistribution from right failed\n";
            file.close();
            return false;
        }

        header.staleFlag = false;
        if (!file.writeHeader(header))
        {
            std::cerr << "ERROR: failed updating header after redistribution\n";
            file.close();
            return false;
        }

        file.close();

        if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
        {
            std::cerr << "ERROR: failed rebuilding index after redistribution\n";
            return false;
        }

        return true;
    }

    // Case 4: merge with left if possible
    if (hasLeft)
    {
        size_t combined = static_cast<size_t>(left.getRecordCount() + target.getRecordCount());
        if (combined <= computeMaxRecordsPerBlock(header.blockSize))
        {
            if (!mergeWithLeft(file, header, leftRBN, left, candidateRBN, target))
            {
                std::cerr << "ERROR: merge with left failed\n";
                file.close();
                return false;
            }

            header.staleFlag = false;
            if (!file.writeHeader(header))
            {
                std::cerr << "ERROR: failed updating header after merge\n";
                file.close();
                return false;
            }

            file.close();

            if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
            {
                std::cerr << "ERROR: failed rebuilding index after merge\n";
                return false;
            }

            return true;
        }
    }

    // Case 5: merge with right if possible
    if (hasRight)
    {
        size_t combined = static_cast<size_t>(target.getRecordCount() + right.getRecordCount());
        if (combined <= computeMaxRecordsPerBlock(header.blockSize))
        {
            if (!mergeWithRight(file, header, candidateRBN, target, rightRBN, right))
            {
                std::cerr << "ERROR: merge with right failed\n";
                file.close();
                return false;
            }

            header.staleFlag = false;
            if (!file.writeHeader(header))
            {
                std::cerr << "ERROR: failed updating header after merge\n";
                file.close();
                return false;
            }

            file.close();

            if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
            {
                std::cerr << "ERROR: failed rebuilding index after merge\n";
                return false;
            }

            return true;
        }
    }

    // Fallback: write target anyway if no rebalance action was possible
    if (!file.writeBlock(candidateRBN, target))
    {
        std::cerr << "ERROR: failed writing fallback target block\n";
        file.close();
        return false;
    }

    header.staleFlag = false;
    if (!file.writeHeader(header))
    {
        std::cerr << "ERROR: failed updating header in fallback case\n";
        file.close();
        return false;
    }

    file.close();

    if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
    {
        std::cerr << "ERROR: failed rebuilding index in fallback case\n";
        return false;
    }

    std::cout << "Delete completed in fallback mode (no redistribution/merge possible).\n";
    return true;
}