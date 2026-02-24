#!/bin/bash
# KingDubby macOS Installer Build Script
# Creates a signed and notarized .pkg installer

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"
INSTALLER_DIR="$SCRIPT_DIR"
VERSION="1.0.1"
PKG_NAME="KingDubby-$VERSION-macOS"

# Signing identities (from user's CLAUDE.md)
CODE_SIGN_IDENTITY="Developer ID Application: Nathan Turczan (9DVDAB3FWL)"
INSTALLER_SIGN_IDENTITY="Developer ID Installer: Nathan Turczan (9DVDAB3FWL)"
NOTARYTOOL_PROFILE="notarytool-profile"

echo "=== KingDubby macOS Installer Builder ==="
echo "Project: $PROJECT_DIR"
echo "Build: $BUILD_DIR"
echo ""

# Step 1: Build Release
echo "Step 1: Building Release..."
cd "$PROJECT_DIR/Builds/MacOSX"
xcodebuild -project KingDubby.xcodeproj \
    -scheme "KingDubby - All" \
    -configuration Release \
    ARCHS="x86_64 arm64" \
    ONLY_ACTIVE_ARCH=NO \
    build

# Step 2: Create staging directory
echo "Step 2: Creating staging directory..."
STAGING_DIR="$INSTALLER_DIR/staging"
rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/vst3"
mkdir -p "$STAGING_DIR/au"

# Copy built plugins
cp -R "$BUILD_DIR/KingDubby.vst3" "$STAGING_DIR/vst3/"
cp -R "$BUILD_DIR/KingDubby.component" "$STAGING_DIR/au/"

# Step 3: Sign plugins
echo "Step 3: Signing plugins..."
codesign --force --sign "$CODE_SIGN_IDENTITY" \
    --timestamp --options runtime --deep \
    "$STAGING_DIR/vst3/KingDubby.vst3"

codesign --force --sign "$CODE_SIGN_IDENTITY" \
    --timestamp --options runtime --deep \
    "$STAGING_DIR/au/KingDubby.component"

# Verify signatures
echo "Verifying signatures..."
codesign -dv --verbose=2 "$STAGING_DIR/vst3/KingDubby.vst3"
codesign -dv --verbose=2 "$STAGING_DIR/au/KingDubby.component"

# Step 4: Build component packages
echo "Step 4: Building component packages..."

# VST3 package
pkgbuild --root "$STAGING_DIR/vst3" \
    --identifier "com.ScaleNavigatorLLC.KingDubby.vst3" \
    --version "$VERSION" \
    --install-location "/Library/Audio/Plug-Ins/VST3" \
    "$INSTALLER_DIR/KingDubby-VST3.pkg"

# AU package
pkgbuild --root "$STAGING_DIR/au" \
    --identifier "com.ScaleNavigatorLLC.KingDubby.component" \
    --version "$VERSION" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    "$INSTALLER_DIR/KingDubby-AU.pkg"

# Step 5: Create distribution XML
echo "Step 5: Creating distribution package..."
cat > "$INSTALLER_DIR/distribution.xml" << 'EOF'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>KingDubby</title>
    <organization>com.ScaleNavigatorLLC</organization>
    <domains enable_localSystem="true"/>
    <options customize="allow" require-scripts="false" hostArchitectures="x86_64,arm64"/>

    <welcome file="welcome.html"/>
    <license file="license.txt"/>

    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
    </choices-outline>

    <choice id="vst3" title="VST3 Plugin" description="Install KingDubby VST3 to /Library/Audio/Plug-Ins/VST3">
        <pkg-ref id="com.ScaleNavigatorLLC.KingDubby.vst3"/>
    </choice>

    <choice id="au" title="Audio Unit Plugin" description="Install KingDubby AU to /Library/Audio/Plug-Ins/Components">
        <pkg-ref id="com.ScaleNavigatorLLC.KingDubby.component"/>
    </choice>

    <pkg-ref id="com.ScaleNavigatorLLC.KingDubby.vst3" version="1.0.1" onConclusion="none">KingDubby-VST3.pkg</pkg-ref>
    <pkg-ref id="com.ScaleNavigatorLLC.KingDubby.component" version="1.0.1" onConclusion="none">KingDubby-AU.pkg</pkg-ref>
</installer-gui-script>
EOF

# Create welcome.html
cat > "$INSTALLER_DIR/welcome.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; padding: 20px; }
        h1 { color: #333; }
        p { color: #666; line-height: 1.6; }
        .highlight { color: #FFD700; font-weight: bold; }
    </style>
</head>
<body>
    <h1>KingDubby</h1>
    <p>Dub tape delay plugin - a recreation of the classic Lowcoders plugin from 2008.</p>
    <p>Revived in 2026 by <span class="highlight">Scale Navigator</span></p>
    <p>This installer will install:</p>
    <ul>
        <li><strong>VST3</strong> to /Library/Audio/Plug-Ins/VST3/</li>
        <li><strong>Audio Unit</strong> to /Library/Audio/Plug-Ins/Components/</li>
    </ul>
    <p>Visit <a href="https://scalenavigator.com">scalenavigator.com</a> for more free plugins.</p>
</body>
</html>
EOF

# Create license.txt
cat > "$INSTALLER_DIR/license.txt" << 'EOF'
KingDubby - Dub Tape Delay Plugin
Copyright (c) 2026 Scale Navigator LLC

Original plugin by Franck "Lowcoders" Stauffer (2008)
Revived with blessing from original developer.

This software is provided free of charge for personal and commercial use.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF

# Build product archive
productbuild --distribution "$INSTALLER_DIR/distribution.xml" \
    --package-path "$INSTALLER_DIR" \
    --resources "$INSTALLER_DIR" \
    "$INSTALLER_DIR/${PKG_NAME}-unsigned.pkg"

# Step 6: Sign the installer
echo "Step 6: Signing installer..."
productsign --sign "$INSTALLER_SIGN_IDENTITY" \
    "$INSTALLER_DIR/${PKG_NAME}-unsigned.pkg" \
    "$INSTALLER_DIR/${PKG_NAME}.pkg"

# Verify installer signature
echo "Verifying installer signature..."
pkgutil --check-signature "$INSTALLER_DIR/${PKG_NAME}.pkg"

# Step 7: Notarize
echo "Step 7: Submitting for notarization..."
xcrun notarytool submit "$INSTALLER_DIR/${PKG_NAME}.pkg" \
    --keychain-profile "$NOTARYTOOL_PROFILE" \
    --wait

# Step 8: Staple
echo "Step 8: Stapling notarization ticket..."
xcrun stapler staple "$INSTALLER_DIR/${PKG_NAME}.pkg"

# Verify final result
echo "Step 9: Final verification..."
spctl -a -vv -t install "$INSTALLER_DIR/${PKG_NAME}.pkg"

# Cleanup
echo "Cleaning up..."
rm -rf "$STAGING_DIR"
rm -f "$INSTALLER_DIR/KingDubby-VST3.pkg"
rm -f "$INSTALLER_DIR/KingDubby-AU.pkg"
rm -f "$INSTALLER_DIR/${PKG_NAME}-unsigned.pkg"

echo ""
echo "=== SUCCESS ==="
echo "Installer created: $INSTALLER_DIR/${PKG_NAME}.pkg"
echo ""
