#include <unity.h>

void setUp() {}
void tearDown() {}

void test_firmware_compiles() {
  TEST_ASSERT_TRUE(true);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_firmware_compiles);
  UNITY_END();
}
