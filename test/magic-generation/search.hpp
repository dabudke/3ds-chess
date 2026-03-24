#include <cinttypes>
#include <vector>
#include <thread>
#include <map>
#include <atomic>
#include <mutex>
#include <functional>

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

  MagicSearch(bool start, const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets, uint64_t bestMagic, int bestShift) : shouldStop{!start}, occupancySets{occupancySets}, bestMagic{bestMagic}, bestShift{bestShift}
  {
    thisThread = std::thread(&MagicSearch::entrypoint, this);
  }
  ~MagicSearch()
  {
    stop();
  }
};
