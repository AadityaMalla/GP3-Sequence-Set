/**
 * @file BlockedFileHeader.cpp
 * @brief Implementation of BlockedFileHeader
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "BlockedFileHeader.h"

// -----------------------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------------------

BlockedFileHeader::BlockedFileHeader()
    : fileStructureType("ZIPCODE_BSS_V1")
    , version(1)
    , headerSize(0)
    , recordSizeIntegerBytes(2)
    , sizeFormatType("binary")
    , sizeOfSizes(2)
    , sizeInclusionFlag(false)
    , blockSize(512)
    , minBlockCapacity(0.50)
    , recordCount(0)
    , blockCount(0)
    , activeListHead(-1)
    , availListHead(-1)
    , staleFlag(false)
    , primaryKeyIndexFileName("us_postal_codes_block_idx.txt")
    , indexFileStructureType("ZIPCODE_BLOCK_INDEX_V1")
    , primaryKeyFieldIndex(-1)
    , fieldCount(0)
{
}

BlockedFileHeader::BlockedFileHeader(const std::string& structType, int ver)
    : fileStructureType(structType)
    , version(ver)
    , headerSize(0)
    , recordSizeIntegerBytes(2)
    , sizeFormatType("binary")
    , sizeOfSizes(2)
    , sizeInclusionFlag(false)
    , blockSize(512)
    , minBlockCapacity(0.50)
    , recordCount(0)
    , blockCount(0)
    , activeListHead(-1)
    , availListHead(-1)
    , staleFlag(false)
    , primaryKeyIndexFileName("us_postal_codes_block_idx.txt")
    , indexFileStructureType("ZIPCODE_BLOCK_INDEX_V1")
    , primaryKeyFieldIndex(-1)
    , fieldCount(0)
{
}

// -----------------------------------------------------------------------------
// addField
// -----------------------------------------------------------------------------

void BlockedFileHeader::addField(const std::string& name,
                                 const std::string& typeSchema,
                                 bool isPrimaryKey)
{
    fields.push_back(FieldDescriptor(name, typeSchema, isPrimaryKey));
    fieldCount = static_cast<int>(fields.size());

    if (isPrimaryKey)
        primaryKeyFieldIndex = fieldCount - 1;
}

// -----------------------------------------------------------------------------
// getPackedSize
// -----------------------------------------------------------------------------

size_t BlockedFileHeader::getPackedSize() const
{
    size_t size = 0;

    // strings with null terminators
    size += fileStructureType.length() + 1;
    size += sizeFormatType.length() + 1;
    size += primaryKeyIndexFileName.length() + 1;
    size += indexFileStructureType.length() + 1;

    // primitive fields
    size += sizeof(version);
    size += sizeof(headerSize);
    size += sizeof(recordSizeIntegerBytes);
    size += sizeof(sizeOfSizes);
    size += sizeof(sizeInclusionFlag);

    size += sizeof(blockSize);
    size += sizeof(minBlockCapacity);
    size += sizeof(recordCount);
    size += sizeof(blockCount);
    size += sizeof(activeListHead);
    size += sizeof(availListHead);
    size += sizeof(staleFlag);

    size += sizeof(primaryKeyFieldIndex);
    size += sizeof(fieldCount);

    // field descriptors
    for (const auto& field : fields)
    {
        size += field.name.length() + 1;
        size += field.typeSchema.length() + 1;
        size += sizeof(field.isPrimaryKey);
    }

    return size;
}

// -----------------------------------------------------------------------------
// pack
// -----------------------------------------------------------------------------

size_t BlockedFileHeader::pack(char* buffer) const
{
    char* p = buffer;

    auto writeString = [&p](const std::string& s)
    {
        std::memcpy(p, s.c_str(), s.length() + 1);
        p += s.length() + 1;
    };

    auto writeInt = [&p](int value)
    {
        std::memcpy(p, &value, sizeof(value));
        p += sizeof(value);
    };

    auto writeBool = [&p](bool value)
    {
        std::memcpy(p, &value, sizeof(value));
        p += sizeof(value);
    };

    auto writeDouble = [&p](double value)
    {
        std::memcpy(p, &value, sizeof(value));
        p += sizeof(value);
    };

    writeString(fileStructureType);
    writeInt(version);
    writeInt(headerSize);
    writeInt(recordSizeIntegerBytes);
    writeString(sizeFormatType);
    writeInt(sizeOfSizes);
    writeBool(sizeInclusionFlag);

    writeInt(blockSize);
    writeDouble(minBlockCapacity);
    writeInt(recordCount);
    writeInt(blockCount);
    writeInt(activeListHead);
    writeInt(availListHead);
    writeBool(staleFlag);

    writeString(primaryKeyIndexFileName);
    writeString(indexFileStructureType);
    writeInt(primaryKeyFieldIndex);
    writeInt(fieldCount);

    for (const auto& field : fields)
    {
        writeString(field.name);
        writeString(field.typeSchema);
        writeBool(field.isPrimaryKey);
    }

    return static_cast<size_t>(p - buffer);
}

// -----------------------------------------------------------------------------
// unpack
// -----------------------------------------------------------------------------

size_t BlockedFileHeader::unpack(const char* buffer)
{
    const char* p = buffer;

    auto readString = [&p](std::string& s)
    {
        s = p;
        p += s.length() + 1;
    };

    auto readInt = [&p](int& value)
    {
        std::memcpy(&value, p, sizeof(value));
        p += sizeof(value);
    };

    auto readBool = [&p](bool& value)
    {
        std::memcpy(&value, p, sizeof(value));
        p += sizeof(value);
    };

    auto readDouble = [&p](double& value)
    {
        std::memcpy(&value, p, sizeof(value));
        p += sizeof(value);
    };

    fields.clear();

    readString(fileStructureType);
    readInt(version);
    readInt(headerSize);
    readInt(recordSizeIntegerBytes);
    readString(sizeFormatType);
    readInt(sizeOfSizes);
    readBool(sizeInclusionFlag);

    readInt(blockSize);
    readDouble(minBlockCapacity);
    readInt(recordCount);
    readInt(blockCount);
    readInt(activeListHead);
    readInt(availListHead);
    readBool(staleFlag);

    readString(primaryKeyIndexFileName);
    readString(indexFileStructureType);
    readInt(primaryKeyFieldIndex);
    readInt(fieldCount);

    for (int i = 0; i < fieldCount; ++i)
    {
        FieldDescriptor fd;
        readString(fd.name);
        readString(fd.typeSchema);
        readBool(fd.isPrimaryKey);
        fields.push_back(fd);
    }

    return static_cast<size_t>(p - buffer);
}

// -----------------------------------------------------------------------------
// display
// -----------------------------------------------------------------------------

void BlockedFileHeader::display() const
{
    std::cout << "==========================================\n";
    std::cout << "Blocked Sequence-Set File Header\n";
    std::cout << "==========================================\n";
    std::cout << "File Structure Type  : " << fileStructureType << "\n";
    std::cout << "Version              : " << version << "\n";
    std::cout << "Header Size          : " << headerSize << " bytes\n";
    std::cout << "Record Size Bytes    : " << recordSizeIntegerBytes << "\n";
    std::cout << "Size Format          : " << sizeFormatType << "\n";
    std::cout << "Size of Sizes        : " << sizeOfSizes << "\n";
    std::cout << "Size Inclusion Flag  : "
              << (sizeInclusionFlag ? "true" : "false") << "\n";

    std::cout << "Block Size           : " << blockSize << "\n";
    std::cout << "Min Block Capacity   : " << std::fixed << std::setprecision(2)
              << minBlockCapacity << "\n";
    std::cout << "Record Count         : " << recordCount << "\n";
    std::cout << "Block Count          : " << blockCount << "\n";
    std::cout << "Active List Head     : " << activeListHead << "\n";
    std::cout << "Avail List Head      : " << availListHead << "\n";
    std::cout << "Stale Flag           : " << (staleFlag ? "true" : "false") << "\n";

    std::cout << "PK Index File        : " << primaryKeyIndexFileName << "\n";
    std::cout << "Index Structure Type : " << indexFileStructureType << "\n";
    std::cout << "Field Count          : " << fieldCount << "\n";
    std::cout << "PK Field Index       : " << primaryKeyFieldIndex << "\n";

    std::cout << "\nFields:\n";
    std::cout << "------------------------------------------\n";
    for (size_t i = 0; i < fields.size(); ++i)
    {
        std::cout << "  [" << i << "] "
                  << std::left << std::setw(16) << fields[i].name
                  << " (" << std::setw(8) << fields[i].typeSchema << ")";
        if (fields[i].isPrimaryKey)
            std::cout << "  [PRIMARY KEY]";
        std::cout << "\n";
    }
    std::cout << "==========================================\n";
}