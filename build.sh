#!/bin/bash
cd "$(dirname "$0")" || exit
source "scripts/utils.sh"

# ---------------------- Default Variables ---------------------- #
CC="${CC:-gcc}"
CCFLAGS=( -Wall -Werror -std="c11" -O2 )

if [ -f "compile_flags.txt" ]; then
    readFlags=$(tr '\n' ' ' < compile_flags.txt)
    if [ -n "$readFlags" ]; then
        read -ra CCFLAGS <<< "$readFlags"
    else
        CCFLAGS=( -Wall -Werror -std="c11" -O2 )
    fi
fi

SuccessExit=0
CompilationErrorExit=1
LinkingErrorExit=2
ErrorExit=3

SRC="src"
BUILD="build"
OUTDIR="out"
OUT="clines"

run=false

# ---------------------- Parsing Arguments ---------------------- #
for arg in "$@"; do
	case $arg in
	--CC=*)
		CC="${arg#*=}"
		;;
	--CCFLAGS=*)
		read -ra CCFLAGS <<< "${arg#*=}"
		;;
	--OUT-FILE=*)
		OUT="${arg#*=}"
		;;
	--OUT-DIR=*)
		OUTDIR="${arg#*=}"
		;;
	--BUILD-DIR=*)
		BUILD="${arg#*=}"
		;;
	run | --run)
		run=true
		;;
	clean | --clean)
		[[ ! (-z "$BUILD" || -z "$OUTDIR") ]] && rm -rf "$BUILD" "$OUTDIR"
		exit $SuccessExit
		;;
	*)
		ShowError $ErrorExit "Unknown argument: $arg"
		;;
	esac
done

# ---------------------- Libs and includePath ---------------------- #
includePath=(
	"/usr/include"
	"$PWD/src/Include"
	"$PWD/src"
)

libs=(
	# nothing yet...
)

# ---------------------- Building Project ---------------------- #
mkdir -p "$BUILD" || ShowError $ErrorExit "Failed to create build directory"
mkdir -p "$OUTDIR" || ShowError $ErrorExit "Failed to create output directory"

SOURCES=( $(find "$SRC" -type f -name "*.c") )
TOBUILD=()
OBJECTS=()

total=0
compiled=0

ShowInfo "Building $OUT..."

# ---------------------- Compiling Sources ---------------------- #
for src in "${SOURCES[@]}"; do
	obj="$BUILD/$(realpath --relative-to="$SRC" "$src" | tr '/' '_').o"
	if [[ ! -f "$obj" || "$(stat -c %Y "$src")" -gt "$(stat -c %Y "$obj")" ]]; then
		TOBUILD+=("$src")
		((total++))
	fi
	OBJECTS+=("$obj")
done

if [[ ${#TOBUILD[@]} -gt 0 ]]; then
	ShowInfo "Compiling sources..."
	for src in "${TOBUILD[@]}"; do
		obj="$BUILD/$(realpath --relative-to="$SRC" "$src" | tr '/' '_').o"

		$CC "${CCFLAGS[@]}" "${includePath[@]/#/-I}" -c "$src" -o "$obj" || ShowError $CompilationErrorExit "Compilation failed for $src"
		((compiled++))
		printf "\033[1;33m[%3d%% ]\033[93m %s \033[0m-> \033[93;1m%s\n" "$((compiled * 100 / total))" "$src" "$(basename "${src%.c}.o")"
	done
fi

# ------------------------ Linking Objects ------------------------ #
ShowInfo "Linking objects..."

$CC "${CCFLAGS[@]}" "${OBJECTS[@]}" "${libs[@]}" -o "$OUTDIR/$OUT" || ShowError $LinkingErrorExit "Linking failed."

# ------------------------ Finishing Build ------------------------ #
chmod +x "$OUTDIR/$OUT"
ShowSuccess "Build completed: $OUT"

if [[ $run = true ]]; then
	"$OUTDIR/$OUT" || ShowError $? "An error occurred while running vish. exit code: $?"
fi

exit $SuccessExit
