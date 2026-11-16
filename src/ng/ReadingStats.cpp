/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "gui/GpuiBridge.h"
#include "gui/Dpi.h"
#include "gui/UIModels.h"

#include "Settings.h"
#include "AppSettings.h"
#include "Commands.h"
#include "Translations.h"
#include "DisplayMode.h"
#include "DocController.h"
#include "EngineBase.h"
#include "DisplayModel.h"
#include "Theme.h"
#include "AIChatTypes.h"
#include "base/File.h"
#include "WindowTab.h"
#include "MainWindow.h"
#include "SumatraPDF.h"
#include "AppTools.h"
#include "ReadingAutoScrollCommon.h"
#include "gui/AppShell.h"
#include "gui/ToolWindow.h"
#include "Notifications.h"

#include "ReadingStats.h"
#include "ReadingStatsCommon.h"

// Wall-clock reading time while a document is open, like Moon+ Reader tracks
// it. Counts foreground seconds per day and per display name; the file is
// rewritten at most every kFlushSeconds seconds of counted reading.

static constexpr int kAccumMs = 1000; // tick granularity
static constexpr int kFlushSecs = 30; // rewrite the data file every N counted seconds
static constexpr int kKeepDays = 366; // drop days older than a year on load
static constexpr int kHeatWeeks = 12;
static constexpr int kKeepFileRows = 8; // per-file rows shown in the dialog

static ReadingStatsData gStats;
static int gPendingMs = 0;
static int gUnsavedSecs = 0;
static bool gStatsDirty = false;

static TempStr StatsFilePathTemp() {
    return path::JoinTemp(GetAppDataDirTemp(), StrL("reading-stats.tsv"));
}

static void LoadStatsIfEmpty() {
    if (len(gStats.dayIndex) > 0 || len(gStats.fileNames) > 0 || gStats.lifetimeSeconds > 0) {
        return;
    }
    TempStr path = StatsFilePathTemp();
    Str data = file::ReadFile(path);
    if (len(data) > 0) {
        ReadingStatsParse(data, &gStats);
    }
    str::Free(data);
    // keep the file from growing forever: days outside the last year are
    // moved into lifetimeSeconds so Total remains cumulative.
    i64 today = ReadingStatsDayIndexFromUnix((i64)time(nullptr));
    for (int i = len(gStats.dayIndex); i > 0; i--) {
        int idx = i - 1;
        if (today - gStats.dayIndex[idx] > kKeepDays) {
            gStats.lifetimeSeconds += gStats.seconds[idx];
            gStatsDirty = true;
            VecRemoveAt(gStats.dayIndex, (int)idx);
            VecRemoveAt(gStats.seconds, (int)idx);
        }
    }
}

void ReadingStatsFlush() {
    if (gUnsavedSecs <= 0 && !gStatsDirty) {
        return;
    }
    str::Builder out;
    ReadingStatsSerialize(&gStats, &out);
    TempStr path = StatsFilePathTemp();
    Str content = str::Dup(ToStrTemp(out));
    TempStr tempCandidate = fmt("%s.tmp", path);
    TempStr tempPath = MakeUniqueFilePathTemp(tempCandidate);
    bool saved = dir::CreateForFile(path) && file::WriteFile(tempPath, content) &&
                 file::RenameReplace(path, tempPath);
    if (!saved) file::Delete(tempPath);
    str::Free(content);
    if (saved) { gUnsavedSecs = 0; gStatsDirty = false; }
    else logf("ReadingStatsFlush: failed to persist stats; retaining pending seconds\\n");
}

void ReadingStatsTick(MainWindow* win, int elapsedMs) {
    if (!IsMainWindowValidAndNotClosing(win) || win->wasMinimized || !win->gpuiWin || !win->gpuiWin->active) {
        return;
    }
    WindowTab* tab = ActiveTab(win);
    if (!tab) {
        return;
    }
    gPendingMs += elapsedMs;
    if (gPendingMs < kAccumMs) {
        return;
    }
    int secs = gPendingMs / kAccumMs;
    gPendingMs -= secs * kAccumMs;
    LoadStatsIfEmpty();
    i64 today = ReadingStatsDayIndexFromUnix((i64)time(nullptr));
    ReadingStatsAccumDay(&gStats, today, secs);
    ReadingStatsAccumFile(&gStats, tab->displayName, secs);
    gUnsavedSecs += secs;
    if (gUnsavedSecs >= kFlushSecs) {
        ReadingStatsFlush();
    }
}

// --- the dialog -------------------------------------------------------------

static const gp::Rgba kHeatCols[5] = {
    {0x22, 0x30, 0x4a, 255}, {0x27, 0x49, 0x6d, 255}, {0x2f, 0x6a, 0xa3, 255},
    {0x4d, 0xa3, 0xff, 255}, {0x8c, 0xc5, 0xff, 255},
};

static gp::El* StatsRow(gp::Ctx* cx, Str label, Str value) {
    const gp::Theme& th = gp::ThemeNow(cx->app);
    auto* row = gp::Div(cx->a)->FlexRow()->Gap((float)DpiScale(12))->ItemsCenter();
    row->Child(gp::TextEl(cx->a, GpuiDup(cx->a, label))->Font(14)->Fg(th.foreground)->Shrink0());
    row->Child(
        gp::TextEl(cx->a, GpuiDup(cx->a, value))->Font(14)->Bold()->Fg(ToGpui(ThemeWindowLinkColor()))->Shrink0());
    return row;
}

