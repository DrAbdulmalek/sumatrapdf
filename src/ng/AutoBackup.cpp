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

static void SaveBackupState(i64 nowUnix) {
    TempStr path = LastRunPathTemp();
    dir::CreateForFile(path);
    TempStr data = fmt("%lld\n", (long long)nowUnix);
    Str s = str::Dup(data);
    file::WriteFile(path, s);
    str::Free(s);
}

// appends the new name and deletes the oldest files beyond kKeepBackups
static void RotateLedger(TempStr newName) {
    StrVec names;
    TempStr ledgerPath = LedgerPathTemp();
    if (file::Exists(ledgerPath)) {
        Str data = file::ReadFile(ledgerPath);
        Split(&names, data, StrL("\n"), true);
        str::Free(data);
    }
    names.Append(str::Dup(newName));

    Vec<int> toPrune = AutoBackupSelectPrune(len(names), kKeepBackups);
    for (int i : toPrune) {
        TempStr dead = path::JoinTemp(BackupDirTemp(), names[i]);
        file::Delete(dead);
    }
    int firstKept = len(toPrune) > 0 ? toPrune[len(toPrune) - 1] + 1 : 0;
    str::Builder out;
    for (int i = firstKept; i < len(names); i++) {
        out.Append(names[i]);
        out.Append(StrL("\n"));
    }
    dir::CreateForFile(ledgerPath);
    Str content = str::Dup(ToStrTemp(out));
    file::WriteFile(ledgerPath, content);
    str::Free(content);
}

static void AutoBackupRun() {
    TempStr dir = BackupDirTemp();
    TempStr settingsPath = GetSettingsPathTemp();
    i64 nowUnix = (i64)time(nullptr);
    TempStr name = AutoBackupStampNameTemp("SumatraPDF-settings", ".txt", nowUnix);
    TempStr dst = path::JoinTemp(dir, name);
    Str data = file::ReadFile(settingsPath);
    if (len(data) == 0) {
        str::Free(data);
        return;
    }
    dir::CreateForFile(dst);
    bool ok = file::WriteFile(dst, data);
    str::Free(data);
    if (!ok) {
        logf("AutoBackup: writing '%s' failed\n", dst);
        return;
    }
    RotateLedger(name);
    SaveBackupState(nowUnix);
    logf("AutoBackup: saved '%s'\n", dst);
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
