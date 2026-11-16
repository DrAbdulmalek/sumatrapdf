# Building SumatraPDF NG on Manjaro

This is the native Manjaro validation path. The GitHub workflow also builds in the official Manjaro base container, but a container build is not a substitute for testing the user's installed Manjaro/KDE environment.

## System dependencies

Install the native build toolchain and Linux UI development packages:

```bash
sudo pacman -S --needed base-devel git pkgconf nasm \
  libx11 cairo pango gdk-pixbuf2 glib2 openssl \
  fontconfig ttf-dejavu ttf-liberation noto-fonts
```

Refresh the font cache after installation:

```bash
fc-cache -f
```

Bun must be available on `PATH`. Confirm the toolchain before building:

```bash
bun --version
g++ --version
ar --version
git status --short
git rev-parse --short HEAD
```

## Build and test

Run these commands from the repository root. They do not delete the existing `out/` tree or modify boot/system configuration.

```bash
set -o pipefail

bun cmd/ng-build.ts -linux -dbg test_util -run -- -for-ai \
  2>&1 | tee /tmp/sumatrapdf-ng-test-util.log

bun cmd/ng-build.ts -linux -dbg SumatraPDF \
  2>&1 | tee /tmp/sumatrapdf-ng-debug.log
```

Verify the generated Tesseract archive rather than applying `ranlib` blindly:

```bash
archive="$(find out/linux/dbg -type f -name '*tesseract*.a' -print -quit)"
test -n "$archive"
file "$archive"
ar t "$archive" >/dev/null
```

Then build the release target:

```bash
bun cmd/ng-build.ts -linux -rel SumatraPDF \
  2>&1 | tee /tmp/sumatrapdf-ng-release.log
```

Expected outputs:

- `out/linux/dbg/SumatraPDF`
- `out/linux/rel/SumatraPDF`

## If the Tesseract archive fails

Do not immediately delete `out/`, run `ranlib`, or remove individual archives. Preserve the evidence first:

```bash
file "$archive"
ar t "$archive"
stat "$archive"
git status --short
```

Attach the three logs under `/tmp` and the archive inspection output to the review. The CI job independently checks the archive with both `file` and `ar t`.

## OCR model files

The build bundles the Tesseract engine, not the language models. For runtime OCR, install or supply the required `.traineddata` files and configure `ocr-settings.txt` as described in [ocr-settings.md](ocr-settings.md). The application validates the selected models before starting OCR.
