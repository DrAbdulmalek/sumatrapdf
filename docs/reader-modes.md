# Reader modes from the mobile readers (Manjaro port)

Three features ported from an analysis of two mobile reading apps —
Moon+ Reader Pro 10.5 and Foxit PDF Editor (Lite) 12.2.3 — as original
clean-room code. No code was taken from either APK (both are proprietary;
their binaries were only studied for behavior, see the feature tables in
`docs/reader_features_analysis.md` of the clinic project for that analysis).

## What was added

| Command | Menu | Source feature | Files |
| --- | --- | --- | --- |
| `CmdReadingStats` (576) | View → Reading Statistics... | Moon+ Reader's daily reading time, streak, 12-week calendar | `src/shared/ReadingStatsCommon.*`, `src/ng/ReadingStats.*` |
| `CmdExportNotesMd` (577) | File → Export Notes as Markdown... | Foxit's comment data exporting / Moon+ highlight sharing | `src/shared/NotesExportCommon.*`, `src/ng/NotesExport.cpp` |
| (automatic) | — | Moon+ Reader's auto backup (v9) | `src/shared/AutoBackupCommon.*`, `src/ng/AutoBackup.cpp` |

## Reading statistics

`ReadingStatsTick` runs from the shell tick and counts foreground seconds
while a tab is open (paused when the window is minimized). The data lands in
`<app data dir>/reading-stats.tsv`:

```
days <dayIndex> <seconds>
files <display name> <seconds>
```

`dayIndex` is days since the Unix epoch in local time. The dialog (View menu)
shows today's minutes, the streak of consecutive reading days, the total, a
12-week heatmap (4 shades, minute thresholds 10/25/45/70 like Moon+'s map)
and the most-read documents. The file is rewritten every 30 counted seconds
and pruned to the last 366 days on load.

## Export notes as Markdown

Collects the annotations of every page (`EngineMupdfLoadAnnotsForPage` +
`FzPageInfo::annotations`), maps `AnnotationType` to a human label (Highlight,
Underline, Note, Ink, Stamp, ...), keeps the annotation contents as the
comment, and writes Markdown grouped by page. Annotations without text and
non-verbal types (links) are skipped. Saved through the same save dialog the
other Manjaro-port commands use.

## Auto backup

Once a day (state in `<app data>/backup/last-run.txt`) the settings file is
copied to `<app data>/backup/SumatraPDF-settings-<stamp>.txt`. A ledger
(`backup/ledger.tsv`) tracks the copies; the oldest are deleted beyond 8.
Pure policy (interval, stamp naming, rotation selection) lives in
`AutoBackupCommon` and is unit-tested.

## Unit tests

`ReadingStats_UnitTests`, `NotesExport_UnitTests`, `AutoBackup_UnitTests` in
`src/ng/tests/`, registered in `SumatraPDF_UnitTests` — run with
`bun cmd/run-unit-tests.ts -dbg` (they cover accumulation, streaks, heat
levels, the TSV round trip, markdown building, interval policy, stamp sorting
and rotation selection).

## Pre-existing build blockers fixed on the way

The tip did not compile on Linux before this branch:

1. `MkFunc1(OnLogPathPicked, args)` — the handler had a 1-arg signature while
   `MkFunc1` requires `(ctx, args)`; fixed to `(SavePathArgs*, void*)`.
2. `SearchablePdfCtx` used MuPDF types (`fz_document_writer`, ...) without any
   MuPDF include; `#include <mupdf/pdf.h>` added.
3. Command dispatch fell through from `CmdSaveAs` through `CmdSaveLogAs` /
   `CmdSaveAsSearchablePdf` / `CmdFlattenPdf` (no `break`), so "Save As..."
   also started the OCR and flatten pipelines; breaks added.
