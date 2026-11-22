#!/bin/bash

# verify-build.sh - Verify INSTALLED plugin (not build artifacts)
#
# IMPORTANT: Run this on the INSTALLED plugin, not the build directory.
# The installed plugin is the truth. Build artifacts can lie.
#
# This script mechanizes the stale-build triage checklist in
# vibecoding/plugin.md. Exits nonzero if any blocker is found.
#
# Customize: Set the variables at the top (or pass as env vars).

# NOTE: no `set -e` here on purpose. We count errors and report all of
# them at once. (Also, with set -e, `((ERRORS++))` returns nonzero when
# ERRORS is 0 and silently kills the script on the first failed check.)

# ============================================
# CUSTOMIZE THESE FOR YOUR PLUGIN
# ============================================
PLUGIN_NAME="${PLUGIN_NAME:-KingDubby}"
VERSION="${VERSION:-1.0.2}"
EXPECTED_MINOS="${EXPECTED_MINOS:-11.0}"

# String that must appear inside the binary to prove THIS build is
# installed (e.g. a version label compiled into the UI). Leave empty
# to skip the check.
VERSION_STRING="${VERSION_STRING:-KingDubby v$VERSION}"

# Source directory for the staleness check (binary must be newer than
# every .cpp/.h under here). Leave empty to skip.
SOURCE_DIR="${SOURCE_DIR:-$(cd "$(dirname "$0")/.." && pwd)/Source}"

# ============================================
# RELEASE IDENTITY (update each release)
# ============================================
# Version:        $VERSION
# Bundle ID:      com.scalenavigator.$PLUGIN_NAME
# Plugin Name:    $PLUGIN_NAME
# Deployment:     macOS $EXPECTED_MINOS+
# Architectures:  arm64 + x86_64

# ============================================
# Don't modify below unless necessary
# ============================================

VST3_PATH=~/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3
AU_PATH=~/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component
SYS_VST3_PATH=/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3
SYS_AU_PATH=/Library/Audio/Plug-Ins/Components/${PLUGIN_NAME}.component
VST3_BIN="$VST3_PATH/Contents/MacOS/$PLUGIN_NAME"
AU_BIN="$AU_PATH/Contents/MacOS/$PLUGIN_NAME"

echo "=== Plugin Build Verification ==="
echo "Plugin: $PLUGIN_NAME"
echo "Expected Version: $VERSION"
echo ""

ERRORS=0

# Check 1: VST3 exists
if [ -d "$VST3_PATH" ]; then
    echo "✓ VST3 found: $VST3_PATH"
else
    echo "✗ VST3 NOT FOUND: $VST3_PATH"
    ERRORS=$((ERRORS+1))
fi

# Check 2: AU exists
if [ -d "$AU_PATH" ]; then
    echo "✓ AU found: $AU_PATH"
else
    echo "✗ AU NOT FOUND: $AU_PATH"
    ERRORS=$((ERRORS+1))
fi

# Check 3: Duplicate installs (user vs system)
# DAW may load the OTHER copy. Duplicates are a blocker.
for DUP in "$SYS_VST3_PATH" "$SYS_AU_PATH"; do
    if [ -d "$DUP" ]; then
        echo "✗ DUPLICATE install in system location - BLOCKER: $DUP"
        echo "  DAW may load this copy instead of the user-library one. Delete it."
        ERRORS=$((ERRORS+1))
    fi
done
if [ ! -d "$SYS_VST3_PATH" ] && [ ! -d "$SYS_AU_PATH" ]; then
    echo "✓ No duplicate installs in /Library/Audio/Plug-Ins"
fi

echo ""

# Check 4: Architecture (must be universal)
if [ -f "$VST3_BIN" ]; then
    ARCH=$(file "$VST3_BIN")
    if echo "$ARCH" | grep -q "universal binary" && echo "$ARCH" | grep -q "arm64" && echo "$ARCH" | grep -q "x86_64"; then
        echo "✓ Architecture: Universal (arm64 + x86_64)"
    else
        echo "✗ Architecture: NOT UNIVERSAL - BLOCKER"
        echo "  $ARCH"
        ERRORS=$((ERRORS+1))
    fi

    # Check 5: Deployment target (minos)
    MINOS=$(otool -l "$VST3_BIN" 2>/dev/null | grep -A1 "minos" | grep minos | awk '{print $2}' | head -1)
    if [ "$MINOS" = "$EXPECTED_MINOS" ]; then
        echo "✓ Deployment target (minos): $MINOS"
    else
        echo "✗ Deployment target (minos): $MINOS - BLOCKER (should be $EXPECTED_MINOS)"
        ERRORS=$((ERRORS+1))
    fi
fi

echo ""

