/**
 * @file DataFileHeader.cpp
 * @brief Implementation of DataFileHeader
 * @author CSCI 331 Team 2
 * @date 04/01/2026
 */

#include "DataFileHeader.h"

// -----------------------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------------------

DataFileHeader::DataFileHeader()
    : fileStructureType("ZIPCODE_V2")
    , version(2)
    , headerSize(0)
    , recordSizeIntegerBytes(2)
    , sizeFormatType("binary")
    , sizeOfSizes(2)
    , sizeInclusionFlag(false)
    , primaryKeyIndexFileName("us_postal_codes.idx")
    , recordCount(0)
    , fieldCount(0)
    , primaryKeyFieldIndex(-1)
{
}

DataFileHeader::DataFileHeader(const std::string& structType, int ver)
    : fileStructureType(structType)
    , version(ver)
    , headerSize(0)
    , recordSizeIntegerBytes(2)
    , sizeFormatType("binary")
    , sizeOfSizes(2)
    , sizeInclusionFlag(false)
    , primaryKeyIndexFileName("us_postal_codes.idx")
    , recordCount(0)
    , fieldCount(0)
    , primaryKeyFieldIndex(-1)
{
}

// -----------------------------------------------------------------------------
// Field management
// -----------------------------------------------------------------------------

void DataFileHeader::addField(const std::string& name,
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

size_t DataFileHeader::getPackedSize() const
{
    size_t size = 0;

    // Strings including null terminator
    size += fileStructureType.length() + 1;
    size += sizeFormatType.length() + 1;
    size += primaryKeyIndexFileName.length() + 1;

    // Primitive members
    size += sizeof(version);
    size += sizeof(headerSize);
    size += sizeof(recordSizeIntegerBytes);
    size += sizeof(sizeOfSizes);
    size += sizeof(sizeInclusionFlag);
    size += sizeof(recordCount);
    size += sizeof(fieldCount);
    size += sizeof(primaryKeyFieldIndex);

    // Field descriptors
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

size_t DataFileHeader::pack(char* buffer) const
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

    writeString(fileStructureType);
    writeInt(version);
    writeInt(headerSize);
    writeInt(recordSizeIntegerBytes);
    writeString(sizeFormatType);
    writeInt(sizeOfSizes);
    writeBool(sizeInclusionFlag);
    writeString(primaryKeyIndexFileName);
    writeInt(recordCount);
    writeInt(fieldCount);
    writeInt(primaryKeyFieldIndex);

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

size_t DataFileHeader::unpack(const char* buffer)
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

    fields.clear();

    readString(fileStructureType);
    readInt(version);
    readInt(headerSize);
    readInt(recordSizeIntegerBytes);
    readString(sizeFormatType);
    readInt(sizeOfSizes);
    readBool(sizeInclusionFlag);
    readString(primaryKeyIndexFileName);
    readInt(recordCount);
    readInt(fieldCount);
    readInt(primaryKeyFieldIndex);

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

void DataFileHeader::display() const
{
    std::cout << "==========================================\n";
    std::cout << "Data File Header\n";
    std::cout << "==========================================\n";
    std::cout << "File Structure Type  : " << fileStructureType << "\n";
    std::cout << "Version              : " << version << "\n";
    std::cout << "Header Size          : " << headerSize << " bytes\n";
    std::cout << "Record Size Bytes    : " << recordSizeIntegerBytes << "\n";
    std::cout << "Size Format          : " << sizeFormatType << "\n";
    std::cout << "Size of Sizes        : " << sizeOfSizes << "\n";
    std::cout << "Size Inclusion Flag  : "
              << (sizeInclusionFlag ? "true" : "false") << "\n";
    std::cout << "PK Index File        : " << primaryKeyIndexFileName << "\n";
    std::cout << "Record Count         : " << recordCount << "\n";
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