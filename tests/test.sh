#!/bin/bash

cd "$(dirname "$0")" || exit 1
source "../scripts/utils.sh" || exit 1

CC="${CC:-gcc}"
CCFLAGS=( -Wall -Werror -O2 )

if [ -f "../compile_flags.txt" ]; then
    readFlags=$(tr '\n' ' ' < ../compile_flags.txt)
    if [ -n "$readFlags" ]; then
        read -ra CCFLAGS <<< "$readFlags"
    else
        CCFLAGS=( -Wall -Werror -O2 )
    fi
fi

includePath=(
    "../src/include"
    "/Unity/"
    "."
)

SuccessExit=0
CompilationErrorExit=1
LinkingErrorExit=2
EnvironmentErrorExit=3
InvalidFlagExit=4
ErrorExit=5

OUTDIR="${OUTDIR:-build}"
CLINES_OBJECTS=()

mkdir -p "$OUTDIR"
verbose=false

tests=()

Clean() {
    if [[ -d "$OUTDIR" ]]; then
        ShowProgress "Cleaning directory: $OUTDIR"
        rm -rf "$OUTDIR"
    fi

    mkdir -p "$OUTDIR"
    return $SuccessExit
}

Help() {
    echo "Usage: script [options] [targets]"
    echo
    echo "Options:"
    echo "  --echo, --verbose, -v       Show verbose output"
    echo "  --clean                     Clean build directory"
    echo "  --cc=<compiler>             Set C compiler path"
    echo "  --ccflags=<flags>           Set C compilation flags"
    echo "  --outdir=<directory>        Set output directory"
    echo "  --help                      Show this help message"
    echo
    echo "Targets:"
    echo "  -Test{NAME}                 Run specific test"
    echo "  -TestAll                    Run all available tests"
    echo
    echo "If no targets specified, all tests will be run."

    return $SuccessExit
}

ParseFlags() {
    for arg in "$@"; do
        case "$arg" in
        -TestAll)
            tests=("ALL") ;;
        -Test*)
            tests+=("${arg##-}") ;;

        --echo | --verbose | -v)
            verbose=true ;;
        --clean)
            Clean
            exit $?
            ;;
        --help | -h)
            Help
            exit $?
            ;;

        --cc=*)
            CC="${arg#*=}" ;;
        --ccflags=*)
            read -ra CCFLAGS <<< "${arg#*=}" ;;

        --outdir=*)
            OUTDIR="${arg#*=}" ;;

        *)
            ShowError $InvalidFlagExit "Invalid flag: $arg"
            ;;
        esac
    done
}

CheckEnvironment() {
    if ! command -v "$CC" >/dev/null 2>&1; then
        ShowError $EnvironmentErrorExit "The C compiler needed to compile the tests was not found. try specifying the correct path in the --cc=<compiler> flag or in the CC environment variable"
    fi
}

RunTarget() {
    local target="$1"
    chmod +x "$target"

    "$target"
    return $?
}

NeedsCompile() {
    local target="$1"
    local dependency="$2"
    [[ ! -f "$target" ]] || [[ "$dependency" -nt "$target" ]]
    return $?
}

NameObj() {
    echo "$OUTDIR/$1.o"
}

NameOut() {
    echo "$OUTDIR/$1"
    # echo "$OUTDIR/$1.elf"
    # echo "$OUTDIR/$1.out"
}

CompileTest() {
    local testFile="$1"
    local obj="$(NameObj "$testFile")"
    local out="$(NameOut "$testFile")"

    # Check if object file needs to be recompiled
    if NeedsCompile "$obj" "$testFile.c"; then
        "$CC" "${CCFLAGS[@]}" "$testFile.c" "${includePath[@]/#/-I}" -c -o "$obj" || ShowError $CompilationErrorExit "Test \"$testFile\" failed to compile."
    fi

    if NeedsCompile "$out" "$obj" || [[ "build/testing.o" -nt "$out" ]]; then
        "$CC" "${CCFLAGS[@]}" "${CLINES_OBJECTS[@]}" "$obj" "build/Unity.o" -o "$out" || ShowError $LinkingErrorExit "Test \"$testFile\" failed to link."
    fi
}

Test() {
    local testFile="$1"
    local obj="$(NameObj "$testFile")"
    local out="$(NameOut "$testFile")"

    echo -e "\033[34;1m" "Testing $testFile..." "\033[0m"
    CompileTest "$testFile"

    RunTarget "$out"
    local exitCode="$?"
    if [[ $exitCode -ne 0 ]]; then
        echo -e "\033[31;1m" "$testFile: fail." "\033[0m\033[31mExit code: $exitCode" "\033[0m"
        return 1
    else
        echo -e  "\033[32;1m" "$testFile: pass." "\033[0m"
        return 0
    fi
}

CompileLib() {
    if [[ ! -f "build/Unity.o" ]] || [[ "Unity/unity.c" -nt "build/Unity.o" ]]; then
        "$CC" "${CCFLAGS[@]}" "Unity/unity.c" "${includePath[@]/#/-I}" -c -o "build/Unity.o" || ShowError $CompilationErrorExit "Failed to compile unity.c."
    fi
}

CollectClinesObjects() {
    while IFS= read -r -d '' f; do
        if [[ $f == "../build/main.c.o" ]]; then
            continue
        fi
        CLINES_OBJECTS+=("$f")
    done < <(find ../build/ -name '*.o' -print0)
}

PrintAll() {
    local targets=("$@")

    local obj=""
    local out=""

    if [[ $verbose = true ]]; then
        for tg in "${targets[@]}"; do
            obj="$(NameObj "$tg")"
            out="$(NameOut "$tg")"
            if NeedsCompile "$obj" "$tg.c" && NeedsCompile "$out" "$obj"; then
                echo -en "${tg} "
            elif NeedsCompile "$obj" "$tg.c"; then
                echo -en "${BOLD}${YELLOW}${tg}${RESET} "
            else
                echo -en "${BOLD}${tg}${RESET} "
            fi
        done
    fi
}

Main() {
    ParseFlags "$@"

    CheckEnvironment
    CollectClinesObjects
    CompileLib

    totalTests="${#tests[@]}"
    if [[ "${tests[*]}" == "ALL" || $totalTests -eq 0 ]]; then
        tests=()
        while IFS= read -r file; do
            if [[ "$(dirname "$file")" == "." ]]; then
                tests+=("$(basename "$file" .c)")
            fi
        done < <(find . -maxdepth 1 -iname "Test*.c")
    fi

    totalTests="${#tests[@]}"
    if [[ $totalTests -gt 0 ]]; then
        passed=0
        failed=0

        ShowInfo "Running tests..."
        PrintAll "${tests[@]}"

        for test in "${tests[@]}"; do
            if Test "$test"; then
                (( passed++ ))
            else
                (( failed++ ))
            fi
        done

        if [[ $passed -eq $totalTests ]]; then
            ShowSuccess "All tests have passed!"
        elif [[ $failed -eq $totalTests ]]; then
            ShowWarn "All tests failed"
        else
            ShowWarn "Some tests failed (failed $failed/$totalTests)"
        fi
    fi

    return $SuccessExit
}

Main "$@"
exit $?
