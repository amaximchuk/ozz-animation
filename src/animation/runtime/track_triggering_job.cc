//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2017 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/animation/runtime/track_triggering_job.h"
#include "ozz/animation/runtime/track.h"

#include <algorithm>
#include <cassert>

namespace ozz {
namespace animation {

FloatTrackTriggeringJob::FloatTrackTriggeringJob()
    : from(0.f), to(0.f), threshold(0.f), track(NULL), edges(NULL) {}

bool FloatTrackTriggeringJob::Validate() const {
  bool valid = true;
  valid &= track != NULL;
  valid &= edges != NULL;
  return valid;
}

bool FloatTrackTriggeringJob::Run() const {
  if (!Validate()) {
    return false;
  }

  // Triggering can only happen in a valid range of time.
  if (from == to) {
    edges->Clear();
    return true;
  }

  Iterator iterator(this);

  // Adjust output length.
  Edge* edges_end = edges->begin;
  for (; iterator != end(); ++iterator, ++edges_end) {
    if (edges_end == edges->end) {
      return false;
    }
    *edges_end = *iterator;
  }
  edges->end = edges_end;

  return true;
}

namespace {
inline bool DetectEdge(ptrdiff_t _i0, ptrdiff_t _i1, bool _forward,
                       const FloatTrackTriggeringJob& _job,
                       FloatTrackTriggeringJob::Edge* _edge) {
  const Range<const float>& values = _job.track->values();

  const float vk0 = values[_i0];
  const float vk1 = values[_i1];

  bool detected = false;
  if (vk0 <= _job.threshold && vk1 > _job.threshold) {
    // Rising edge
    _edge->rising = _forward;
    detected = true;
  } else if (vk0 > _job.threshold && vk1 <= _job.threshold) {
    // Falling edge
    _edge->rising = !_forward;
    detected = true;
  }

  if (detected) {
    const Range<const float>& times = _job.track->times();
    const Range<const uint8_t>& steps = _job.track->steps();

    const bool step = (steps[_i0 / 8] & (1 << (_i0 & 7))) != 0;
    if (step) {
      _edge->time = times[_i1];
    } else {
      assert(vk0 != vk1);  // Won't divide by 0

      if (_i1 == 0) {
        _edge->time = 0.f;
      } else {
        // Finds where the curve crosses threshold value.
        // This is the lerp equation, where we know the result and look for
        // alpha, aka un-lerp.
        const float alpha = (_job.threshold - vk0) / (vk1 - vk0);

        // Remaps to keyframes actual times.
        const float tk0 = times[_i0];
        const float tk1 = times[_i1];
        _edge->time = math::Lerp(tk0, tk1, alpha);
      }
    }
  }
  return detected;
}
}  // namespace

FloatTrackTriggeringJob::Iterator::Iterator(const FloatTrackTriggeringJob* _job)
    : job_(_job) {
  outer_ = floorf(job_->from);
  if (job_->to > job_->from) {
    inner_ = 0;
  } else {
    outer_ += 1.f;
    inner_ = job_->track->times().Count() - 1;
  }
  ++*this;  // Evaluate first edge
}

void FloatTrackTriggeringJob::Iterator::operator++() {
  const Range<const float>& times = job_->track->times();
  const ptrdiff_t num_keys = times.Count();

  if (job_->to > job_->from) {
    for (; outer_ < job_->to; outer_ += 1.f) {
      for (; inner_ < num_keys; ++inner_) {
        // Find relevant keyframes value around i.
        const ptrdiff_t i0 = inner_ == 0 ? num_keys - 1 : inner_ - 1;
        if (DetectEdge(i0, inner_, true, *job_, &edge_)) {
          // Convert local loop time to global time space.
          edge_.time += outer_;
          // Pushes the new edge only if it's in the input range.
          if (edge_.time >= job_->from &&
              (edge_.time < job_->to || job_->to >= 1.f + outer_)) {
            // Yield found edge.
            ++inner_;
            return;
          }
          // Won't find any further edge.
          if (times[inner_] + outer_ >= job_->to) {
            break;
          }
        }
      }
      inner_ = 0;  // Ready for next loop.
    }
  } else {
    for (; outer_ > job_->to; outer_ -= 1.f) {
      for (; inner_ >= 0; --inner_) {
        // Find relevant keyframes value around i.
        const ptrdiff_t i0 = inner_ == 0 ? num_keys - 1 : inner_ - 1;
        if (DetectEdge(i0, inner_, false, *job_, &edge_)) {
          // Convert local loop time to global time space.
          edge_.time += outer_ - 1.f;
          // Pushes the new edge only if it's in the input range.
          if (edge_.time >= job_->to &&
              (edge_.time < job_->from || job_->from >= outer_)) {
            // Yield found edge.
            --inner_;
            return;
          }
        }
        // Won't find any further edge.
        if (times[inner_] + outer_ - 1.f <= job_->to) {
          break;
        }
      }
      inner_ = times.Count() - 1;  // Ready for next loop.
    }
  }

  // Set iterator to end position.
  outer_ = job_->to;  // End
  inner_ = 0;
}
}  // namespace animation
}  // namespace ozz
