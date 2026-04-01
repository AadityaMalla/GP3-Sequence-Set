#include <iostream>
#include "BlockIndex.h"

int main()
{
    BlockIndex index("test_block_index.dat");

    index.addEntry(99107, 3836);
    index.addEntry(99121, 3837);
    index.addEntry(99133, 3838);
    index.addEntry(99146, 3839);

    index.sortEntries();

    std::cout << "Original in-memory index:\n";
    index.display();

    if (!index.writeIndex())
    {
        std::cout << "Failed writing index.\n";
        return 1;
    }

    BlockIndex loaded("test_block_index.dat");
    if (!loaded.loadIndex())
    {
        std::cout << "Failed loading index.\n";
        return 1;
    }

    std::cout << "\nLoaded index:\n";
    loaded.display();

    int rbn = -1;

    if (loaded.findCandidateBlock(99110, rbn))
        std::cout << "\nCandidate block for 99110: " << rbn << "\n";
    else
        std::cout << "\nNo candidate block for 99110\n";

    if (loaded.findCandidateBlock(99146, rbn))
        std::cout << "Candidate block for 99146: " << rbn << "\n";
    else
        std::cout << "No candidate block for 99146\n";

    if (loaded.findCandidateBlock(99999, rbn))
        std::cout << "Candidate block for 99999: " << rbn << "\n";
    else
        std::cout << "No candidate block for 99999\n";

    return 0;
}