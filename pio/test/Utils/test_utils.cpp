#include "unity.h"
#include "utils.h"

void test_avg_1(void) {
  WeightedAverage<float, 7> wavg(1.1f, 100.0f);
  TEST_ASSERT_EQUAL_FLOAT(1.1, wavg.Compute());
}

void test_avg_2(void) {
  WeightedAverage<float, 10> wavg(1.0f, 1.0f);
  float a = 0.0f;
  for (size_t i=0; i<wavg.Size(); i++) {
    a = wavg.AddSample(2.0f, 2.0f);
  }
  TEST_ASSERT_EQUAL_FLOAT(2.0, a);
  for (size_t i=0; i<wavg.Size(); i++) {
    a = wavg.AddSample(4.0f, 1.0f);
  }
  TEST_ASSERT_EQUAL_FLOAT(4.0, a);
}

void test_avg_3(void) {
  WeightedAverage<float, 3> wavg(1.0f, 10.0f);
  wavg.AddSample(10.0f, 1.0f);
  wavg.AddSample(20.0f, 2.0f);
  wavg.AddSample(10.0f, 1.0f);
  TEST_ASSERT_EQUAL_FLOAT(15.0f, wavg.Compute());
}

void test_clamp() {
  TEST_ASSERT_EQUAL_FLOAT(2.0, clamp(2.0, 1.0, 3.0));
  TEST_ASSERT_EQUAL_FLOAT(-2.0, clamp(-2.0, -3.0, -1.0));
  TEST_ASSERT_EQUAL_FLOAT(3.0, clamp(4.0, 1.0, 3.0));
  TEST_ASSERT_EQUAL_FLOAT(-3.0, clamp(-4.0, -3.0, -1.0));
}

#if defined(ARDUINO)
#include <Arduino.h>
void setup() {
#else
int main(int argc, char **argv) {
#endif
  UNITY_BEGIN();
  RUN_TEST(test_avg_1);
  RUN_TEST(test_avg_2);
  RUN_TEST(test_avg_3);
  RUN_TEST(test_clamp);
  UNITY_END();
}

#if defined(ARDUINO)
void loop() {}
#endif
