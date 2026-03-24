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
#include <unistd.h>
#include <csignal>
#include <ncurses.h>
#include <iomanip>
#include <fstream>
#include <memory>
#include <format>

std::atomic_bool terminate{false};

extern "C" void signalHandler(int signum)
{
  std::cout << "Terminate signal recieved\n";
  terminate = true;
}

struct MagicMapEntry
{
  int shift{0};
  uint64_t magic{0};
  int collisions{0};
  std::map<uint64_t, uint64_t> moveMap;
};

void saveToFile(const std::string &filename, std::array<std::pair<MagicMapEntry, MagicMapEntry>, 64> &magicMap)
{
  std::remove(filename.c_str());
  std::stringstream orthMagics{};
  std::stringstream orthShifts{};
  std::stringstream orthMap{};
  std::stringstream diagMagics{};
  std::stringstream diagShifts{};
  std::stringstream diagMap{};

  for (int i{0}; i < 64; i++)
  {
    auto &[orth, diag] = magicMap[i];

    orthMagics << orth.magic << "ull";
    orthShifts << orth.shift << "u";
    orthMap << "{";
    unsigned int maxIndex = orth.moveMap.rbegin()->first;
    for (int i{0}; i <= maxIndex; i++)
    {
      if (orth.moveMap.contains(i))
      {
        orthMap << orth.moveMap[i] << "ull";
      }
      else
      {
        orthMap << "0ull";
      }
      if (i < maxIndex)
      {
        orthMap << ",";
      }
    }
    orthMap << "}";

    diagMagics << diag.magic << "ull";
    diagShifts << diag.shift << "u";
    diagMap << "{";
    maxIndex = diag.moveMap.rbegin()->first;
    for (int i{0}; i <= maxIndex; i++)
    {
      if (diag.moveMap.contains(i))
      {
        diagMap << diag.moveMap[i] << "ull";
      }
      else
      {
        diagMap << "0ull";
      }
      if (i < maxIndex)
      {
        diagMap << ",";
      }
    }
    diagMap << "}";

    if (i != 63)
    {
      orthMagics << ",";
      orthShifts << ",";
      orthMap << ",\n";
      diagMagics << ",";
      diagShifts << ",";
      diagMap << ",\n";
    }
  }

  std::ofstream magicOutput(filename.c_str());
  magicOutput << "{" << orthMagics.str() << "}\n{" << orthShifts.str() << "}\n{" << orthMap.str() << "}\n\n{"
              << diagMagics.str() << "}\n{" << diagShifts.str() << "}\n{" << diagMap.str() << "}\n";
}

