#include <iostream>
#include "BlockFileBuffer.h"
#include "BlockBuffer.h"
#include "BlockedFileHeader.h"
#include "ZipCodeRecord.h"

int main()
{
    const std::string filename = "test_seqset.dat";

    // -- Step 1: Create header ---------------------------------------------
    BlockedFileHeader header;
    header.blockSize = 256;
    header.minBlockCapacity = 0.50;
    header.recordCount = 4;
    header.blockCount = 2;
    header.activeListHead = 0;
    header.availListHead = -1;
    header.primaryKeyIndexFileName = "test_block_index.dat";

    header.addField("zipCode", "int", true);
    header.addField("placeName", "string");
    header.addField("state", "string");
    header.addField("county", "string");
    header.addField("latitude", "double");
    header.addField("longitude", "double");

    // -- Step 2: Create two blocks -----------------------------------------
    BlockBuffer block0(256);
    block0.setPrevRBN(-1);
    block0.setNextRBN(1);
    block0.insertRecordSorted(
        ZipCodeRecord(501, "Holtsville", "NY", "Suffolk", 40.8154, -73.0451));
    block0.insertRecordSorted(
        ZipCodeRecord(10001, "New York", "NY", "New York", 40.7506, -73.9970));

    BlockBuffer block1(256);
    block1.setPrevRBN(0);
    block1.setNextRBN(-1);
    block1.insertRecordSorted(
        ZipCodeRecord(56301, "Saint Cloud", "MN", "Stearns", 45.5608, -94.1622));
    block1.insertRecordSorted(
        ZipCodeRecord(90210, "Beverly Hills", "CA", "Los Angeles", 34.0901, -118.4065));

    // -- Step 3: Write file ------------------------------------------------
    BlockFileBuffer file(filename);

    if (!file.openForWrite())
    {
        std::cout << "Failed: openForWrite\n";
        return 1;
    }

    if (!file.writeHeader(header))
    {
        std::cout << "Failed: writeHeader\n";
        return 1;
    }

    if (!file.writeBlock(0, block0))
    {
        std::cout << "Failed: writeBlock(0)\n";
        return 1;
    }

    if (!file.writeBlock(1, block1))
    {
        std::cout << "Failed: writeBlock(1)\n";
        return 1;
    }

    file.close();

    // -- Step 4: Read file back --------------------------------------------
    if (!file.openForRead())
    {
        std::cout << "Failed: openForRead\n";
        return 1;
    }

    BlockedFileHeader restoredHeader;
    if (!file.readHeader(restoredHeader))
    {
        std::cout << "Failed: readHeader\n";
        return 1;
    }

    BlockBuffer restored0(256);
    BlockBuffer restored1(256);

    if (!file.readBlock(0, restored0))
    {
        std::cout << "Failed: readBlock(0)\n";
        return 1;
    }

    if (!file.readBlock(1, restored1))
    {
        std::cout << "Failed: readBlock(1)\n";
        return 1;
    }

    file.close();

    // -- Step 5: Display results -------------------------------------------
    std::cout << "\nRESTORED HEADER\n";
    restoredHeader.display();

    std::cout << "\nRESTORED BLOCK 0\n";
    restored0.display();

    std::cout << "\nRESTORED BLOCK 1\n";
    restored1.display();

    return 0;
}