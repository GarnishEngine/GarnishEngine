#include <limits>
#include <memory>

#include "garnish.hpp"

const int32_t FRAME_RATE = 90;

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
using std::chrono::duration_cast;
using namespace garnish;
static float yaw = 0.0f;
static float pitch = 0;
static float x = 0;
static float y = 0;
