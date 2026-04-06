/**
 * @file BlockedFileHeader.h
 * @brief Header structure for the GP 3.0 blocked sequence-set ZIP code file
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 *
 * @details
 * This class stores file-level metadata for the blocked sequence-set file.
 * It extends the ideas from the GP 2.0 header and adds blocked-file fields:
 *   - block size
 *   - minimum block capacity
 *   - active list head
 *   - avail list head
 *   - block count
 *   - stale flag
 *   - index file metadata
 *
 * The header is stored as the FIRST length-indicated record in the file,
 * just like in Project 2.0.
 */

#ifndef BLOCKEDFILEHEADER_H
#define BLOCKEDFILEHEADER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>
#include "DataFileHeader.h"

/**
 * @class BlockedFileHeader
 * @brief Stores metadata for a blocked sequence-set file.
 *
 * @details
 * Responsibilities:
 *   - store file-level metadata
 *   - store field descriptions
 *   - pack/unpack itself into a raw buffer
 *   - display itself for debugging
 *
 * NOT responsible for:
 *   - opening or closing files
 *   - reading/writing streams directly
 */
class BlockedFileHeader
{
public:
    // -- General file metadata ---------------------------------------------
    std::string fileStructureType;      /**< e.g. "ZIPCODE_BSS_V1" */
    int         version;                /**< File format version   */
    int         headerSize;             /**< Packed header size    */

    // -- Size format metadata ----------------------------------------------
    int         recordSizeIntegerBytes; /**< Bytes used for LI size fields */
    std::string sizeFormatType;         /**< e.g. "binary" */
    int         sizeOfSizes;            /**< Size of size fields */
    bool        sizeInclusionFlag;      /**< Whether size includes itself */

    // -- GP 3.0 blocked-file metadata --------------------------------------
    int         blockSize;              /**< Physical bytes per block */
    double      minBlockCapacity;       /**< Minimum occupancy ratio (0.50 default) */
    int         recordCount;            /**< Total active records in file */
    int         blockCount;             /**< Total physical blocks in file */
    int         activeListHead;         /**< RBN of first active block; -1 if none */
    int         availListHead;          /**< RBN of first avail block; -1 if none */
    bool        staleFlag;              /**< True if index is out-of-date */

    // -- Index metadata ----------------------------------------------------
    std::string primaryKeyIndexFileName;/**< Associated block-index filename */
    std::string indexFileStructureType; /**< e.g. "ZIPCODE_BLOCK_INDEX_V1" */
    int         primaryKeyFieldIndex;   /**< Which field is the PK */

    // -- Field metadata ----------------------------------------------------
    int fieldCount;                     /**< Number of fields in each record */
    std::vector<FieldDescriptor> fields;/**< Field descriptors */

    // -- Constructors ------------------------------------------------------
    BlockedFileHeader();
    BlockedFileHeader(const std::string& structType, int ver);

    // -- Field management --------------------------------------------------
    void addField(const std::string& name,
                  const std::string& typeSchema,
                  bool isPrimaryKey = false);

    // -- Serialization -----------------------------------------------------
    size_t getPackedSize() const;
    size_t pack(char* buffer) const;
    size_t unpack(const char* buffer);

    // -- Utility -----------------------------------------------------------
    void display() const;
};

#endif // BLOCKEDFILEHEADER_H