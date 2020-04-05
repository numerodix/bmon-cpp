#ifndef BAR_CHART_H
#define BAR_CHART_H

#include <cstdint>
#include <vector>

#include "display.h"

namespace bmon {
namespace termui {

class BarChart {
  public:
    BarChart(Display *display) : display_{display} {}
    void draw_bars_from_right(std::vector<uint64_t> values);

  private:
    Display *display_{nullptr};
};

} // namespace termui
} // namespace bmon

#endif // BAR_CHART_H
