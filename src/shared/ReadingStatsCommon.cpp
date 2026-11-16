/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "ReadingStatsCommon.h"

// minutes thresholds for the 12-week calendar grid (Moon+'s has 4 shades)
static constexpr int kHeatMinuteLevels[4] = {10, 25, 45, 70};

i64 ReadingStatsDayIndexFromUnix(i64 unixTime) {
    // seconds from the local midnight of the epoch day; time.h for local offset
    time_t t = (time_t)unixTime;
    struct tm lt;
#if OS_WIN
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    // normalize so that the division is on the local day start
    i64 localMidnight = unixTime - lt.tm_hour * 3600 - lt.tm_min * 60 - lt.tm_sec;
    return localMidnight / 86400;
}

static int DayPos(const ReadingStatsData* d, i64 dayIndex) {
    for (int i = 0; i < len(d->dayIndex); i++) {
        if (d->dayIndex[i] == dayIndex) {
            return (int)i;
        }
    }
    return -1;
}

void ReadingStatsAccumDay(ReadingStatsData* d, i64 dayIndex, int seconds) {

    int pos = DayPos(d, dayIndex);
    if (pos < 0) {
        VecAppend(d->dayIndex, dayIndex);
        VecAppend(d->seconds, seconds);
        return;
    }
    d->seconds[pos] += seconds;
}

int ReadingStatsDaySeconds(const ReadingStatsData* d, i64 dayIndex) {
    int pos = DayPos(d, dayIndex);
    if (pos < 0) {
        return 0;
    }
    return d->seconds[pos];
}

i64 ReadingStatsTotalSeconds(const ReadingStatsData* d) {
    i64 total = d->lifetimeSeconds;
    for (int s : d->seconds) {
        total += s;
    }
    return total;
}

void ReadingStatsAccumFile(ReadingStatsData* d, Str fileName, int seconds) {

    if (len(fileName) == 0) {
        return;
    }
    for (int i = 0; i < len(d->fileNames); i++) {
        if (str::Eq(d->fileNames[i], fileName)) {
            d->fileSeconds[i] += seconds;
            return;
        }
    }
    VecAppend(d->fileNames, str::Dup(fileName));
    VecAppend(d->fileSeconds, seconds);
}

void ReadingStatsFree(ReadingStatsData* d) {
    for (Str& s : d->fileNames) {
        str::Free(s);
    }
    VecReset(d->fileNames);
    VecReset(d->fileSeconds);
    VecReset(d->dayIndex);
    VecReset(d->seconds);
    d->lifetimeSeconds = 0;
}

int ReadingStatsStreak(const ReadingStatsData* d, i64 todayIndex) {
    if (ReadingStatsDaySeconds(d, todayIndex) == 0) {
        return 0;
    }
    int streak = 1;
    i64 day = todayIndex - 1;
    while (ReadingStatsDaySeconds(d, day) > 0) {
        streak++;
        day--;
    }
    return streak;
}

int ReadingStatsHeatLevel(int daySeconds) {
    int mins = daySeconds / 60;
    for (int i = 3; i >= 0; i--) {
        if (mins >= kHeatMinuteLevels[i]) {
            return i + 1;
        }
    }
    if (mins > 0) {
        return 1;
    }
    return 0;
}

void ReadingStatsSerialize(const ReadingStatsData* d, str::Builder* out) {
    out->Append(fmt("total %lld\n", (long long)d->lifetimeSeconds));
    for (int i = 0; i < len(d->dayIndex); i++) {
        out->Append(fmt("days %lld %d\n", d->dayIndex[i], d->seconds[i]));
    }
    for (int i = 0; i < len(d->fileNames); i++) {
        // file names are single-line display names; replace any newline to
        // keep the TSV line-per-record format
        TempStr name = str::ReplaceTemp(d->fileNames[i], StrL("\n"), StrL(" "));
        out->Append(StrL("files "));
        out->Append(name);
        out->Append(fmt(" %lld\n", d->fileSeconds[i]));
    }
}

bool ReadingStatsParse(Str data, ReadingStatsData* d) {
    if (len(data) == 0) {
        return false;
    }
    StrVec lines;
    Split(&lines, data, StrL("\n"), true);
    for (Str line : lines) {
        if (len(line) == 0 || line.s[0] == '#') {
            continue;
        }
        StrVec parts;
        Split(&parts, line, StrL(" "), true, 3);
        if (len(parts) < 2) {
            continue;
        }
        if (str::Eq(parts[0], StrL("total")) && len(parts) == 2) {
            d->lifetimeSeconds = std::max<i64>(0, ParseInt64(parts[1]));
        } else if (str::Eq(parts[0], StrL("days")) && len(parts) == 3) {
            i64 day = ParseInt64(parts[1]);
            int secs = ParseInt(parts[2]);
            ReadingStatsAccumDay(d, day, secs);
        } else if (str::Eq(parts[0], StrL("files")) && len(parts) == 3) {
            // name was saved with newlines replaced, so it can contain spaces:
            // max=3 keeps it whole in parts[1]
            i64 secs = ParseInt64(parts[2]);
            ReadingStatsAccumFile(d, parts[1], (int)secs);
        }
    }
    return len(d->dayIndex) > 0 || len(d->fileNames) > 0 || d->lifetimeSeconds > 0;
}
