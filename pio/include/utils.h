#include <algorithm>
#include <stdint.h>

template<typename T> T clamp(T value, T min, T max) {
  return std::min(max, std::max(min, value));
}

template<typename T, size_t SIZE>
class WeightedAverage {
 public:
  WeightedAverage(T value, T weight) {
    for (size_t i = 0; i < SIZE; i++) {
      values_[i] = value;
      weights_[i] = weight;
    }
    index_ = 0;
  }

  T AddSample(T value, T weight) {
    index_++;
    if (index_ >= SIZE) {
      index_ = 0;
    }
    values_[index_] = value;
    weights_[index_] = weight;
    return Compute();
  }

  T Compute() {
    // For simplification, and avoid loss of accuracy, we simply go over the
    // all the elements rather than adding the newest and substracting the oldest
    T wsum(0);
    T total(0);
    for (size_t i = 0; i < SIZE; i++) {
      wsum += values_[i]*weights_[i];
      total += weights_[i];
    }
    return wsum / total;
  }

  size_t Size() {
    return SIZE;
  }

 protected:
  size_t index_;

 private:
  WeightedAverage();
  WeightedAverage(WeightedAverage&);
  T values_[SIZE];
  T weights_[SIZE];
};
