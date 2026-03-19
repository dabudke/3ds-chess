#include <cinttypes>
#include <thread>
#include <map>

class MagicSearch
{
  std::thread thisThread;
  void entrypoint();
  const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets;
  std::atomic_bool shouldStop;

public:
  std::atomic_bool newMagicFound{false};
  std::mutex valueMutex;
  int bestShift;
  uint64_t bestMagic;
  int bestMapCollisions{0};
  std::map<uint64_t, uint64_t> bestMoveMap{};

  void stop()
  {
    shouldStop = true;
    if (thisThread.joinable())
      thisThread.join();
  }

  MagicSearch(bool start, const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets, uint64_t bestMagic, int bestShift) : shouldStop{!start}, occupancySets{occupancySets}, thisThread(&MagicSearch::entrypoint, this), bestMagic{bestMagic}, bestShift{bestShift} {}
  ~MagicSearch()
  {
    stop();
  }
};
