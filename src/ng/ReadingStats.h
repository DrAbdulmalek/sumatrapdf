/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

struct MainWindow;

// per-second reading-time accumulation behind Moon+ Reader's statistics; the
// data file lives next to the settings in the app data dir
void ReadingStatsTick(MainWindow* win, int elapsedMs);
void ReadingStatsShowDialog(MainWindow* win);
// flush pending seconds to the data file (called on dialog open and exit)
void ReadingStatsFlush();
