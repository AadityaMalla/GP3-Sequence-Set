#include <iostream>
#include <vector>
#include "BlockBuffer.h"
#include "ZipCodeRecord.h"

int main()
{
    BlockBuffer block(256);

    block.setPrevRBN(-1);
    block.setNextRBN(1);

    ZipCodeRecord a(56301, "Saint Cloud", "MN", "Stearns", 45.5608, -94.1622);
    ZipCodeRecord b(10001, "New York", "NY", "New York", 40.7506, -73.9970);
    ZipCodeRecord c(90210, "Beverly Hills", "CA", "Los Angeles", 34.0901, -118.4065);
    ZipCodeRecord d(501, "Holtsville", "NY", "Suffolk", 40.8154, -73.0451);

    std::cout << "Insert 56301: " << (block.insertRecordSorted(a) ? "OK" : "FAIL") << "\n";
    std::cout << "Insert 10001: " << (block.insertRecordSorted(b) ? "OK" : "FAIL") << "\n";
    std::cout << "Insert 90210: " << (block.insertRecordSorted(c) ? "OK" : "FAIL") << "\n";
    std::cout << "Insert 00501: " << (block.insertRecordSorted(d) ? "OK" : "FAIL") << "\n";

    std::cout << "\nOriginal block:\n";
    block.display();

    std::vector<char> raw(block.getBlockSize());

    if (!block.pack(raw.data()))
    {
        std::cout << "Pack failed.\n";
        return 1;
    }

    BlockBuffer restored(256);
    if (!restored.unpack(raw.data()))
    {
        std::cout << "Unpack failed.\n";
        return 1;
    }

    std::cout << "\nRestored block:\n";
    restored.display();

    ZipCodeRecord found;
    if (restored.findRecord(90210, found))
    {
        std::cout << "\nFound ZIP 90210:\n";
        found.display();
    }
    else
    {
        std::cout << "\nZIP 90210 not found.\n";
    }

    std::cout << "\nRemoving ZIP 10001...\n";
    restored.removeRecordByZip(10001);
    restored.display();

    std::cout << "\nTesting avail block...\n";
    BlockBuffer avail(256);
    avail.clearToAvail(7);
    avail.display();

    return 0;
}