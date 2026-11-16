/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"
#include "base/File.h"
#include "AppTools.h"

#include "AutoBackup.h"
#include "AutoBackupCommon.h"

#include "Settings.h"
#include "AppSettings.h"
#include "MainWindow.h"
#include "SumatraPDF.h"

#include "SumatraLog.h"

// Backs up SumatraPDF-settings.txt once a day. The ledger ("ledger.tsv", one
// file name per line, oldest first) drives rotation; kKeepBackups copies stay.

static constexpr int kIntervalSecs = 86400;
static constexpr int kKeepBackups = 8;
static constexpr int kCheckEveryMs = 5 * 60 * 1000; // decide every 5 minutes, not per tick

static int gCheckAccumMs = 0;
static bool gHaveLastRun = false;
static i64 gLastRunUnix = 0;

static TempStr BackupDirTemp() {
    return path::JoinTemp(GetAppDataDirTemp(), StrL("backup"));
}

static TempStr LedgerPathTemp() {
    return path::JoinTemp(BackupDirTemp(), StrL("ledger.tsv"));
}

static TempStr LastRunPathTemp() {
    return path::JoinTemp(BackupDirTemp(), StrL("last-run.txt"));
}

static void LoadBackupState() {
    if (gHaveLastRun) {
        return;
    }
    gHaveLastRun = true;
    gLastRunUnix = 0;
    TempStr path = LastRunPathTemp();
    Str data = file::ReadFile(path);
    gLastRunUnix = ParseInt64(data);
    str::Free(data);
}

static bool WriteFileAtomically(Str path, Str data) {
    TempStr tempPath = MakeUniqueFilePathTemp(path);
    if (!file::WriteFile(tempPath, data) || !file::RenameReplace(path, tempPath)) {
        file::Delete(tempPath);
        return false;
    }
    return true;
}

static bool SaveBackupState(i64 nowUnix) {
    TempStr path = LastRunPathTemp();
    if (!dir::CreateForFile(path)) return false;
    TempStr data = fmt("%lld\n", (long long)nowUnix);
    return WriteFileAtomically(path, data);
}

// Keep ledger slices valid until all uses complete; Split() returns views into
// the original buffer. Commit the new ledger before pruning any old backup.
static bool RotateLedger(TempStr newName) {
    if (!AutoBackupLedgerNameIsSafe(newName)) return false;
    StrVec names;
    Str ledgerData;
    TempStr ledgerPath = LedgerPathTemp();
    if (file::Exists(ledgerPath)) {
        ledgerData = file::ReadFile(ledgerPath);
        if (len(ledgerData) == 0) { str::Free(ledgerData); return false; }
        Split(&names, ledgerData, StrL("\n"), true);
    }
    StrVec safeNames;
    for (Str name : names) {
        if (AutoBackupLedgerNameIsSafe(name)) safeNames.Append(name);
    }
    safeNames.Append(newName);
    Vec<int> toPrune = AutoBackupSelectPrune(len(safeNames), kKeepBackups);
    int firstKept = len(toPrune) > 0 ? toPrune[len(toPrune) - 1] + 1 : 0;
    str::Builder out;
    for (int i = firstKept; i < len(safeNames); i++) {
        out.Append(safeNames[i]);
        out.Append(StrL("\n"));
    }
    if (!dir::CreateForFile(ledgerPath) || !WriteFileAtomically(ledgerPath, ToStrTemp(out))) {
        str::Free(ledgerData);
        return false;
    }
    for (int i : toPrune) {
        if (AutoBackupLedgerNameIsSafe(safeNames[i])) {
            TempStr dead = path::JoinTemp(BackupDirTemp(), safeNames[i]);
            file::Delete(dead);
        }
    }
    str::Free(ledgerData);
    return true;
}

static bool AutoBackupRun() {
    TempStr dir = BackupDirTemp();
    TempStr settingsPath = GetSettingsPathTemp();
    i64 nowUnix = (i64)time(nullptr);
    TempStr name;
    TempStr dst;
    for (int attempt = 0; attempt < 10; attempt++) {
        name = AutoBackupStampNameTemp("SumatraPDF-settings", ".txt", nowUnix + attempt);
        dst = path::JoinTemp(dir, name);
        if (!file::Exists(dst)) { nowUnix += attempt; break; }
        if (attempt == 9) return false;
    }
    Str data = file::ReadFile(settingsPath);
    if (len(data) == 0) { str::Free(data); return false; }
    if (!dir::CreateForFile(dst)) { str::Free(data); return false; }
    TempStr tempPath = MakeUniqueFilePathTemp(dst);
    bool ok = file::WriteFile(tempPath, data);
    str::Free(data);
    if (!ok || !file::RenameReplace(dst, tempPath)) {
        file::Delete(tempPath);
        logf("AutoBackup: writing '%s' failed\n", dst);
        return false;
    }
    if (!RotateLedger(name)) {
        file::Delete(dst);
        logf("AutoBackup: ledger update failed for '%s'\n", dst);
        return false;
    }
    if (!SaveBackupState(nowUnix)) logf("AutoBackup: last-run state could not be persisted\n");
    gLastRunUnix = nowUnix;
    logf("AutoBackup: saved '%s'\n", dst);
    return true;
}

void AutoBackupTick(MainWindow* win, int elapsedMs) {
    (void)win;
    gCheckAccumMs += elapsedMs;
    if (gCheckAccumMs < kCheckEveryMs) {
        return;
    }
    gCheckAccumMs = 0;
    LoadBackupState();
    i64 nowUnix = (i64)time(nullptr);
    if (AutoBackupShouldRun(gLastRunUnix, nowUnix, kIntervalSecs)) {
        AutoBackupRun();
    }
}
