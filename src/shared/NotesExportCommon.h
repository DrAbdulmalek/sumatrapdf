/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// Export the annotations of the current document as a Markdown file, in the
// spirit of Foxit's "comment data exporting" and Moon+ Reader's notes share.
// Pure markdown building here; the ng front end collects the items and saves.

// 0 highlight, 1 underline, 2 squiggly, 3 strikeout, 4 text note / free text,
// 5 ink, 6 stamp, 7 caret, 8 other
constexpr int kNoteKindHighlight = 0;
constexpr int kNoteKindUnderline = 1;
constexpr int kNoteKindSquiggly = 2;
constexpr int kNoteKindStrikeOut = 3;
constexpr int kNoteKindText = 4;
constexpr int kNoteKindInk = 5;
constexpr int kNoteKindStamp = 6;
constexpr int kNoteKindCaret = 7;
constexpr int kNoteKindOther = 8;

int NoteKindFromAnnotationType(int annotationType);
TempStr NoteKindNameTemp(int kind);

struct NoteItem {
    int pageNo = 0;
    int kind = kNoteKindOther;
    // contents of the annotation; owned (str::Dup) or null
    Str comment;
};

// builds Markdown grouped by page; items must already be page-sorted
// (the ng collector walks pages in order). Date line: exportedOnUnix.
void BuildNotesMarkdown(const Vec<NoteItem>& notes, Str docName, i64 exportedOnUnix, str::Builder* out);
void NotesExportFreeItems(Vec<NoteItem>& notes);
