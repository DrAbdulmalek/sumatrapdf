/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// Per-day reading-time tracking and per-document totals, inspired by Moon+
// Reader's reading statistics (daily minutes, streak, year heatmap). Pure
// logic here; persistence is a small TSV file owned by the ng front end.

struct ReadingStatsData {
    // Accumulated seconds from days pruned out of the daily history.
    i64 lifetimeSeconds = 0;
    // parallel arrays, one entry per day that has any reading time
    Vec<i64> dayIndex; // days since Unix epoch, local time
    Vec<int> seconds;
    // parallel arrays, total reading time per document display name
    Vec<Str> fileNames; // entries are owned (str::Dup), free with ReadingStatsFree
    Vec<i64> fileSeconds;
};

// days since Unix epoch, in local time (midnight-to-midnight like Moon+)
i64 ReadingStatsDayIndexFromUnix(i64 unixTime);
void ReadingStatsAccumDay(ReadingStatsData* d, i64 dayIndex, int seconds);
int ReadingStatsDaySeconds(const ReadingStatsData* d, i64 dayIndex);
i64 ReadingStatsTotalSeconds(const ReadingStatsData* d);
void ReadingStatsAccumFile(ReadingStatsData* d, Str fileName, int seconds);
void ReadingStatsFree(ReadingStatsData* d);

// consecutive days with reading time ending at todayIndex (0 if none today)
int ReadingStatsStreak(const ReadingStatsData* d, i64 todayIndex);

// heat level 0..4 for a calendar grid; thresholds in minutes like Moon+'s map
int ReadingStatsHeatLevel(int daySeconds);

// TSV serialization: "days <dayIndex> <seconds>" and "files <name> <seconds>"
// lines. Pure so the format can be tested without touching the file system.
void ReadingStatsSerialize(const ReadingStatsData* d, str::Builder* out);
bool ReadingStatsParse(Str data, ReadingStatsData* d);
