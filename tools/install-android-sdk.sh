#!/usr/bin/env bash
# ============================================================================
# Install everything needed to build the APK, into ~/android-tools.
# No sudo. Nothing touches the system. ~2.6 GB.
#
#   JDK 17            Gradle refuses to run without it
#   cmdline-tools     gives us sdkmanager
#   platform-tools    adb
#   platforms;34      the Android API we compile against
#   build-tools;34    aapt2, d8, apksigner
#   ndk               the C cross-compilers (this is the big one)
# ============================================================================
set -e
ROOT="$HOME/android-tools"
SDK="$ROOT/sdk"
mkdir -p "$ROOT"

if [ ! -x "$ROOT/jdk/bin/java" ]; then
    echo "==> JDK 17"
    curl -sSL -o "$ROOT/jdk.tar.gz" \
      "https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.13%2B11/OpenJDK17U-jdk_x64_linux_hotspot_17.0.13_11.tar.gz"
    tar xzf "$ROOT/jdk.tar.gz" -C "$ROOT"
    mv "$ROOT"/jdk-17* "$ROOT/jdk"
    rm -f "$ROOT/jdk.tar.gz"
fi

if [ ! -x "$SDK/cmdline-tools/latest/bin/sdkmanager" ]; then
    echo "==> Android command-line tools"
    curl -sSL -o "$ROOT/cmdtools.zip" \
      "https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip"
    mkdir -p "$SDK/cmdline-tools"
    unzip -q -o "$ROOT/cmdtools.zip" -d "$SDK/cmdline-tools"
    rm -rf "$SDK/cmdline-tools/latest"
    mv "$SDK/cmdline-tools/cmdline-tools" "$SDK/cmdline-tools/latest"
    rm -f "$ROOT/cmdtools.zip"
fi

export JAVA_HOME="$ROOT/jdk"
export PATH="$JAVA_HOME/bin:$PATH"
SM="$SDK/cmdline-tools/latest/bin/sdkmanager"

echo "==> accepting licences"
yes | "$SM" --sdk_root="$SDK" --licenses > /dev/null 2>&1 || true

echo "==> SDK + NDK (this is the ~2.5GB part; go and make a coffee)"
"$SM" --sdk_root="$SDK" \
    "platform-tools" "platforms;android-34" "build-tools;34.0.0" \
    "ndk;26.3.11579264" "cmake;3.22.1"

echo
echo "done. now:  make apk"
