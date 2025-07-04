#include "Unity/unity_internals.h"
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

void TestCopy() {
    StringList l1;
    SL_Init(&l1);

    SL_Append(&l1, "Hello");
    SL_Append(&l1, "Good morning");
    SL_Append(&l1, "Hi");

    StringList l2;
    SL_Copy(&l2, &l1);

    TEST_ASSERT( SL_Eq(&l1, &l2) );
    TEST_ASSERT( SL_EqTo(&l2, (const char*[]) { "Hello", "Good morning", "Hi", NULL }) );

    SL_Destroy(&l1);
    SL_Destroy(&l2);
}

void TestMove() {
    StringList l1;
    SL_Init(&l1);

    SL_Append(&l1, "1");
    SL_Append(&l1, "2");
    SL_Append(&l1, "3");
    SL_Append(&l1, "4");

    StringList l2;
    SL_Move(&l2, &l1);

    TEST_ASSERT( SL_EqTo(&l2, (const char*[]) { "1", "2", "3", "4", NULL }) );

    SL_Destroy(&l2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(TestAppend);
    RUN_TEST(TestCopy);
    RUN_TEST(TestMove);
    return UNITY_END();
}