int main(int argc, char **argv)
{
  char opt;
  std::array<std::pair<MagicMapEntry, MagicMapEntry>, 64> magicMap{};
  bool doOrthogonal{false};
  bool doDiagonal{false};
  bool gui{false};
  std::string outputFilename{"./magic-output.txt"};

  while ((opt = getopt(argc, argv, "l:odgO:")) != -1)
  {
    if (opt == 'o')
      doOrthogonal = true;
    if (opt == 'd')
      doDiagonal = true;
    if (opt == 'g')
      gui = true;
    if (opt == 'O')
      outputFilename = optarg;
    if (opt == 'l')
    {
      std::ifstream magicLoad(optarg);

      magicLoad.ignore(); // '{'
      for (int i{0}; i < 64; i++)
      {
        uint64_t magic{0};
        magicLoad >> magic;
        magicMap[i].first.magic = magic;
        magicLoad.ignore(4); // 'ull,' / 'ull}'
      }
      magicLoad.ignore(2); // '\n{'
      for (int i{0}; i < 64; i++)
      {
        unsigned int shift{0};
        magicLoad >> shift;
        magicMap[i].first.shift = shift;
        magicLoad.ignore(2); // 'u,' / 'u}'
      }
      magicLoad.ignore(2); // '\n{'
      for (int i{0}; i < 64; i++)
      {
        uint64_t index{0};
        std::map<uint64_t, uint64_t> map{};
        while (!magicLoad.eof() && magicLoad.peek() != '}')
        {
          magicLoad.ignore(1); // '{' / ','
          uint64_t bitboard{0};
          magicLoad >> bitboard;
          if (bitboard > 0ull)
          {
            map[index] = bitboard;
          }
          magicLoad.ignore(3); // 'ull'
          index++;
        }
        magicMap[i].first.moveMap = map;
        magicLoad.ignore(3); // '},\n'
      }
      magicLoad.ignore(2); // '\n{'

      for (int i{0}; i < 64; i++)
      {
        uint64_t magic{0};
        magicLoad >> magic;
        magicMap[i].second.magic = magic;
        magicLoad.ignore(4); // 'ull,' / 'ull}'
      }
      magicLoad.ignore(2); // '\n{'
      for (int i{0}; i < 64; i++)
      {
        unsigned int shift{0};
        magicLoad >> shift;
        magicMap[i].second.shift = shift;
        magicLoad.ignore(2); // 'u,' / 'u}'
      }
      magicLoad.ignore(2); // '\n{'
      for (int i{0}; i < 64; i++)
      {
        uint64_t index{0};
        std::map<uint64_t, uint64_t> map{};
        while (!magicLoad.eof() && magicLoad.peek() != '}')
        {
          magicLoad.ignore(1); // '{' / ','
          uint64_t bitboard{0};
          magicLoad >> bitboard;
          if (bitboard > 0ull)
          {
            map[index] = bitboard;
          }
          magicLoad.ignore(3); // 'ull'
          index++;
        }
        magicMap[i].second.moveMap = map;
        magicLoad.ignore(3); // '},\n'
      }
      magicLoad.ignore(2); // '\n{'
    }
  }

  if (!doOrthogonal && !doDiagonal)
    doOrthogonal = true, doDiagonal = true;

  std::setfill("0");
  std::vector<std::pair<std::shared_ptr<MagicSearch>, std::shared_ptr<MagicSearch>>> magicSearches{};
  size_t orthBaselineLength{0};
  size_t diagBaselineLength{0};

  // spawn magic searching threads
  for (int i{0}; i < 64; i++)
  {
    auto &[orthMap, diagMap] = magicMap[i];

    int orthPossibilityBits = std::popcount(orthOccupancyMasks[i]);
    auto orthSearch = std::make_shared<MagicSearch>(doOrthogonal, orthOccupancySets[i], orthMap.magic, orthMap.shift);
    orthBaselineLength += (1ul << orthPossibilityBits);

    int diagPossibilityBits = std::popcount(diagOccupancyMasks[i]);
    auto diagSearch = std::make_shared<MagicSearch>(doDiagonal, diagOccupancySets[i], diagMap.magic, diagMap.shift);
    diagBaselineLength += (1ul << diagPossibilityBits);

    magicSearches.emplace_back(orthSearch, diagSearch);
  }

  static const size_t baseSize = 612 + 64 + 256;
  const size_t baselineOrthSize{orthBaselineLength * 8 + baseSize};
  const size_t baselineDiagSize{diagBaselineLength * 8 + baseSize};

  if (!gui)
  {
    int maxX, maxY;

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
    for (int i{0}; i < 64; i++)
    {
      auto &[orthMap, diagMap] = magicMap[i];

      if (orthMap.moveMap.size() == 0)
        mvwprintw(orthList, i, 1, "%2d 0x---------------- >> -- (not found)", i);
      else
      {
        uint32_t maxIndex = orthMap.moveMap.rbegin()->first;
        size_t arraySize = 8 + 1 + 8 + (maxIndex + 1) * 8;
        mvwprintw(orthList, i, 1, "%2d 0x%016llX >> %02d (%d c, %lu m, %.02fkb)", i, orthMap.magic, orthMap.shift, orthMap.collisions, maxIndex, arraySize / 1000.0f);
      }

      if (diagMap.moveMap.size() == 0)
        mvwprintw(diagList, i, 1, "%2d 0x---------------- >> -- (not found)", i);
      else
      {
        uint32_t maxIndex = diagMap.moveMap.rbegin()->first;
        size_t arraySize = 8 + 1 + 8 + (maxIndex + 1) * 8;
        mvwprintw(diagList, i, 1, "%2d 0x%016llX >> %02d (%d c, %lu m, %.02fkb)", i, diagMap.magic, diagMap.shift, diagMap.collisions, maxIndex, arraySize / 1000.0f);
      }
    }
    wrefresh(orthList);
    wrefresh(diagList);

    bool update{true};
    while (!terminate)
    {
      switch (getch())
      {
      case 'q':
        terminate = true;
        break;
      case 's':
        saveToFile(outputFilename, magicMap);
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

      if (update)
      {
        int notFoundOrth{0};
        int notFoundDiag{0};
        // draw all orthogonal and diagonal magics
        size_t orthSize{0};
        size_t diagSize{0};
        for (auto &[orth, diag] : magicMap)
        {
          if (orth.moveMap.size() == 0)
            notFoundOrth++;
          else
            orthSize += (orth.moveMap.rbegin()->first + 1);
          if (diag.moveMap.size() == 0)
            notFoundDiag++;
          else
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
        wprintw(totalSize, "/%.3f KB (%d/64)", baselineOrthSize / 1000.0f, 64 - notFoundOrth);
        wclrtoeol(totalSize);

        mvwprintw(totalSize, 1, 1, "Diagonal: ");
        if (baselineDiagSize >= diagSize)
          wattron(totalSize, COLOR_PAIR(1));
        wattron(totalSize, A_BOLD);
        wprintw(totalSize, "%.3f KB", diagSize / 1000.0f);
        wattroff(totalSize, COLOR_PAIR(1) | A_BOLD);
        wprintw(totalSize, "/%.3f KB (%d/64)", baselineDiagSize / 1000.0f, 64 - notFoundDiag);
        wclrtoeol(totalSize);

        wrefresh(totalSize);
        wrefresh(orthList);
        wrefresh(diagList);

        update = false;
      }
    }

    endwin();
    std::cout << "Stopping magic searches...\n";

    // save last bits of information
    for (int i{0}; i < 64; i++)
    {
      auto &[orth, diag] = magicSearches[i];
      auto &[orthMap, diagMap] = magicMap[i];

      orth->stop();
      if (orth->newMagicFound)
      {
        orthMap.magic = orth->bestMagic;
        orthMap.shift = orth->bestShift;
        orthMap.moveMap = orth->bestMoveMap;
      }

      diag->stop();
      if (diag->newMagicFound)
      {
        diagMap.magic = diag->bestMagic;
        diagMap.shift = diag->bestShift;
        diagMap.moveMap = diag->bestMoveMap;
      }
    }

    std::cout << "Stopped, saving magics to file '" << outputFilename << "'...\n";
    saveToFile(outputFilename, magicMap);
  }
  else
  {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    while (!terminate)
    {
      bool newMagicFound{false};
      for (int i{0}; i < 64; i++)
      {
        auto &[orth, diag] = magicSearches[i];
        if (orth->newMagicFound)
        {
          auto &orthMap = magicMap[i].first;
          newMagicFound = true;
          std::lock_guard lock(orth->valueMutex);
          orthMap.magic = orth->bestMagic;
          orthMap.shift = orth->bestShift;
          orthMap.moveMap = orth->bestMoveMap;
          orth->newMagicFound = false;
        }
        if (diag->newMagicFound)
        {
          auto &diagMap = magicMap[i].second;
          newMagicFound = true;
          std::lock_guard lock(diag->valueMutex);
          diagMap.magic = diag->bestMagic;
          diagMap.shift = diag->bestShift;
          diagMap.moveMap = diag->bestMoveMap;
          diag->newMagicFound = false;
        }
      }
      if (newMagicFound)
      {
        size_t orthSize{baseSize};
        size_t diagSize{baseSize};
        for (auto &[orth, diag] : magicMap)
        {
          if (orth.moveMap.size() > 0)
            orthSize += (orth.moveMap.rbegin()->first + 1) * 8;
          if (diag.moveMap.size() > 0)
            diagSize += (diag.moveMap.rbegin()->first + 1) * 8;
        }

        std::cout << std::format("New magic found: orth {:.3f} KB, diag {:.3f} KB\n", orthSize / 1000.0f, diagSize / 1000.0f);
        newMagicFound = false;
      }
    }
    std::cout << "Stopping magic searches...\n";

    for (int i{0}; i < 64; i++)
    {
      auto &[orth, diag] = magicSearches[i];
      auto &[orthMap, diagMap] = magicMap[i];

      orth->stop();
      if (orth->newMagicFound)
      {
        orthMap.magic = orth->bestMagic;
        orthMap.shift = orth->bestShift;
        orthMap.moveMap = orth->bestMoveMap;
      }

      diag->stop();
      if (diag->newMagicFound)
      {
        diagMap.magic = diag->bestMagic;
        diagMap.shift = diag->bestShift;
        diagMap.moveMap = diag->bestMoveMap;
      }
    }
    std::cout << "Stopped, saving magics to file '" << outputFilename << "'...\n";
    saveToFile(outputFilename, magicMap);
  }
}
