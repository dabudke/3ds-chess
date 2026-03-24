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

    bool succeeded{true};
    int collisions{0};
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

    for (int shift{bestShift + 1}; !shouldStop && succeeded && shift < 64; shift++)
    {
      collisions = 0;
      // start search for this shift
      std::map<uint64_t, uint64_t> moveMap{};
      // check if every occupancy gets a unique* index
      for (auto &occupancySet : occupancySets)
      {
        uint64_t index = occupancySet.first * magic;
        index >>= shift;

        // if this index provides a bad moveset
        if (moveMap.contains(index))
        {
          if (moveMap.at(index) != occupancySet.second)
          {
            succeeded = false;
            break;
          }
          collisions++;
        }
        moveMap[index] = occupancySet.second;
      }
      // if this set didn't fail!
      if (succeeded)
      {
        // outputStream << "Shift " << std::dec << shift << " succeeded!\n";
        // std::cout << outputStream.str();
        // outputStream.clear();
        std::lock_guard lock(valueMutex);
        bestMagic = magic;
        bestShift = shift;
        bestMoveMap = moveMap;
        bestMapCollisions = collisions;
        newMagicFound = true;
      }
    } // shift for loop
  } // outer magic generation loop
}
