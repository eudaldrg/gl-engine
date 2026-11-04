// Frame times over a rolling window, for the stats panel.
#pragma once

#include <chrono>
#include <cstddef>
#include <deque>

namespace gl_engine::viewer {

class FrameStats {
  public:
    using Duration = std::chrono::duration<double, std::milli>;

    explicit FrameStats(std::size_t window = kDefaultWindow) : window_{window} {}

    void Add(Duration frame_time);

    // Over the last `window` frames; zero before the first frame.
    Duration Average() const;
    Duration Max() const;

    static constexpr std::size_t kDefaultWindow = 120;

  private:
    std::size_t window_;
    std::deque<Duration> frames_;
    Duration sum_{0};
};

}  // namespace gl_engine::viewer
