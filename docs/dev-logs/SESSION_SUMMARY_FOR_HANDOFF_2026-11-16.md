# SumatraPDF NG — Session Handoff Log (2026-11-16)

## Executive Summary
Working toward native SumatraPDF NG on Manjaro Linux (KDE Wayland, x86_64) with Foxit/Adobe-like features, full Arabic/RTL support, and AI model integration (Qwen, AlphaCode, etc.). Baseline: commit `b1a8e22577c9e35064315d279fc33ba6b67c5bef`.

---

## What Has Been Done

### 1. Build Environment & Baseline
- **Workspace:** `/home/xorthomson/Projects/sumatrapdf`
- **Target commit:** `b1a8e22` (upstream master)
- **Build command:** `bun cmd/ng-build.ts -linux -dbg`
- **Output:** `out/linux/dbg/SumatraPDF` (186 MB, ELF 64-bit, debug info)
- **Dependencies resolved:** `minio` (npm) for build scripts; `libtesseract.a` corruption fixed via targeted rebuild (not full clean)
- **Launch verified:** Binary starts, reaches GPUI window creation (crashes on headless SSH — expected)

### 2. Features Implemented (local commits, partially pushed)
| Feature | Command ID | Status |
|---------|------------|--------|
| Save Log As | `CmdSaveLogAs` (573) | ✅ Implemented, tested conceptually |
| Save as Searchable PDF (OCR) | `CmdSaveAsSearchablePdf` (574) | ✅ Implemented via `fz_new_pdfocr_writer` |
| Flatten PDF | `CmdFlattenPdf` (575) | ✅ Implemented via `pdf_bake_document` |
| Save Trimmed Pages As | `CmdSaveTrimmedPagesAs` | ✅ Added in `91c7cd8` |
| Arabic font fallbacks (UI + chat) | `gpui.cpp`, `AIChatCommon.cpp` | ✅ Added Noto Sans Arabic, etc. |
| CustomAIProvider improvements | `AIChatAPI.cpp` | ✅ Defaults, guidance, Qwen suggestions |

### 3. Code Changes (Files Modified)
- `ext/gpui/gpui.cpp` — Arabic font fallback chain
- `src/shared/AIChatCommon.cpp` — Arabic font fallback for chat
- `src/shared/AIChatAPI.cpp` — CustomAIProvider enhancements
- `src/shared/Commands.h` — New command IDs (573-575)
- `src/shared/Commands.cpp` — Command metadata
- `src/shared/MenuDefs.cpp` — File menu entries (with separators)
- `src/shared/CommandAvailability.cpp` — Visibility rules
- `src/ng/SumatraPDF.cpp` — Command handlers + critical `SaveCurrentFileAs` fix
- `.gitignore` — Exclude build logs/artifacts
- `docs/dev-logs/SESSION_LOG_2026-11-16.md` — Previous session log
- `AppDir/` — Staged AppImage structure (AppRun, desktop, icons, mime) — **not committed**

### 4. Git History & Push Status
| Branch | Commit | Description |
|--------|--------|-------------|
| `feature/sumatra-linux-arabic-ocr-feedback` (local) | `12b2f3a` | **HEAD of P0 fix branch** (6 commits: `bc9c913`→`12b2f3a`) |
| `sumatrapdf-ng-manjaro-port` (remote `upstream-gh`) | `5c478a9` | Pushed earlier — **without** P0 fixes |
| `fix/p0-menu-dispatch-ocr-safety-2026-11-16` (remote `upstream-gh`) | `12b2f3a` | **Complete P0 safety fixes** (6 commits) |
| `push-temp` (local only) | `5c478a9` | Orphan branch created for clean push |

**Pushed to GitHub:** `sumatrapdf-ng-manjaro-port` → `https://github.com/DrAbdulmalek/sumatrapdf/tree/sumatrapdf-ng-manjaro-port` (at `5c478a9`)

