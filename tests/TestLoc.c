#include <Unity/unity.h>

#include <LocSettings.h>
#include <LocUtils.h>
#include <LocParser.h>

#include <stdio.h>
#include <string.h>

#define DBG \
    fprintf(stderr, "Got: "); \
    LS_DebugBump(&result, stderr, NULL); \
    fprintf(stderr, "Expected: "); \
    LS_DebugBump(&expected, stderr, NULL); \
    fprintf(stderr, "Code: ```\n"); \
    puts(testCode); \
    fprintf(stderr, "```\n");

void setUp() {}
void tearDown() {}

void TestFindLocEntryFor() {
    const LocEntry* lang;

    GetLocLangFor("main.c", &lang);
    TEST_ASSERT_EQUAL_STRING("C", lang->langName);

    GetLocLangFor("someting.go", &lang);
    TEST_ASSERT_EQUAL_STRING("Go", lang->langName);

    GetLocLangFor("Makefile", &lang);
    TEST_ASSERT_EQUAL_STRING("Makefile", lang->langName);

    GetLocLangFor("script.sh", &lang);
    TEST_ASSERT_EQUAL_STRING("Shell Script", lang->langName);

    GetLocLangFor("unknown.xyz", &lang);
    TEST_ASSERT_NULL(lang);
}

void TestLocParserC() {
    const char* testCode =
        "#include <stdio.h>\n"
        "#define MAX 100\n"
        "#ifdef DEBUG\n"
        "#endif\n"
        "\n"
        "/* Multi-line\n"
        "   C style comment */\n"
        "\n"
        "// Single line comment\n"
        "\n"
        "int main() {\n"
        "   // This is a comment \\\n"
        "      And this is another comment\n"
        "   printf(\"Hello /* not a comment */ world!\\n\");\n"
        "   /* Inline comment */ int x = 5;\n"
        "   return 0;\n"
        "}\n"
        "\n";
    LocStat expected = {
        .codeLines = 5,
        .commentLines = 6,
        .emptyLines = 4,
        .preprocessorLines = 4,
        .totalLines = 18,
    };

    LocParser parser;
    LP_Init(&parser);

    LocStat result = {0};
    LP_ParseCode(&parser, LOC_LANG_C, testCode, &result);

    if (!LS_Eql(&result, &expected)) {
        DBG
    }
    TEST_ASSERT(LS_Eql(&result, &expected));
}

void TestLocParserGo() {
    const char* testCode =
        "package main\n"
        "\n"
        "import (\n"
        "    \"fmt\"\n"
        "    \"strings\" // imported package\n"
        ")\n"
        "\n"
        "/*\n"
        " * Multi-line\n"
        " * Go comment\n"
        " */\n"
        "\n"
        "// Function declaration\n"
        "func main() {\n"
        "    // String literal with // inside\n"
        "    str := \"Hello // world\"\n"
        "    /* Block comment */ fmt.Println(str)\n"
        "    \n"
        "    /* Another \n"
        "       multi-line comment */\n"
        "    fmt.Print(\"Done!\")\n"
        "    multiline := `This is a\n"
        "        multiline string with // comments\n"
        "        /* and block comments */\n"
        "        inside`\n"
        "}\n";
    LocStat expected = {
        .codeLines = 14,
        .commentLines = 10,
        .emptyLines = 4,
        .preprocessorLines = 0,
        .totalLines = 26,
    };

    LocParser parser;
    LP_Init(&parser);

    LocStat result = {0};
    LP_ParseCode(&parser, LOC_LANG_GO, testCode, &result);

    if (!LS_Eql(&result, &expected)) {
        DBG
    }
    TEST_ASSERT(LS_Eql(&result, &expected));
}

void TestLocParserShellScript() {
    const char* testCode =
        "#!/bin/sh\n"
        "\n"
        "# Configuration\n"
        "NAME=\"test\"\n"
        "\n"
        "# Multiple\n"
        "# Comments\n"
        "# In a row\n"
        "\n"
        "echo \"Hello #not a comment\" # This is a comment\n"
        "  # Indented comment\n"
        "echo \"World\"\n"
        "\n"
        "# Final comment";
    LocStat expected = {
        .codeLines = 3,
        .commentLines = 8,
        .emptyLines = 4,
        .preprocessorLines = 0,
        .totalLines = 14,
    };

    LocParser parser;
    LP_Init(&parser);

    LocStat result = {0};
    LP_ParseCode(&parser, LOC_LANG_SHELL, testCode, &result);

    if (!LS_Eql(&result, &expected)) {
        DBG
    }
    TEST_ASSERT(LS_Eql(&result, &expected));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(TestFindLocEntryFor);
    RUN_TEST(TestLocParserC);
    RUN_TEST(TestLocParserGo);
    RUN_TEST(TestLocParserShellScript);
    return UNITY_END();
}
