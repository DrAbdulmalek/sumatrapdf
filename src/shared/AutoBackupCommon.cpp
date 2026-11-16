/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "AutoBackupCommon.h"

bool AutoBackupShouldRun(i64 lastRunUnix, i64 nowUnix, int intervalSecs) {
    if (lastRunUnix <= 0) {
        return true;
    }
    return nowUnix - lastRunUnix >= intervalSecs;
}

TempStr AutoBackupStampNameTemp(const char* baseName, const char* ext, i64 unixTime) {
    time_t t = (time_t)unixTime;
    struct tm lt;
#if OS_WIN
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    return fmt("%s-%04d%02d%02d-%02d%02d%02d%s", baseName, lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour,
               lt.tm_min, lt.tm_sec, ext);
}

Vec<int> AutoBackupSelectPrune(int ledgerCount, int keepCount) {
    Vec<int> res;
    if (keepCount <= 0 || ledgerCount <= keepCount) {
        return res;
    }
    for (int i = 0; i < ledgerCount - keepCount; i++) {
        VecAppend(res, i);
    }
    return res;
}

bool AutoBackupLedgerNameIsSafe(Str name) {
    constexpr Str prefix = StrL("SumatraPDF-settings-");
    constexpr Str suffix = StrL(".txt");
    if (len(name) != 39 || !str::StartsWith(name, prefix) || !str::EndsWith(name, suffix)) return false;
    for (int i = len(prefix); i < len(name) - len(suffix); i++) {
        char c = name.s[i];
        if (i == len(prefix) + 8) { if (c != '-') return false; }
        else if (c < '0' || c > '9') return false;
    }
    return true;
}
