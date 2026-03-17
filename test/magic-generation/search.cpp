#include "search.hpp"
#include <map>
#include <iostream>
#include <iomanip>

void MagicSearch::entrypoint()
{
  // conduct a search for magic numbers
  while (!shouldStop)
  {
    // generate random magic number
    uint64_t magic = (static_cast<uint64_t>(rand()) << 32) + rand();

    // sizeof(std::array<uint64_t *, 64>);

    bool failed{true};
    // start at initial shift value (minimum possible)
    // stop if
    //   search stopped
    //   shift is less than best shift (more bits)
    //   last shift didn't fail
    // ergo, continue if
    //    search not stopped
    //    shift is more than best shift
    //    last shift failed

    // outputStream << "Testing magic: 0x" << std::setw(16) << std::hex << magic << "\n";
    // std::cout << outputStream.str();
    // outputStream.clear();

    for (int shift{bestPossibleShift}; !shouldStop && shift > bestShift && failed; shift--)
    {
      // start search for this shift
      failed = false;
      std::map<uint64_t, uint64_t> moveMap{};
      uint64_t maxIndex{0};
      // check if every occupancy gets a unique* index
      for (auto &occupancySet : occupancySets)
      {
        uint64_t index = occupancySet.first * magic;
        index >>= shift;

        // if this index provides a bad moveset
        if (moveMap.contains(index) && moveMap.at(index) != occupancySet.second)
        {
          // outputStream << "Shift " << std::dec << shift << " failed\n";
          // std::cout << outputStream.str();
          // outputStream.clear();
          failed = true;
          break;
        }
        moveMap[index] = occupancySet.second;
        if (index > maxIndex)
          maxIndex = index;
      }
      // if this set didn't fail!
      if (!failed)
      {
        // outputStream << "Shift " << std::dec << shift << " succeeded!\n";
        // std::cout << outputStream.str();
        // outputStream.clear();
        std::lock_guard lock(valueMutex);
        bestMagic = magic;
        bestShift = shift;
        bestMoveMap = moveMap;
        bestMaxIndex = maxIndex;
        newMagicFound = true;
        if (shift == bestPossibleShift)
          return;
      }
    } // shift for loop
  } // outer magic generation loop
}
