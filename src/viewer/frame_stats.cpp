#include "gl_engine/viewer/frame_stats.h"

#include <algorithm>

namespace gl_engine::viewer {

void FrameStats::Add(Duration frame_time) {
    frames_.push_back(frame_time);
    sum_ += frame_time;
    if (frames_.size() > window_) {
        sum_ -= frames_.front();
        frames_.pop_front();
    }
}

FrameStats::Duration FrameStats::Average() const {
    if (frames_.empty()) {
        return Duration{0};
    }
    return sum_ / static_cast<double>(frames_.size());
}

FrameStats::Duration FrameStats::Max() const {
    if (frames_.empty()) {
        return Duration{0};
    }
    return *std::ranges::max_element(frames_);
}

}  // namespace gl_engine::viewer
