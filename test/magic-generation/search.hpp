#include <cinttypes>
#include <thread>
#include <map>

class MagicSearch
{
  std::thread thisThread;
  void entrypoint();
  const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets;
  std::atomic_bool shouldStop{false};

public:
  std::atomic_bool newMagicFound{false};
  std::mutex valueMutex;
  int bestShift{0};
  uint64_t bestMagic{0};
  int bestMapCollisions{0};
  std::map<uint64_t, uint64_t> bestMoveMap{};

  void stop()
  {
    shouldStop = true;
    if (thisThread.joinable())
      thisThread.join();
  }

  MagicSearch(const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets, int initialShift) : occupancySets{occupancySets}, thisThread(&MagicSearch::entrypoint, this) {}
  ~MagicSearch()
  {
    stop();
  }
};