### 5. P0 Safety Fixes (from `upstream-gh/fix/p0-menu-dispatch-ocr-safety-2026-11-16`)
**6 commits, applied to local `feature/sumatra-linux-arabic-ocr-feedback`:**
1. `bc9c913` — Make MuPDF API visible, fix SavePathArgs callback contract
2. `d703fd6` — Adapt OCR/Flatten code to repo's actual contracts
3. `a59e12d` — Reach engine via EngineAll.h helpers (not EngineMupdf.h)
4. `fdb4cfe` — Drop last AsEngineMupdf uses from SaveAsSearchablePdf/FlattenPdf
5. `12b2f3a` — Pass Str (not raw char*) to logf in workers

**Key safety improvements:**
- Workers own independent `fz_context` + `fz_document` (reopen source file)
- No shared `e->_doc` / `pdfdoc` between UI and worker threads
- Atomic save: write to `.ocr-tmp` / `.flatten-tmp` → `rename` on success
- Proper cleanup: `fz_drop_document` / `fz_drop_context` (no `fz_close_document`)
- Official down-cast: `pdf_document_from_fz_document`
- Helpers in `EngineAll.h` to avoid amalgamation breakage
- CI fix: `getGitLinearVersion` with `SUMATRA_BUILD_NUMBER_MIN`, shallow-clone guard

### 6. Build Status
- **Last successful build:** `12b2f3a` (P0 fix branch HEAD) — Exit 0, ~27 min
- **Binary:** `out/linux/dbg/SumatraPDF` (186 MB, debug info)
- **Test:** Starts, loads PDF, crashes on GPUI window (no display in SSH — expected)

---

## Current Repository State (as of 2026-11-16 16:54 UTC)

### Remotes
```
origin          → https://github.com/sumatrapdfreader/sumatrapdf.git (upstream)
upstream-gh     → https://github.com/DrAbdulmalek/sumatrapdf.git (fork)
```

### Branches
```
* feature/sumatra-linux-arabic-ocr-feedback  12b2f3a [upstream-gh/fix/p0-menu-dispatch-ocr-safety-2026-11-16]
  push-temp                                   5c478a9
  sumatrapdf-ng-manjaro-port                  5c478a9 [upstream-gh/sumatrapdf-ng-manjaro-port]
```

### Remote `upstream-gh` refs
```
HEAD  → sumatrapdf-ng-manjaro-port (5c478a9)
fix/p0-menu-dispatch-ocr-safety-2026-11-16  → 12b2f3a
sumatrapdf-ng-manjaro-port                 → 5c478a9
pull/1/head                                → 12b2f3a
pull/1/merge                               → 2c88174
```

---

## Open Issues & Decisions Needed

### 🔴 Critical (Security)
1. **Leaked PAT** — Remote `dr-abdulmalek` previously contained PAT in URL (removed locally). **Must revoke/rotate at GitHub Settings → Developer settings → Personal access tokens.**

### 🟠 High Priority (Functional)
2. **Sync remote `sumatrapdf-ng-manjaro-port` with P0 fixes**
   - Option A: Force-push fix branch → `sumatrapdf-ng-manjaro-port`
     ```bash
     git push upstream-gh fix/p0-menu-dispatch-ocr-safety-2026-11-16:sumatrapdf-ng-manjaro-port --force
     ```
   - Option B: Open PR on fork (`fix/...` → `sumatrapdf-ng-manjaro-port`) for review
   - Option C: Keep as-is; work from local `feature/sumatra-linux-arabic-ocr-feedback`

3. **OCR Arabic support**
   - Install `tesseract-data-ara` (Manjaro: `pamac install tesseract-data-ara`)
   - Add `ocr-language` option (UI: combo box, default `eng`, allow `ara`, `eng+ara`)
   - Add `ocr-datadir` override for custom traineddata locations

4. **OCR/Flatten progress & cancel**
   - Progress callback: `fz_pdfocr_writer_set_progress` → `uitask::Post` page counter
   - Cancel flag: `std::atomic<bool>` checked per page

### 🟡 Medium Priority (Packaging)
5. **AppImage from fixed build**
   - Rebuild `AppDir/` with new binary (`out/linux/dbg/SumatraPDF`)
   - Test on clean VM/container
   - Consider `linuxdeploy` / `appimagetool` for proper bundling

