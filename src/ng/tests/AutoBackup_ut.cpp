/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "AutoBackupCommon.h"

// must be last to over-write assert()
#include "base/tests/UtAssert.h"

void AutoBackup_UnitTests() {
    // interval policy: never ran -> run now; inside the interval -> wait
    utassert(AutoBackupShouldRun(0, 1000, 86400));
    utassert(AutoBackupShouldRun(-1, 1000, 86400));
    utassert(!AutoBackupShouldRun(1000, 1000 + 3600, 86400));
    utassert(AutoBackupShouldRun(1000, 1000 + 86400, 86400));

    // stamp names sort chronologically as strings
    i64 t1 = 1731744000;
    i64 t2 = t1 + 3600;
    TempStr s1 = AutoBackupStampNameTemp("settings", ".txt", t1);
    TempStr s2 = AutoBackupStampNameTemp("settings", ".txt", t2);
    utassert(str::StartsWith(s1, StrL("settings-")));
    utassert(str::EndsWith(s1, StrL(".txt")));
    utassert(str::Cmp(s1, s2) < 0);

    // rotation: keep the newest entries, oldest first in the ledger
    Vec<int> none = AutoBackupSelectPrune(3, 8);
    utassert(len(none) == 0);
    Vec<int> prune = AutoBackupSelectPrune(10, 8);
    utassert(len(prune) == 2);
    utassert(prune[0] == 0 && prune[1] == 1);
    utassert(len(AutoBackupSelectPrune(8, 8)) == 0);
    utassert(len(AutoBackupSelectPrune(5, 0)) == 0);
}
