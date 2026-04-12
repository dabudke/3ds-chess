#include <array>
#include <cstdint>
#include <utility>
#include <vector>

extern const std::array<uint64_t, 64> orthOccupancyMasks;
extern const std::array<uint64_t, 64> diagOccupancyMasks;

extern const std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> orthOccupancySets;
extern const std::array<std::vector<std::pair<uint64_t, uint64_t>>, 64> diagOccupancySets;
