#include <cinttypes>
#include <thread>
#include <map>

class MagicSearch
{
  std::thread thisThread;
  void entrypoint();
  const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets;
  std::atomic_bool shouldStop{false};
  const uint64_t initialShift;

public:
  std::atomic_bool newMagicFound{false};
  std::mutex valueLock;
  uint8_t bestShift{0};
  uint64_t bestMagic{0};
  std::map<uint64_t, uint64_t> bestMoveMap{};

  void stop()
  {
    shouldStop = true;
    if (thisThread.joinable())
      thisThread.join();
  }

  MagicSearch(const std::vector<std::pair<uint64_t, uint64_t>> &occupancySets, uint64_t initialShift) : initialShift(initialShift), occupancySets{occupancySets}, thisThread(&MagicSearch::entrypoint, this) {}
  ~MagicSearch()
  {
    stop();
  }
};
