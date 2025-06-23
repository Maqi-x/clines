#include <Unity/unity.h>

#include <StringList.h>

void setUp() {}
void tearDown() {}

void TestAppend() {
    StringList list;
    SL_Init(&list);

    SL_Append(&list, "Hello");
    SL_Append(&list, "Good morning");
    SL_Append(&list, "Hi");

    TEST_ASSERT( SL_EqTo(&list, (const char*[]) { "Hello", "Good morning", "Hi", NULL }) );

    SL_Clear(&list);
    SL_Append(&list, "1");
    SL_Append(&list, "2");
    SL_Append(&list, "3");
    SL_Append(&list, "4");

    TEST_ASSERT( SL_EqTo(&list, (const char*[]) {"1", "2", "3", "4", NULL}) );

    SL_Destroy(&list);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(TestAppend);
    return UNITY_END();
}