// one 12-week grid row per weekday: 7 rows of kHeatWeeks cells, oldest first
static gp::El* StatsHeatGrid(gp::Ctx* cx) {
    i64 today = ReadingStatsDayIndexFromUnix((i64)time(nullptr));
    i64 todayDow = (today + 4) % 7; // 1970-01-01 was a Thursday
    i64 gridStart = today - (kHeatWeeks * 7 - 1) - todayDow;
    float cell = (float)DpiScale(12);
    float gap = (float)DpiScale(2);

    auto* col = gp::Div(cx->a)->FlexCol()->Gap(gap);
    for (int wk = 0; wk < kHeatWeeks; wk++) {
        auto* row = gp::Div(cx->a)->FlexRow()->Gap(gap);
        for (int dow = 0; dow < 7; dow++) {
            i64 day = gridStart + wk * 7 + dow;
            int level = day <= today ? ReadingStatsHeatLevel(ReadingStatsDaySeconds(&gStats, day)) : 0;
            auto* c = gp::Div(cx->a)->W(cell)->H(cell)->Bg(kHeatCols[level]);
            int mins = ReadingStatsDaySeconds(&gStats, day) / 60;
            c->Tip(GpuiDup(cx->a, fmt("%lld min", (long long)mins)));
            row->Child(c);
        }
        col->Child(row);
    }
    return col;
}

// newest first, at most kKeepFileRows entries; indices of the biggest rows
static Vec<int> TopFileIdx() {
    Vec<int> idx;
    for (int i = 0; i < len(gStats.fileNames); i++) {
        VecAppend(idx, (int)i);
    }
    // insertion sort by seconds, descending; the list is small
    for (int i = 1; i < len(idx); i++) {
        int v = idx[i];
        int j = i;
        while (j > 0 && gStats.fileSeconds[idx[j - 1]] < gStats.fileSeconds[v]) {
            idx[j] = idx[j - 1];
            j--;
        }
        idx[j] = v;
    }
    while (len(idx) > kKeepFileRows) {
        VecRemoveAt(idx, (int)len(idx) - 1);
    }
    return idx;
}

static Str StatsWindowTitle() {
    return Tr("Reading Statistics");
}

static gp::El* StatsToolBuild(MainWindow* win, gp::Ctx* cx) {
    (void)win;
    const gp::Theme& th = gp::ThemeNow(cx->app);
    LoadStatsIfEmpty();
    i64 today = ReadingStatsDayIndexFromUnix((i64)time(nullptr));

    auto* panel =
        gp::Div(cx->a)->FlexCol()->Gap((float)DpiScale(8))->PadX((float)DpiScale(16))->PadY((float)DpiScale(16));
    panel->Child(gp::TextEl(cx->a, GpuiDup(cx->a, Tr("Time spent reading per day, like Moon+ Reader's statistics")))
                     ->Font(13)
                     ->Fg(th.foreground));

    TempStr todayStr = fmt("%d min", ReadingStatsDaySeconds(&gStats, today) / 60);
    panel->Child(StatsRow(cx, Tr("Today"), todayStr));
    TempStr streakStr = fmt("%d %s", ReadingStatsStreak(&gStats, today), Tr("day(s)"));
    panel->Child(StatsRow(cx, Tr("Streak"), streakStr));
    i64 total = ReadingStatsTotalSeconds(&gStats);
    TempStr totalStr = fmt("%lld h %lld min", (long long)(total / 3600), (long long)((total / 60) % 60));
    panel->Child(StatsRow(cx, Tr("Total (lifetime)"), totalStr));

    panel->Child(gp::TextEl(cx->a, GpuiDup(cx->a, Tr("Last 12 weeks")))->Font(13)->Bold()->Fg(th.foreground));
    panel->Child(StatsHeatGrid(cx));

    panel->Child(gp::TextEl(cx->a, GpuiDup(cx->a, Tr("Most read documents")))->Font(13)->Bold()->Fg(th.foreground));
    Vec<int> top = TopFileIdx();
    for (int i : top) {
        i64 secs = gStats.fileSeconds[i];
        TempStr line = fmt("%s — %lld min", gStats.fileNames[i], secs / 60);
        panel->Child(gp::TextEl(cx->a, GpuiDup(cx->a, line))->Font(13)->Fg(th.foreground)->Shrink0());
    }
    ReadingStatsFlush();
    return panel;
}

void ReadingStatsShowDialog(MainWindow* win) {
    if (!win || !ToolWindowsAvailable()) {
        return;
    }
    ToolWindowDesc desc;
    desc.name = "readingstats";
    desc.title = StatsWindowTitle;
    desc.frame = ToolWinFrame::Overlapped;
    desc.resize = ToolWinResize::Resizable;
    desc.build = StatsToolBuild;
    int dpi = std::max(AppShellWindowDpi(win), 96);
    Size outer{MulDiv(360, dpi, 96), MulDiv(480, dpi, 96)};
    ToolWindowOpen(desc, win, ToolWindowCenteredOuter(win, outer));
}
