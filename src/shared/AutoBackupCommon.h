/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// Auto-backup of the settings file, inspired by Moon+ Reader's auto backup.
// Pure policy here (interval, file naming, which entries to prune); the ng
// front end does the file system work and keeps a ledger of created backups.

bool AutoBackupShouldRun(i64 lastRunUnix, i64 nowUnix, int intervalSecs);

// "base-YYYYMMDD-HHMMSS.ext" stamp, lexicographically sortable
TempStr AutoBackupStampNameTemp(const char* baseName, const char* ext, i64 unixTime);

// which entries of a ledger (oldest first, as saved) have to be deleted to
// keep at most keepCount backups; returns indices into the ledger
Vec<int> AutoBackupSelectPrune(int ledgerCount, int keepCount);

// Strictly validates the generated backup basename format; never accepts paths.
bool AutoBackupLedgerNameIsSafe(Str name);
