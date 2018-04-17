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

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_TRACK_TRIGGERING_JOB_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_TRACK_TRIGGERING_JOB_H_

#include "ozz/base/platform.h"

namespace ozz {
namespace animation {

class FloatTrack;

// Only FloatTrack is supported, because comparing and un-lerping other tracks
// doesn't make much sense.
struct FloatTrackTriggeringJob {
  FloatTrackTriggeringJob();

  bool Validate() const;

  bool Run() const;

  // Input range.
  float from;
  float to;

  // Edge detection threshold value.
  // A rising edge is detected as soon as the track value becomes greater than
  // the threshold.
  // A falling edge is detected as soon as the track value becomes smaller or
  // equal than the threshold.
  float threshold;

  // Track to sample.
  const FloatTrack* track;

  // Job output.
  struct Edge {
    float time;
    bool rising;
  };
  typedef Range<Edge> Edges;
  Edges* edges;

  class Iterator;

  Iterator end() const;
};

class FloatTrackTriggeringJob::Iterator {
 public:
  void operator++();
  Iterator operator++(int) {
    Iterator prev = *this;
    ++*this;
    return prev;
  }

  bool operator!=(const Iterator& _it) const {
    return inner_ != _it.inner_ || outer_ != _it.outer_ || job_ != _it.job_;
  }
  bool operator==(const Iterator& _it) const {
    return job_ == _it.job_ && outer_ == _it.outer_ && inner_ == _it.inner_;
  }

  const Edge& operator*() const {
    assert(*this != job_->end());
    return edge_;
  }

  const Edge* operator->() const {
    assert(*this != job_->end());
    return &edge_;
  }

 private:
  explicit Iterator(const FloatTrackTriggeringJob* _job);

  struct End {};
  Iterator(const FloatTrackTriggeringJob* _job, End) : job_(_job) {
    outer_ = _job->to;
    inner_ = 0;
  }

  friend struct FloatTrackTriggeringJob;

  // Job this iterator works on.
  const FloatTrackTriggeringJob* job_;

  // Current value of the outer loop, aka a time cursor between from and to.
  float outer_;

  // Current value of the inner loop, aka a key frame index.
  ptrdiff_t inner_;

  // Latest evaluated edge.
  Edge edge_;
};

inline FloatTrackTriggeringJob::Iterator FloatTrackTriggeringJob::end() const {
  return Iterator(this, Iterator::End());
}
}  // namespace animation
}  // namespace ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_TRACK_TRIGGERING_JOB_H_