6. **Installable package (Manjaro/AUR)**
   - PKGBUILD for `sumatrapdf-ng-git` or similar
   - Handle dependencies: muPDF, tesseract, leptonica, harfbuzz, freetype, etc.

### 🟢 Lower Priority (Enhancements)
7. **Arabic/RTL full support**
   - UI mirroring (GPUI RTL mode)
   - Text shaping (HarfBuzz already linked)
   - Font config: prefer Noto Sans Arabic / Amiri / Kacst

8. **AI Model Integration**
   - **Qwen:** OpenAI-compatible API → use existing `CustomAIProvider` (done)
   - **AlphaCode:** Need API/CLI compatibility check
   - Local models (llama.cpp, ollama) — optional future

9. **Foxit/Adobe-like features**
   - Form filling / editing
   - Redaction
   - Bates numbering
   - Comparison / diff
   - Batch processing

---

## Available Options for Next Steps

### Option 1: Push P0 fixes to main fork branch (Recommended)
```bash
git push upstream-gh fix/p0-menu-dispatch-ocr-safety-2026-11-16:sumatrapdf-ng-manjaro-port --force
```
- Makes `sumatrapdf-ng-manjaro-port` the authoritative "working" branch
- Clean history (6 commits linear)
- Ready for AppImage build / further features

### Option 2: Continue on local `feature/sumatra-linux-arabic-ocr-feedback`
- Already at `12b2f3a` (P0 fixes HEAD)
- Build, test, add Arabic OCR, progress/cancel, AppImage
- Push final result as new branch or overwrite `sumatrapdf-ng-manjaro-port`

### Option 3: Open PR on fork for review
- `fix/p0-menu-dispatch-ocr-safety-2026-11-16` → `sumatrapdf-ng-manjaro-port`
- Allows CI to run (GitHub Actions: `build.yml` with `SUMATRA_BUILD_NUMBER_MIN`)
- Merge after green CI

### Option 4: Build AppImage now from current binary
- Use `out/linux/dbg/SumatraPDF` (built at `12b2f3a`)
- Populate `AppDir/`, run `appimagetool`
- Test on clean Manjaro (VM/live USB)

### Option 5: Add Arabic OCR + progress/cancel before any push
- Implement `ocr-language` UI, `tesseract-data-ara` detection
- Progress notifications, cancel button
- Then build, test, push complete feature set

---

## Commands Reference

```bash
# Build Debug
cd /home/xorthomson/Projects/sumatrapdf && bun cmd/ng-build.ts -linux -dbg

# Test binary (needs Wayland/X11 display)
./out/linux/dbg/SumatraPDF /path/to/file.pdf

# Push P0 fixes to main fork branch
git push upstream-gh fix/p0-menu-dispatch-ocr-safety-2026-11-16:sumatrapdf-ng-manjaro-port --force

# Install Arabic tesseract data (Manjaro)
pamac install tesseract-data-ara

# Check remote state
git -c credential.helper='!gh auth git-credential' ls-remote https://github.com/DrAbdulmalek/sumatrapdf.git
```

---

## Files for Reference
- `/home/xorthomson/Projects/sumatrapdf/docs/dev-logs/SESSION_LOG_2026-11-16.md` — Previous detailed log
- `/home/xorthomson/Projects/sumatrapdf/docs/dev-logs/SESSION_SUMMARY_FOR_HANDOFF_2026-11-16.md` — This file
- `/home/xorthomson/Projects/sumatrapdf/AppDir/` — Staged AppImage structure

---

## Recommendation for Next Model
**Start with Option 1** (push P0 fixes to `sumatrapdf-ng-manjaro-port`), then **Option 4** (AppImage build), then **Option 5** (Arabic OCR + progress). This gives:
1. Clean, safe baseline on GitHub
2. Distributable artifact for testing
3. Core Arabic feature before further enhancements

**Security first:** Revoke leaked PAT immediately at GitHub.

---

*Generated 2026-11-16 16:55 UTC — Session handoff for multi-model collaboration*