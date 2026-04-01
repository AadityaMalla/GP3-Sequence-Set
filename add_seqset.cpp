/**
 * @file add_seqset.cpp
 * @brief Adds ZIP code records to the GP 3.0 sequence-set file
 *        (supports insert without split and insert with split)
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"
#include "BlockIndex.h"
#include "ZipCodeRecord.h"

// ── File constants ────────────────────────────────────────────────────────────
static const std::string SEQ_FILE = "data/us_postal_codes_seqset.txt";
static const std::string IDX_FILE = "data/us_postal_codes_block_idx.txt";
static const std::string INPUT_ADD_FILE = "data/add_records.txt";

// ── Forward declarations ──────────────────────────────────────────────────────
bool loadAddRecords(const std::string& filename, std::vector<ZipCodeRecord>& records);
bool parseAddLine(const std::string& line, ZipCodeRecord& record);
bool rebuildBlockIndex(const std::string& seqFilename, const std::string& idxFilename);
bool addOneRecord(const ZipCodeRecord& record);
void printRecord(const ZipCodeRecord& record);

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=============================================================\n";
    std::cout << "   GP 3.0 -- Add Records to Sequence Set (Split Version)\n";
    std::cout << "=============================================================\n";

    std::vector<ZipCodeRecord> records;

    if (!loadAddRecords(INPUT_ADD_FILE, records))
    {
        std::cerr << "ERROR: could not load add-record input file\n";
        return 1;
    }

    std::cout << "Loaded " << records.size() << " record(s) from "
              << INPUT_ADD_FILE << "\n\n";

    int added = 0;
    int failed = 0;

    for (const auto& rec : records)
    {
        std::cout << "Attempting insert:\n";
        printRecord(rec);

        if (addOneRecord(rec))
        {
            std::cout << "Insert succeeded.\n\n";
            ++added;
        }
        else
        {
            std::cout << "Insert failed.\n\n";
            ++failed;
        }
    }

    std::cout << "Summary:\n";
    std::cout << "  Added : " << added << "\n";
    std::cout << "  Failed: " << failed << "\n";

    return 0;
}

// =============================================================================
// loadAddRecords
// =============================================================================

bool loadAddRecords(const std::string& filename, std::vector<ZipCodeRecord>& records)
{
    std::ifstream in(filename);
    if (!in.is_open())
    {
        std::cerr << "ERROR: cannot open add-record file: " << filename << "\n";
        return false;
    }

    records.clear();

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;

        ZipCodeRecord rec;
        if (!parseAddLine(line, rec))
        {
            std::cerr << "WARNING: malformed input line skipped: " << line << "\n";
            continue;
        }

        records.push_back(rec);
    }

    return true;
}

// =============================================================================
// parseAddLine
// =============================================================================

bool parseAddLine(const std::string& line, ZipCodeRecord& record)
{
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> parts;

    while (std::getline(iss, token, ','))
        parts.push_back(token);

    if (parts.size() < 6)
        return false;

    try
    {
        record.zipCode   = std::stoi(parts[0]);
        record.placeName = parts[1];
        record.state     = parts[2];
        record.county    = parts[3];
        record.latitude  = std::stod(parts[4]);
        record.longitude = std::stod(parts[5]);
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
// addOneRecord
// =============================================================================

bool addOneRecord(const ZipCodeRecord& record)
{
    BlockIndex index(IDX_FILE);
    if (!index.loadIndex())
    {
        std::cerr << "ERROR: cannot load block index\n";
        return false;
    }

    int candidateRBN = -1;

    // Normal case: first block whose maxKey >= record.zipCode
    if (!index.findCandidateBlock(record.zipCode, candidateRBN))
    {
        // If new ZIP is greater than all current max keys, insert into last block.
        if (index.size() == 0)
        {
            std::cerr << "ERROR: index is empty\n";
            return false;
        }

        candidateRBN = index.getEntries().back().rbn;
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

    // Reject duplicate key
    ZipCodeRecord existing;
    if (target.findRecord(record.zipCode, existing))
    {
        std::cerr << "ERROR: duplicate ZIP already exists: "
                  << std::setw(5) << std::setfill('0') << record.zipCode << "\n";
        std::cout << std::setfill(' ');
        file.close();
        return false;
    }

    // ── Case 1: record fits with no split ─────────────────────────────────
    if (target.canFit(record))
    {
        if (!target.insertRecordSorted(record))
        {
            std::cerr << "ERROR: insertRecordSorted failed\n";
            file.close();
            return false;
        }

        if (!file.writeBlock(candidateRBN, target))
        {
            std::cerr << "ERROR: failed writing updated block\n";
            file.close();
            return false;
        }

        header.recordCount += 1;
        header.staleFlag = false;

        if (!file.writeHeader(header))
        {
            std::cerr << "ERROR: failed updating blocked header\n";
            file.close();
            return false;
        }

        file.close();

        if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
        {
            std::cerr << "ERROR: failed rebuilding block index after insert\n";
            return false;
        }

        return true;
    }

    // ── Case 2: split required ────────────────────────────────────────────
    std::cout << "Split required at block RBN " << candidateRBN << "\n";

    std::vector<ZipCodeRecord> allRecords = target.getRecords();
    allRecords.push_back(record);

    std::sort(allRecords.begin(), allRecords.end(),
              [](const ZipCodeRecord& a, const ZipCodeRecord& b)
              {
                  return a.zipCode < b.zipCode;
              });

    const int oldPrev = target.getPrevRBN();
    const int oldNext = target.getNextRBN();

    const int newRBN = header.blockCount;

    BlockBuffer left(header.blockSize);
    BlockBuffer right(header.blockSize);

    left.clearToEmptyActive();
    right.clearToEmptyActive();

    left.setPrevRBN(oldPrev);
    left.setNextRBN(newRBN);

    right.setPrevRBN(candidateRBN);
    right.setNextRBN(oldNext);

    const size_t splitPoint = allRecords.size() / 2;

    for (size_t i = 0; i < splitPoint; ++i)
    {
        if (!left.insertRecordSorted(allRecords[i]))
        {
            std::cerr << "ERROR: failed inserting into left split block\n";
            file.close();
            return false;
        }
    }

    for (size_t i = splitPoint; i < allRecords.size(); ++i)
    {
        if (!right.insertRecordSorted(allRecords[i]))
        {
            std::cerr << "ERROR: failed inserting into right split block\n";
            file.close();
            return false;
        }
    }

    // Safety check: both halves must fit
    if (left.getRecordCount() == 0 || right.getRecordCount() == 0)
    {
        std::cerr << "ERROR: invalid split produced empty side\n";
        file.close();
        return false;
    }

    // Rewrite original block as left half
    if (!file.writeBlock(candidateRBN, left))
    {
        std::cerr << "ERROR: failed writing left split block\n";
        file.close();
        return false;
    }

    // Write new right block at file end position (newRBN)
    if (!file.writeBlock(newRBN, right))
    {
        std::cerr << "ERROR: failed writing right split block\n";
        file.close();
        return false;
    }

    // If there was a following block, update its prev pointer
    if (oldNext != -1)
    {
        BlockBuffer nextBlock(header.blockSize);
        if (!file.readBlock(oldNext, nextBlock))
        {
            std::cerr << "ERROR: failed reading successor block for relink\n";
            file.close();
            return false;
        }

        nextBlock.setPrevRBN(newRBN);

        if (!file.writeBlock(oldNext, nextBlock))
        {
            std::cerr << "ERROR: failed writing successor block during relink\n";
            file.close();
            return false;
        }
    }

    header.recordCount += 1;
    header.blockCount += 1;
    header.staleFlag = false;

    if (!file.writeHeader(header))
    {
        std::cerr << "ERROR: failed updating blocked header after split\n";
        file.close();
        return false;
    }

    file.close();

    std::cout << "Split created new block RBN " << newRBN << "\n";
    std::cout << "  Left block max key  : " << left.getHighestKey() << "\n";
    std::cout << "  Right block max key : " << right.getHighestKey() << "\n";

    if (!rebuildBlockIndex(SEQ_FILE, IDX_FILE))
    {
        std::cerr << "ERROR: failed rebuilding block index after split insert\n";
        return false;
    }

    return true;
}

// =============================================================================
// printRecord
// =============================================================================

void printRecord(const ZipCodeRecord& record)
{
    std::cout << "  ZIP Code  : " << std::setw(5) << std::setfill('0')
              << record.zipCode << "\n";
    std::cout << std::setfill(' ');
    std::cout << "  Place     : " << record.placeName << "\n";
    std::cout << "  State     : " << record.state << "\n";
    std::cout << "  County    : " << record.county << "\n";
    std::cout << "  Latitude  : " << std::fixed << std::setprecision(4)
              << record.latitude << "\n";
    std::cout << "  Longitude : " << std::fixed << std::setprecision(4)
              << record.longitude << "\n";
}