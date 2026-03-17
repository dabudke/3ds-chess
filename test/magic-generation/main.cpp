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
  int maxIndex{0};
};
int main(int, char **)
{
  std::setfill("0");
  std::vector<std::pair<std::shared_ptr<MagicSearch>, std::shared_ptr<MagicSearch>>> magicSearches{};
  std::array<std::pair<MagicMapEntry, MagicMapEntry>, 64> magicMap{};
  unsigned long orthSize{0};
  unsigned long diagSize{0};

  // orthogonal magics
  {
    std::ifstream previousOutput("magic-output.csv");
    for (int i{0}; i < 64; i++)
    {
      auto orthSearch = std::make_shared<MagicSearch>(orthOccupancySets[i], 64 - std::popcount(orthOccupancyMasks[i]));
      orthSize += 1ul << std::popcount(orthOccupancyMasks[i]);
      auto diagSearch = std::make_shared<MagicSearch>(diagOccupancySets[i], 64 - std::popcount(diagOccupancyMasks[i]));
      diagSize += 1ul << std::popcount(diagOccupancyMasks[i]);
      magicSearches.emplace_back(orthSearch, diagSearch);
    }
  }

  static const size_t baseSize = 612 + 64 + 256;
  const unsigned long minOrthSize{orthSize + baseSize};
  const unsigned long minDiagSize{diagSize + baseSize};

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

  WINDOW *totalSizeBox = newwin(4, maxX - 54, 0, 0),
         *recentMagicsBox = newwin(maxY - 4, maxX - 54, 4, 0),
         *allMagicsBox = newwin(maxY, 54, 0, maxX - 54);

  WINDOW *totalSize = derwin(totalSizeBox, 2, maxX - 56, 1, 1),
         *recentMagics = derwin(recentMagicsBox, maxY - 6, maxX - 56, 1, 1),
         *allMagics = derwin(allMagicsBox, maxY - 2, 52, 1, 1);

  if (!totalSizeBox || !recentMagicsBox || !allMagicsBox || !totalSize || !recentMagics || !allMagics)
  {
    endwin();
    throw std::runtime_error("window error");
  }

  box(totalSizeBox, 0, 0);
  box(recentMagicsBox, 0, 0);
  box(allMagicsBox, 0, 0);

  mvwprintw(totalSizeBox, 0, 3, "Total Size");
  mvwprintw(recentMagicsBox, 0, 3, "Recent Magics");
  mvwprintw(allMagicsBox, 0, 3, "All Magics");

  scrollok(recentMagics, true);

  wrefresh(totalSizeBox);
  wrefresh(recentMagicsBox);
  wrefresh(allMagicsBox);

  // setup all magics table
  mvwprintw(allMagics, 0, 1, "sq | orthogonal            | diagonal");
  mvwprintw(allMagics, 1, 1, "---+-----------------------+----------------------");
  wrefresh(allMagics);

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
        std::lock_guard lock(orth->valueMutex);
        magicMap[i].first.magic = orth->bestMagic;
        magicMap[i].first.shift = orth->bestShift;
        magicMap[i].first.maxIndex = orth->bestMaxIndex;
        wprintw(recentMagics, "\north %02d: 0x%016llX << %02d", i, orth->bestMagic, orth->bestShift);
        if (orth->bestShift >= orth->bestPossibleShift)
        {
          magicMap[i].first.perfect = true;
          wattron(recentMagics, COLOR_PAIR(1));
          wprintw(recentMagics, " (PERFECT!)");
          wattroff(recentMagics, COLOR_PAIR(1));
        }
        else if (orth->bestShift > orth->bestMagic)
        {
          magicMap[i].first.perfect = true;
          wattron(recentMagics, COLOR_PAIR(2));
          wprintw(recentMagics, " (BETTER THAN PERFECT!)");
          wattroff(recentMagics, COLOR_PAIR(2));
        }
        orth->newMagicFound = false;
        update = true;
      }
      if (diag->newMagicFound)
      {
        std::lock_guard lock(diag->valueMutex);
        magicMap[i].second.magic = diag->bestMagic;
        magicMap[i].second.shift = diag->bestShift;
        magicMap[i].second.maxIndex = diag->bestMaxIndex;
        wprintw(recentMagics, "\ndiag %02d: 0x%016llX << %02d", i, diag->bestMagic, diag->bestShift);
        if (diag->bestShift == diag->bestPossibleShift)
        {
          magicMap[i].second.perfect = true;
          wattron(recentMagics, COLOR_PAIR(1));
          wprintw(recentMagics, " (PERFECT!)");
          wattroff(recentMagics, COLOR_PAIR(1));
        }
        else if (diag->bestShift > diag->bestMagic)
        {
          magicMap[i].second.perfect = true;
          wattron(recentMagics, COLOR_PAIR(2));
          wprintw(recentMagics, " (BETTER THAN PERFECT!)");
          wattroff(recentMagics, COLOR_PAIR(2));
        }
        diag->newMagicFound = false;
        update = true;
      }
    }

    // why is this not firing.
    if (update)
    {

      // draw all orthogonal and diagonal magics
      size_t orthSize{baseSize};
      size_t diagSize{baseSize};
      for (int i{0}; i < 64; i++)
      {
        auto &[orth, diag] = magicMap[i];
        mvwprintw(allMagics, i + 2, 0, " %2hhu | ", i);
        if (orth.perfect)
          wattron(allMagics, COLOR_PAIR(1));
        wprintw(allMagics, "0x%016llX %2i", orth.magic, orth.shift);
        if (orth.perfect)
          wattroff(allMagics, COLOR_PAIR(1));
        orthSize += (orth.maxIndex + 1) * 8;
        wprintw(allMagics, " | ");

        if (diag.perfect)
          wattron(allMagics, COLOR_PAIR(1));
        wprintw(allMagics, "0x%016llX %2i", diag.magic, diag.shift);
        if (diag.perfect)
          wattroff(allMagics, COLOR_PAIR(1));
        diagSize += diag.maxIndex + 1;
      }

      // calculate total sizes, best sizes
      wclear(totalSize);
      mvwprintw(totalSize, 0, 1, "Orthogonal: ");
      if (minOrthSize >= orthSize)
        wattron(totalSize, COLOR_PAIR(1));
      wattron(totalSize, A_BOLD);
      wprintw(totalSize, "%.3f", orthSize / 1000.0f);
      wattroff(totalSize, COLOR_PAIR(1) | A_BOLD);
      wprintw(totalSize, "/%.3f KB", minOrthSize / 1000.0f);

      mvwprintw(totalSize, 1, 1, "Diagonal: ");
      if (minDiagSize >= diagSize)
        wattron(totalSize, COLOR_PAIR(1));
      wattron(totalSize, A_BOLD);
      wprintw(totalSize, "%.3f", diagSize / 1000.0f);
      wattroff(totalSize, COLOR_PAIR(1) | A_BOLD);
      wprintw(totalSize, "/%.3f KB", minDiagSize / 1000.0f);

      touchwin(recentMagicsBox);
      touchwin(totalSizeBox);
      touchwin(allMagicsBox);
      wrefresh(recentMagics);
      wrefresh(totalSize);
      wrefresh(allMagics);

      refresh();
      update = false;
    }
  }

  endwin();

  for (auto &[orth, diag] : magicSearches)
  {
    orth->stop();
    std::cout << "0x" << std::hex << orth->bestMagic << " " << std::dec << orth->bestShift << " " << orth->bestMaxIndex << std::endl;
    diag->stop();
    std::cout << "0x" << std::hex << diag->bestMagic << " " << std::dec << diag->bestShift << " " << diag->bestMaxIndex << std::endl;
  }

  // std::remove("magic-output.csv");
  // std::ofstream output("magic-output.csv");

  std::array<std::map<uint8_t, uint64_t>, 64> orthMagicMap;
  std::array<std::map<uint8_t, uint64_t>, 64> diagMagicMap;

  for (int i{0}; i < 64; i++)
  {
    auto &[orth, diag] = magicSearches[i];

    std::string orthOutputPath = std::format("magics/magic-o{}.txt", i);
    std::remove(orthOutputPath.c_str());
    std::ofstream orthOutput(orthOutputPath);
    orthOutput << "magic: " << orth->bestMagic << "\nshift: " << orth->bestShift << "\nbest shift: " << orth->bestPossibleShift << "\ntheoretical max index: " << (1ull << (64 - orth->bestPossibleShift)) << "\n";
    for (auto &[index, moveset] : orth->bestMoveMap)
    {
      orthOutput << index << ";" << moveset << "\n";
    }
    orthOutput << "\n";

    std::string diagOutputPath = std::format("magics/magic-d{}.txt", i);
    std::remove(diagOutputPath.c_str());

    std::ofstream diagOutput(diagOutputPath);
    diagOutput << "magic: " << diag->bestMagic << "\nshift: " << diag->bestShift << "\nbest shift: " << diag->bestPossibleShift << "\ntheoretical max index: " << (1ull << (64 - diag->bestPossibleShift)) << "\n";
    for (auto &[index, moveset] : diag->bestMoveMap)
    {
      diagOutput << index << ";" << moveset << "\n";
    }
  }
}
