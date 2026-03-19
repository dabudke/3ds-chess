/*
create individual threads for each square for rooks and bishops
64*64 threads

each thread computes a magic number and shift value for the corresponding
square (and direction)'s magic bitboard

s - saves to a file (comma separated values, newline separated lists)
    [orthMagics, orthShifts, diagMagics, diagShifts]
*/

#include <cinttypes>
#include <array>
#include <iostream>
#include <bitset>
#include "bitboards.hpp"
#include "search.hpp"

#include <ncurses.h>
#include <iomanip>
#include <fstream>

struct MagicMapEntry
{
  int shift{0};
  uint64_t magic{0};
  bool perfect{false};
  int collisions{0};
  std::map<uint64_t, uint64_t> moveMap{};
};
int main(int, char **)
{
  std::setfill("0");
  std::vector<std::pair<std::shared_ptr<MagicSearch>, std::shared_ptr<MagicSearch>>> magicSearches{};
  std::array<std::pair<MagicMapEntry, MagicMapEntry>, 64> magicMap{};
  size_t orthBaselineLength{0};
  size_t diagBaselineLength{0};

  // orthogonal magics
  for (int i{0}; i < 64; i++)
  {
    int orthPossibilityBits = std::popcount(orthOccupancyMasks[i]);
    auto orthSearch = std::make_shared<MagicSearch>(orthOccupancySets[i], 64 - orthPossibilityBits);
    orthBaselineLength += (1ul << orthPossibilityBits);

    int diagPossibilityBits = std::popcount(diagOccupancyMasks[i]);
    auto diagSearch = std::make_shared<MagicSearch>(diagOccupancySets[i], 64 - diagPossibilityBits);
    diagBaselineLength += (1ul << diagPossibilityBits);

    magicSearches.emplace_back(orthSearch, diagSearch);
  }

  static const size_t baseSize = 612 + 64 + 256;
  const size_t baselineOrthSize{orthBaselineLength * 8 + baseSize};
  const size_t baselineDiagSize{diagBaselineLength * 8 + baseSize};

  int maxX, maxY;
  bool running{true};

  initscr();
  cbreak();
  noecho();
  curs_set(0);
  nodelay(stdscr, true);
  getmaxyx(stdscr, maxY, maxX);

  start_color();
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_RED, COLOR_BLACK);

  refresh();

  int halfSize = maxX / 2;
  WINDOW *totalSizeBox = newwin(4, maxX, 0, 0),
         *orthBox = newwin(maxY - 4, halfSize, 4, 0),
         *diagBox = newwin(maxY - 4, halfSize, 4, halfSize + 1);

  WINDOW *totalSize = derwin(totalSizeBox, 2, maxX - 56, 1, 1),
         *orthList = derwin(orthBox, maxY - 6, halfSize - 2, 1, 1),
         *diagList = derwin(diagBox, maxY - 6, halfSize - 2, 1, 1);

  if (!totalSizeBox || !orthBox || !diagBox || !totalSize || !orthList || !diagList)
  {
    endwin();
    throw std::runtime_error("window error");
  }

  box(totalSizeBox, 0, 0);
  box(orthBox, 0, 0);
  box(diagBox, 0, 0);

  mvwprintw(totalSizeBox, 0, 2, " Total Size ");
  mvwprintw(orthBox, 0, 2, " Orthogonal ");
  mvwprintw(diagBox, 0, 2, " Diagonal ");

  wrefresh(totalSizeBox);
  wrefresh(orthBox);
  wrefresh(diagBox);

  // setup all magics table

  bool update{true};
  while (running)
  {
    switch (getch())
    {
    case 'q':
      running = false;
      break;
    default:
      break;
    }

    for (int i{0}; i < 64; i++)
    {
      auto &[orth, diag] = magicSearches[i];
      if (orth->newMagicFound)
      {
        auto &orthMap = magicMap[i].first;
        {
          std::lock_guard lock(orth->valueMutex);
          orthMap.magic = orth->bestMagic;
          orthMap.shift = orth->bestShift;
          orthMap.collisions = orth->bestMapCollisions;
          orthMap.moveMap = orth->bestMoveMap;
          orth->newMagicFound = false;
        }
        update = true;

        uint32_t maxIndex = orthMap.moveMap.rbegin()->first;
        size_t arraySize = 8 + 1 + 8 + (maxIndex + 1) * 8;
        mvwprintw(orthList, i, 1, "%2d 0x%016llX >> %02d (%d c, %lu m, %.02fkb)", i, orthMap.magic, orthMap.shift, orthMap.collisions, maxIndex, arraySize / 1000.0f);
        wclrtoeol(orthList);
      }

      if (diag->newMagicFound)
      {
        auto &diagMap = magicMap[i].second;
        {
          std::lock_guard lock(diag->valueMutex);
          diagMap.magic = diag->bestMagic;
          diagMap.shift = diag->bestShift;
          diagMap.collisions = diag->bestMapCollisions;
          diagMap.moveMap = diag->bestMoveMap;
          diag->newMagicFound = false;
        }
        update = true;

        uint32_t maxIndex = diagMap.moveMap.rbegin()->first;
        size_t arraySize = 8 + 1 + 8 + (maxIndex + 1) * 8;
        mvwprintw(diagList, i, 1, "%2d 0x%016llX >> %02d (%d c, %lu m, %.02fkb)", i, diagMap.magic, diagMap.shift, diagMap.collisions, maxIndex, arraySize / 1000.0f);
        wclrtoeol(diagList);
      }
    }

    // why is this not firing.
    if (update)
    {

      // draw all orthogonal and diagonal magics
      size_t orthSize{0};
      size_t diagSize{0};
      for (auto &[orth, diag] : magicMap)
      {
        orthSize += (orth.moveMap.rbegin()->first + 1);
        diagSize += (diag.moveMap.rbegin()->first + 1);
      }
      orthSize *= 8;
      orthSize += baseSize;
      diagSize *= 8;
      diagSize += baseSize;

      // calculate total sizes, best sizes
      mvwprintw(totalSize, 0, 1, "Orthogonal: ");
      if (baselineOrthSize >= orthSize)
        wattron(totalSize, COLOR_PAIR(1));
      wattron(totalSize, A_BOLD);
      wprintw(totalSize, "%.3f KB", orthSize / 1000.0f);
      wattroff(totalSize, COLOR_PAIR(1) | A_BOLD);
      wprintw(totalSize, "/%.3f KB", baselineOrthSize / 1000.0f);
      wclrtoeol(totalSize);

      mvwprintw(totalSize, 1, 1, "Diagonal: ");
      if (baselineDiagSize >= diagSize)
        wattron(totalSize, COLOR_PAIR(1));
      wattron(totalSize, A_BOLD);
      wprintw(totalSize, "%.3f KB", diagSize / 1000.0f);
      wattroff(totalSize, COLOR_PAIR(1) | A_BOLD);
      wprintw(totalSize, "/%.3f KB", baselineDiagSize / 1000.0f);
      wclrtoeol(totalSize);

      wrefresh(totalSize);
      wrefresh(orthList);
      wrefresh(diagList);

      update = false;
    }
  }

  endwin();

  for (auto &[orth, diag] : magicSearches)
  {
    orth->stop();
    std::cout << "0x" << std::hex << orth->bestMagic << " " << std::dec << orth->bestShift << std::endl;
    diag->stop();
    std::cout << "0x" << std::hex << diag->bestMagic << " " << std::dec << diag->bestShift << std::endl;
  }

  // save last bits of information
  // for (int i{0}; i < 64; i++)
  // {
  //   auto &[orth, diag] = magicSearches[i];
  //   auto &[orthMap, diagMap] = magicMap[i];

  //   orth->stop();
  //   orthMap.magic = orth->bestMagic;
  //   orthMap.shift = orth->bestShift;
  //   orthMap.moveMap = orth->bestMoveMap;

  //   diag->stop();
  //   diagMap.magic = diag->bestMagic;
  //   diagMap.shift = diag->bestShift;
  //   diagMap.moveMap = diag->bestMoveMap;
  // }

  for (int i{0}; i < 64; i++)
  {
    auto &[orth, diag] = magicSearches[i];

    std::string orthOutputPath = std::format("magics/magic-o{}.txt", i);
    std::remove(orthOutputPath.c_str());
    std::ofstream orthOutput(orthOutputPath);
    orthOutput << orth->bestMagic << ";" << orth->bestShift << ";{";
    unsigned int maxIndex = orth->bestMoveMap.rbegin()->first;
    for (int i{0}; i < maxIndex; i++)
    {
      if (orth->bestMoveMap.contains(i))
      {
        orthOutput << orth->bestMoveMap[i];
      }
      else
      {
        orthOutput << "0";
      }
      if (i < maxIndex - 1)
      {
        orthOutput << ",";
      }
    }
    orthOutput << "}\n";

    std::string diagOutputPath = std::format("magics/magic-d{}.txt", i);
    std::remove(diagOutputPath.c_str());

    std::ofstream diagOutput(diagOutputPath);
    diagOutput << diag->bestMagic << "," << diag->bestShift << ",{";
    maxIndex = diag->bestMoveMap.rbegin()->first;
    for (int i{0}; i < maxIndex; i++)
    {
      if (diag->bestMoveMap.contains(i))
      {
        diagOutput << diag->bestMoveMap[i];
      }
      else
      {
        diagOutput << "0";
      }
      if (i < maxIndex - 1)
      {
        diagOutput << ",";
      }
    }
    diagOutput << "}\n";
  }
}
