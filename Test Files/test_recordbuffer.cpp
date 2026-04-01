#include <iostream>
#include <vector>
#include "RecordBuffer.h"
#include "ZipCodeRecord.h"

int main()
{
    RecordBuffer rb;

    ZipCodeRecord original(501, "Holtsville", "NY", "Suffolk", 40.8154, -73.0451);

    size_t packedSize = rb.getPackedSize(original);
    std::vector<char> buffer(packedSize);

    size_t written = rb.pack(original, buffer.data());

    ZipCodeRecord restored;
    size_t consumed = rb.unpack(buffer.data(), restored);

    std::cout << "Packed size : " << packedSize << "\n";
    std::cout << "Bytes written: " << written << "\n";
    std::cout << "Bytes read   : " << consumed << "\n\n";

    std::cout << "Original record:\n";
    original.display();

    std::cout << "\nRestored record:\n";
    restored.display();

    return 0;
}