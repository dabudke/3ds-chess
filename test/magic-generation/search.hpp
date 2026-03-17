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
  const int bestPossibleShift;
  std::atomic_bool newMagicFound{false};
  std::mutex valueMutex;
  int bestShift{0};
  uint64_t bestMagic{0};
  uint64_t bestMaxIndex{0};
  std::map<uint64_t, uint64_t> bestMoveMap{};

  void stop()
  {
    shouldStop = true;
    if (thisThread.joinable())
      thisThread.join();
  }

  MagicSearch(const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets, int initialShift) : bestPossibleShift(initialShift), occupancySets{occupancySets}, thisThread(&MagicSearch::entrypoint, this) {}
  ~MagicSearch()
  {
    stop();
  }
};
