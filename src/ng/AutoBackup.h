/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// once-a-day backup of the settings file into <app data>/backup, like Moon+
// Reader's auto backup; a ledger TSV tracks the created copies for rotation
struct MainWindow;

void AutoBackupTick(MainWindow* win, int elapsedMs);