# Check 6: Version string baked into the binary
# Proves the installed binary is THIS build, not a stale one.
if [ -n "$VERSION_STRING" ]; then
    for BIN in "$VST3_BIN" "$AU_BIN"; do
        if [ -f "$BIN" ]; then
            if strings "$BIN" 2>/dev/null | grep -qF "$VERSION_STRING"; then
                echo "✓ Version string \"$VERSION_STRING\" found in: $BIN"
            else
                echo "✗ Version string \"$VERSION_STRING\" NOT in binary - BLOCKER (stale build?): $BIN"
                ERRORS=$((ERRORS+1))
            fi
        fi
    done
else
    echo "⚠ VERSION_STRING not set - skipping binary version check"
fi

echo ""

# Check 7: Binary newer than newest source file
# Catches silently-failed or stale builds (SIGPIPE-killed builds, cached
# object files, forgot-to-install, etc.)
if [ -n "$SOURCE_DIR" ] && [ -d "$SOURCE_DIR" ]; then
    NEWEST_SRC=$(find "$SOURCE_DIR" \( -name "*.cpp" -o -name "*.h" \) -exec stat -f "%m %N" {} + 2>/dev/null | sort -rn | head -1)
    NEWEST_SRC_TIME=${NEWEST_SRC%% *}
    NEWEST_SRC_FILE=${NEWEST_SRC#* }
    for BIN in "$VST3_BIN" "$AU_BIN"; do
        if [ -f "$BIN" ]; then
            BIN_TIME=$(stat -f "%m" "$BIN" 2>/dev/null)
            if [ -n "$BIN_TIME" ] && [ -n "$NEWEST_SRC_TIME" ] && [ "$BIN_TIME" -ge "$NEWEST_SRC_TIME" ]; then
                echo "✓ Binary newer than newest source: $BIN"
            else
                echo "✗ Binary OLDER than newest source - BLOCKER (stale build): $BIN"
                echo "  Newest source: $NEWEST_SRC_FILE"
                echo "  Rebuild and reinstall before testing."
                ERRORS=$((ERRORS+1))
            fi
        fi
    done
else
    echo "⚠ SOURCE_DIR not set - skipping binary-vs-source staleness check"
fi

echo ""

# Check 8: moduleinfo.json (VST3)
MODULEINFO="$VST3_PATH/Contents/Resources/moduleinfo.json"
if [ -f "$MODULEINFO" ]; then
    if python3 -m json.tool "$MODULEINFO" > /dev/null 2>&1; then
        echo "✓ moduleinfo.json: Valid JSON"
    else
        echo "✗ moduleinfo.json: INVALID JSON - BLOCKER"
        ERRORS=$((ERRORS+1))
    fi
else
    echo "⚠ moduleinfo.json: Not found (may be OK for some builds)"
fi

echo ""

# Check 9: Code signature
if codesign --verify --deep --strict "$VST3_PATH" 2>/dev/null; then
    echo "✓ VST3 code signature: Valid"
else
    echo "✗ VST3 code signature: INVALID - BLOCKER"
    ERRORS=$((ERRORS+1))
fi

if codesign --verify --deep --strict "$AU_PATH" 2>/dev/null; then
    echo "✓ AU code signature: Valid"
else
    echo "✗ AU code signature: INVALID - BLOCKER"
    ERRORS=$((ERRORS+1))
fi

echo ""

# Check 10: Dependency sanity (otool -L)
# Any non-system path that isn't inside the bundle is a problem
if [ -f "$VST3_BIN" ]; then
    BAD_DEPS=$(otool -L "$VST3_BIN" 2>/dev/null | grep -v "/System\|/usr/lib\|@executable_path\|@loader_path\|@rpath\|:" | grep -v "^$" || true)
    if [ -z "$BAD_DEPS" ]; then
        echo "✓ Dependencies: All system or embedded"
    else
        echo "✗ Dependencies: Non-system paths found - BLOCKER"
        echo "$BAD_DEPS"
        ERRORS=$((ERRORS+1))
    fi
fi

echo ""

# Info: Binary timestamps
for BIN in "$VST3_BIN" "$AU_BIN"; do
    if [ -f "$BIN" ]; then
        TIMESTAMP=$(stat -f "%Sm" "$BIN")
        echo "ℹ $(basename "$(dirname "$(dirname "$(dirname "$BIN")")")") binary timestamp: $TIMESTAMP"
    fi
done

echo ""

# Summary
echo "=== Summary ==="
if [ $ERRORS -eq 0 ]; then
    echo "✓ All checks passed on INSTALLED plugin."
    echo ""
    echo "NEXT: Clear host caches, then test in a NEW project in the DAW."
    echo "      For releases: test the exact uploaded pkg on a clean machine."
    exit 0
else
    echo "✗ $ERRORS BLOCKER(s) found."
    echo ""
    echo "STOP. Do not test. Do not release. Fix blockers first."
    exit 1
fi
