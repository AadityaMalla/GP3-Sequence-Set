/**
 * @file BlockIndex.cpp
 * @brief Implementation of BlockIndex
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "BlockIndex.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

BlockIndex::BlockIndex(const std::string& indexFilename)
    : indexFilename(indexFilename)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic operations
// ─────────────────────────────────────────────────────────────────────────────

void BlockIndex::clear()
{
    entries.clear();
}

void BlockIndex::addEntry(int maxKey, int rbn)
{
    entries.emplace_back(maxKey, rbn);
}

void BlockIndex::sortEntries()
{
    std::sort(entries.begin(), entries.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// File I/O
// ─────────────────────────────────────────────────────────────────────────────

bool BlockIndex::writeIndex() const
{
    std::ofstream out(indexFilename, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        std::cerr << "BlockIndex::writeIndex -- cannot open \""
                  << indexFilename << "\" for writing\n";
        return false;
    }

    int32_t count = static_cast<int32_t>(entries.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& entry : entries)
    {
        out.write(reinterpret_cast<const char*>(&entry.maxKey), sizeof(entry.maxKey));
        out.write(reinterpret_cast<const char*>(&entry.rbn),    sizeof(entry.rbn));
    }

    if (!out.good())
    {
        std::cerr << "BlockIndex::writeIndex -- write error\n";
        return false;
    }

    return true;
}

bool BlockIndex::loadIndex()
{
    entries.clear();

    std::ifstream in(indexFilename, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "BlockIndex::loadIndex -- cannot open \""
                  << indexFilename << "\" for reading\n";
        return false;
    }

    int32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    if (!in.good() || count < 0)
    {
        std::cerr << "BlockIndex::loadIndex -- malformed entry count\n";
        return false;
    }

    entries.reserve(static_cast<size_t>(count));

    for (int32_t i = 0; i < count; ++i)
    {
        BlockIndexEntry entry;
        in.read(reinterpret_cast<char*>(&entry.maxKey), sizeof(entry.maxKey));
        in.read(reinterpret_cast<char*>(&entry.rbn),    sizeof(entry.rbn));

        if (!in.good())
        {
            std::cerr << "BlockIndex::loadIndex -- read error at entry "
                      << i << "\n";
            return false;
        }

        entries.push_back(entry);
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Search
// ─────────────────────────────────────────────────────────────────────────────

bool BlockIndex::findCandidateBlock(int zipCode, int& rbn) const
{
    if (entries.empty())
        return false;

    BlockIndexEntry key(zipCode, 0);

    auto it = std::lower_bound(entries.begin(), entries.end(), key);

    if (it == entries.end())
        return false;

    rbn = it->rbn;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

size_t BlockIndex::size() const
{
    return entries.size();
}

std::string BlockIndex::getFilename() const
{
    return indexFilename;
}

const std::vector<BlockIndexEntry>& BlockIndex::getEntries() const
{
    return entries;
}

// ─────────────────────────────────────────────────────────────────────────────
// display
// ─────────────────────────────────────────────────────────────────────────────

void BlockIndex::display() const
{
    std::cout << "==========================================\n";
    std::cout << "Simple Block Index\n";
    std::cout << "==========================================\n";
    std::cout << "Index File : " << indexFilename << "\n";
    std::cout << "Entry Count: " << entries.size() << "\n\n";

    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::cout << "[" << i << "] "
                  << "maxKey=" << entries[i].maxKey
                  << " -> RBN=" << entries[i].rbn << "\n";
    }

    std::cout << "==========================================\n";
}
