/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "NotesExportCommon.h"

#include "Annotation.h"

int NoteKindFromAnnotationType(int annotationType) {
    switch ((AnnotationType)annotationType) {
        case AnnotationType::Highlight:
            return kNoteKindHighlight;
        case AnnotationType::Underline:
            return kNoteKindUnderline;
        case AnnotationType::Squiggly:
            return kNoteKindSquiggly;
        case AnnotationType::StrikeOut:
            return kNoteKindStrikeOut;
        case AnnotationType::Text:
        case AnnotationType::FreeText:
            return kNoteKindText;
        case AnnotationType::Ink:
            return kNoteKindInk;
        case AnnotationType::Stamp:
            return kNoteKindStamp;
        case AnnotationType::Caret:
            return kNoteKindCaret;
        default:
            return kNoteKindOther;
    }
}

TempStr NoteKindNameTemp(int kind) {
    switch (kind) {
        case kNoteKindHighlight:
            return StrL("Highlight");
        case kNoteKindUnderline:
            return StrL("Underline");
        case kNoteKindSquiggly:
            return StrL("Squiggly");
        case kNoteKindStrikeOut:
            return StrL("Strike-out");
        case kNoteKindText:
            return StrL("Note");
        case kNoteKindInk:
            return StrL("Ink");
        case kNoteKindStamp:
            return StrL("Stamp");
        case kNoteKindCaret:
            return StrL("Caret");
    }
    return StrL("Annotation");
}

static void MdAppendEscaped(str::Builder* out, Str s) {
    // Collapse line breaks, encode HTML delimiters, and escape CommonMark
    // punctuation so document titles/comments are emitted as literal text.
    constexpr Str punctuation = StrL("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    for (int i = 0; i < len(s); i++) {
        char c = s.s[i];
        if (c == '\r' || c == '\n' || c == '\t') {
            out->AppendChar(' ');
        } else if (c == '&') {
            out->Append(StrL("&amp;"));
        } else if (c == '<') {
            out->Append(StrL("&lt;"));
        } else if (c == '>') {
            out->Append(StrL("&gt;"));
        } else {
            bool escape = false;
            for (int j = 0; j < len(punctuation); j++) {
                if (punctuation.s[j] == c) { escape = true; break; }
            }
            if (escape) out->AppendChar('\\');
            out->AppendChar(c);
        }
    }
}

void BuildNotesMarkdown(const Vec<NoteItem>& notes, Str docName, i64 exportedOnUnix, str::Builder* out) {
    Str title = len(docName) > 0 ? docName : StrL("(no title)");
    out->Append(StrL("# Notes — "));
    MdAppendEscaped(out, title);
    out->Append(StrL("\n\n"));
    time_t t = (time_t)exportedOnUnix;
    struct tm lt;
#if OS_WIN
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    out->Append(fmt("Exported from SumatraPDF on %04d-%02d-%02d %02d:%02d.\n\n", lt.tm_year + 1900, lt.tm_mon + 1,
                    lt.tm_mday, lt.tm_hour, lt.tm_min));

    int curPage = -1;
    int count = 0;
    for (const NoteItem& n : notes) {
        if (n.pageNo != curPage) {
            curPage = n.pageNo;
            out->Append(fmt("## Page %d\n\n", curPage));
        }
        count++;
        out->Append(fmt("- **%s**", NoteKindNameTemp(n.kind)));
        if (len(n.comment) > 0) {
            out->Append(StrL(": "));
            MdAppendEscaped(out, n.comment);
        }
        out->Append(StrL("\n"));
    }
    if (count == 0) {
        out->Append(StrL("_No annotations found in this document._\n"));
    }
}

void NotesExportFreeItems(Vec<NoteItem>& notes) {
    for (NoteItem& n : notes) {
        str::Free(n.comment);
    }
    VecReset(notes);
}
