/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "ReadingStatsCommon.h"

// must be last to over-write assert()
#include "base/tests/UtAssert.h"

void ReadingStats_UnitTests() {
    // day index: two times within the same day (far from midnight edges) share it
    i64 noon = 1731744000; // 2024-11-16 08:00 UTC + local offset, day is stable
    i64 d1 = ReadingStatsDayIndexFromUnix(noon);
    i64 d2 = ReadingStatsDayIndexFromUnix(noon + 3600);
    utassert(d1 == d2);
    i64 d3 = ReadingStatsDayIndexFromUnix(noon + 86400 * 3);
    utassert(d3 > d1);

    // accumulate + read back
    ReadingStatsData data;
    ReadingStatsAccumDay(&data, d1, 60);
    ReadingStatsAccumDay(&data, d1, 30);
    utassert(ReadingStatsDaySeconds(&data, d1) == 90);
    utassert(ReadingStatsDaySeconds(&data, d2) == 0);
    ReadingStatsAccumDay(&data, d1 + 1, 120);
    ReadingStatsAccumDay(&data, d1 + 2, 10);
    utassert(ReadingStatsTotalSeconds(&data) == 90 + 120 + 10);

    // streak counts back from the last day, stops at a gap
    utassert(ReadingStatsStreak(&data, d1 + 2) == 3);
    utassert(ReadingStatsStreak(&data, d1 + 3) == 0);
    utassert(ReadingStatsStreak(&data, d1) == 1);

    // heat levels: minutes thresholds
    utassert(ReadingStatsHeatLevel(0) == 0);
    utassert(ReadingStatsHeatLevel(5 * 60) == 1);
    utassert(ReadingStatsHeatLevel(25 * 60) == 2);
    utassert(ReadingStatsHeatLevel(45 * 60) == 3);
    utassert(ReadingStatsHeatLevel(70 * 60) == 4);

    // per-file totals; names with spaces survive the round trip
    ReadingStatsAccumFile(&data, StrL("ortho guide.pdf"), 10);
    ReadingStatsAccumFile(&data, StrL("ortho guide.pdf"), 20);
    ReadingStatsAccumFile(&data, StrL("other.pdf"), 5);
    utassert(len(data.fileNames) == 2);

    data.lifetimeSeconds = 86400;
    utassert(ReadingStatsTotalSeconds(&data) == 86400 + 90 + 120 + 10);

    // serialize -> parse round trip
    str::Builder out;
    ReadingStatsSerialize(&data, &out);
    ReadingStatsData parsed;
    utassert(ReadingStatsParse(ToStrTemp(out), &parsed));
    utassert(parsed.lifetimeSeconds == 86400);
    utassert(ReadingStatsTotalSeconds(&parsed) == 86400 + 90 + 120 + 10);
    utassert(ReadingStatsDaySeconds(&parsed, d1) == 90);
    utassert(ReadingStatsDaySeconds(&parsed, d1 + 1) == 120);
    utassert(len(parsed.fileNames) == 2);
    utassert(str::Eq(parsed.fileNames[0], StrL("ortho guide.pdf")));
    utassert(parsed.fileSeconds[0] == 30);

    // garbage in, no crash out
    ReadingStatsData junk;
    utassert(!ReadingStatsParse(StrL("hello world"), &junk));
    ReadingStatsFree(&data);
    ReadingStatsFree(&parsed);
}
