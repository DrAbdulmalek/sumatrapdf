/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "gui/GpuiBridge.h"
#include "gui/UIModels.h"

#include "Settings.h"
#include "AppTools.h"
#include "base/GuessFileType.h"
#include "NotesExport.h"
#include "NotesExportCommon.h"

#include "DisplayMode.h"
#include "DocController.h"
#include <mupdf/pdf.h>
#include "EngineBase.h"
#include "EngineAll.h"
#include "EngineMupdf.h"
#include "DisplayModel.h"
#include "Annotation.h"
#include "AIChatTypes.h"
#include "base/File.h"
#include "WindowTab.h"
#include "MainWindow.h"
#include "ReadingAutoScrollCommon.h"
#include "SumatraDialogs.h"
#include "SumatraPDF.h"
#include "Notifications.h"
#include "Translations.h"

// Collects the annotations page by page (they come out page-sorted) and hands
// them to the pure Markdown builder, then saves through the same save dialog
// the other Manjaro-port commands use.

// ShowSavePathDialog hands back the SavePathArgs*; keep the built markdown
// alongside by embedding it first so the pointer doubles as our context
struct NotesSaveCtx {
    SavePathArgs spa;
    Str content; // owned
};

static void WriteNotesFile(MainWindow* win, Str path, Str content) {
    bool ok = dir::CreateForFile(path);
    if (ok) {
        ok = file::WriteFile(path, content);
    }
    if (!ok) {
        MessageBoxWarning(win, Tr("Failed to save notes file"));
        return;
    }
    ShowTemporaryNotification(win, Tr("Notes exported"), kNotif5SecsTimeOut);
}

static void OnNotesPathPicked(NotesSaveCtx* ctx, SavePathArgs* args) {
    (void)args;
    MainWindow* win = ctx->spa.win;
    TempStr path = str::DupTemp(ctx->spa.path);
    str::Free(ctx->spa.path);
    ctx->spa.path = {};
    bool cancelled = len(path) == 0;
    if (cancelled) {
        str::Free(ctx->content);
        delete ctx;
        return;
    }
    if (!IsMainWindowValidAndNotClosing(win)) {
        str::Free(ctx->content);
        delete ctx;
        return;
    }
    WriteNotesFile(win, path, ctx->content);
    str::Free(ctx->content);
    delete ctx;
}

// "doc.pdf" -> "doc-notes.md", next to the document; no extension -> append
static TempStr NotesSeedPathTemp(WindowTab* tab) {
    if (len(tab->filePath) == 0) {
        return path::JoinTemp(GetAppDataDirTemp(), StrL("notes.md"));
    }
    Str fileName = path::GetBaseNameTemp(tab->filePath);
    const char* dot = strrchr(fileName.s, '.');
    Str stem =
        (dot && dot != fileName.s) ? str::DupTemp(Str(fileName.s, (int)(dot - fileName.s))) : str::DupTemp(fileName);
    Str name = str::JoinTemp(stem, StrL("-notes.md"));
    return path::JoinTemp(path::GetDirTemp(tab->filePath), name);
}

// collects the annotations of every page; pages are walked in order so the
// items are page-sorted for the markdown builder
static bool CollectNotes(EngineMupdf* em, int pageCount, Vec<NoteItem>& notes) {
    for (int pageNo = 1; pageNo <= pageCount; pageNo++) {
        EngineMupdfLoadAnnotsForPage(em, pageNo);
        FzPageInfo* pi = em->GetFzPageInfoFast(pageNo);
        if (!pi) {
            continue;
        }
        for (Annotation* a : pi->annotations) {
            NoteItem item;
            item.pageNo = a->pageNo;
            item.kind = NoteKindFromAnnotationType((int)a->type);
            Str comment = Contents(a);
            item.comment = str::Dup(comment);
            // links and other non-verbal annotations without a comment are noise
            if (item.kind == kNoteKindOther && len(item.comment) == 0) {
                str::Free(item.comment);
                continue;
            }
            VecAppend(notes, item);
        }
    }
    return len(notes) > 0;
}

void ExportNotesMd(MainWindow* win) {
    if (!CanAccessDisk()) {
        ShowTemporaryNotification(win, Tr("Disk access not allowed"), kNotif5SecsTimeOut);
        return;
    }
    WindowTab* tab = ActiveTab(win);
    DisplayModel* dm = tab ? win->AsFixed() : nullptr;
    if (!dm) {
        MessageBoxWarning(win, Tr("Notes can be exported for PDF documents"));
        return;
    }
    EngineMupdf* em = AsEngineMupdf(dm->engine);
    if (!em) {
        MessageBoxWarning(win, Tr("Notes can be exported for PDF documents"));
        return;
    }

    Vec<NoteItem> notes;
    CollectNotes(em, dm->PageCount(), notes);
    str::Builder out;
    Str docName = len(tab->displayName) > 0 ? tab->displayName : path::GetBaseNameTemp(tab->filePath);
    BuildNotesMarkdown(notes, docName, (i64)time(nullptr), &out);
    NotesExportFreeItems(notes);

    auto* ctx = new NotesSaveCtx();
    ctx->spa.win = win;
    ctx->spa.title = str::Dup(Tr("Export Notes as Markdown..."));
    ctx->spa.initialPath = str::Dup(NotesSeedPathTemp(tab));
    ctx->spa.defExt = str::Dup(StrL(".md"));
    ctx->spa.noDefExt = true;
    ctx->content = str::Dup(ToStrTemp(out));
    ctx->spa.onDone = MkFunc1(OnNotesPathPicked, ctx);
    ShowSavePathDialog(&ctx->spa);
}
