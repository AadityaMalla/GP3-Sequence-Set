/**
 * @file search_seqset.cpp
 * @brief Indexed ZIP code search for the GP 3.0 sequence-set file
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cctype>

#include "BlockedFileHeader.h"
#include "BlockBuffer.h"
#include "BlockFileBuffer.h"
#include "BlockIndex.h"
#include "ZipCodeRecord.h"

// ── File constants ────────────────────────────────────────────────────────────
static const std::string INPUT_SEQ_FILE = "data/us_postal_codes_seqset.txt";
static const std::string INPUT_IDX_FILE = "data/us_postal_codes_block_idx.txt";

// ── Forward declarations ──────────────────────────────────────────────────────
std::vector<int> parseZipArgs(int argc, char* argv[]);
bool searchOneZip(const std::string& seqFilename,
                  const BlockIndex& index,
                  int zipCode);

// =============================================================================
// main
// =============================================================================

int main(int argc, char* argv[])
{
    std::cout << "=============================================================\n";
    std::cout << "   GP 3.0 -- Indexed ZIP Search\n";
    std::cout << "=============================================================\n";

    std::vector<int> searchZips = parseZipArgs(argc, argv);

    if (searchZips.empty())
    {
        std::cout << "Usage: .\\search_seqset -Z56301 -Z10001 -Z90210\n";
        return 0;
    }

    // Load only the simple block index into RAM
    BlockIndex index(INPUT_IDX_FILE);
    if (!index.loadIndex())
    {
        std::cerr << "ERROR: cannot load block index file: "
                  << INPUT_IDX_FILE << "\n";
        return 1;
    }

    std::cout << "Index loaded: " << index.size() << " entries\n\n";

    for (int zip : searchZips)
    {
        std::cout << "-------------------------------------------------------------\n";
        std::cout << "Searching for ZIP: "
                  << std::setw(5) << std::setfill('0') << zip << "\n";
        std::cout << std::setfill(' ');

        if (!searchOneZip(INPUT_SEQ_FILE, index, zip))
        {
            std::cout << "*** ZIP code "
                      << std::setw(5) << std::setfill('0') << zip
                      << " not found. ***\n\n";
            std::cout << std::setfill(' ');
        }
    }

    return 0;
}

// =============================================================================
// parseZipArgs
// =============================================================================

std::vector<int> parseZipArgs(int argc, char* argv[])
{
    std::vector<int> zips;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg.size() >= 3 &&
            arg[0] == '-' &&
            (arg[1] == 'Z' || arg[1] == 'z'))
        {
            std::string digits = arg.substr(2);

            bool valid = !digits.empty();
            for (char ch : digits)
            {
                if (!std::isdigit(static_cast<unsigned char>(ch)))
                {
                    valid = false;
                    break;
                }
            }

            if (valid)
            {
                try
                {
                    zips.push_back(std::stoi(digits));
                }
                catch (...)
                {
                    std::cerr << "WARNING: invalid ZIP argument ignored: "
                              << arg << "\n";
                }
            }
            else
            {
                std::cerr << "WARNING: invalid ZIP argument ignored: "
                          << arg << "\n";
            }
        }
        else
        {
            std::cerr << "WARNING: unrecognized argument ignored: "
                      << arg << "\n";
        }
    }

    return zips;
}

// =============================================================================
// searchOneZip
// =============================================================================

bool searchOneZip(const std::string& seqFilename,
                  const BlockIndex& index,
                  int zipCode)
{
    int candidateRBN = -1;

    if (!index.findCandidateBlock(zipCode, candidateRBN))
    {
        return false;
    }

    std::cout << "Candidate block RBN from index: " << candidateRBN << "\n";

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
        std::cerr << "ERROR: cannot read sequence-set header\n";
        file.close();
        return false;
    }

    BlockBuffer block(header.blockSize);
    if (!file.readBlock(candidateRBN, block))
    {
        std::cerr << "ERROR: cannot read candidate block RBN "
                  << candidateRBN << "\n";
        file.close();
        return false;
    }

    file.close();

    std::cout << "Block read successfully.\n";
    std::cout << "Block contains " << block.getRecordCount()
              << " records.\n";

    ZipCodeRecord found;
    if (!block.findRecord(zipCode, found))
    {
        return false;
    }

    std::cout << "\nZIP record found:\n";
    std::cout << "  ZIP Code  : " << std::setw(5) << std::setfill('0')
              << found.zipCode << "\n";
    std::cout << std::setfill(' ');
    std::cout << "  Place     : " << found.placeName << "\n";
    std::cout << "  State     : " << found.state << "\n";
    std::cout << "  County    : " << found.county << "\n";
    std::cout << "  Latitude  : " << std::fixed << std::setprecision(4)
              << found.latitude << "\n";
    std::cout << "  Longitude : " << std::fixed << std::setprecision(4)
              << found.longitude << "\n\n";

    return true;
}