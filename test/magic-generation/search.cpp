#include "search.hpp"
#include <map>

void MagicSearch::entrypoint()
{
  // conduct a search for magic numbers
  while (!shouldStop)
  {
    // generate random magic number
    uint64_t magic = (static_cast<uint64_t>(rand()) << 32) + rand();

    bool failed{true};
    // start at initial shift value (minimum possible)
    // continue if:
    //   search not stopped
    //   shift is under 20
    //   last search failed
    for (uint64_t shift{initialShift}; !shouldStop && shift < 20 && failed; shift++)
    {
      // start search for this shift
      failed = false;
      std::map<uint64_t, uint64_t> moveMap{};
      // check if every occupancy gets a unique* index
      for (auto &occupancySet : occupancySets)
      {
        uint64_t index = occupancySet.first * magic;
        index >>= shift;

        // if this index provides a bad moveset
        if (moveMap.contains(index) && moveMap.at(index) != occupancySet.second)
        {
          failed = true;
          break;
        }
        moveMap[index] = occupancySet.second;
      }
      // if this set didn't fail!
      if (!failed)
      {
        std::lock_guard lock(valueLock);
        bestMagic = magic;
        bestShift = shift;
        bestMoveMap = moveMap;
        newMagicFound = true;
      }
    } // shift for loop
  } // outer magic generation loop
}
