/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "base/Base.h"

#include "Annotation.h"
#include "NotesExportCommon.h"

// must be last to over-write assert()
#include "base/tests/UtAssert.h"

void NotesExport_UnitTests() {
    // kind mapping covers the annotation types users can create
    utassert(NoteKindFromAnnotationType((int)AnnotationType::Highlight) == kNoteKindHighlight);
    utassert(NoteKindFromAnnotationType((int)AnnotationType::Underline) == kNoteKindUnderline);
    utassert(NoteKindFromAnnotationType((int)AnnotationType::Ink) == kNoteKindInk);
    utassert(NoteKindFromAnnotationType((int)AnnotationType::Link) == kNoteKindOther);
    utassert(str::Eq(NoteKindNameTemp(kNoteKindHighlight), StrL("Highlight")));
    utassert(str::Eq(NoteKindNameTemp(kNoteKindOther), StrL("Annotation")));

    Vec<NoteItem> notes;
    NoteItem n1;
    n1.pageNo = 3;
    n1.kind = kNoteKindHighlight;
    n1.comment = str::Dup(StrL("important for the exam"));
    VecAppend(notes, n1);
    NoteItem n2;
    n2.pageNo = 3;
    n2.kind = kNoteKindText;
    n2.comment = str::Dup(StrL("check reference"));
    VecAppend(notes, n2);
    NoteItem n3;
    n3.pageNo = 7;
    n3.kind = kNoteKindInk;
    VecAppend(notes, n3);

    str::Builder out;
    BuildNotesMarkdown(notes, StrL("Orthobullets Pediatrics"), 1731744000, &out);
    Str md = ToStrTemp(out);

    utassert(str::Contains(md, StrL("# Notes — Orthobullets Pediatrics")));
    utassert(str::Contains(md, StrL("## Page 3")));
    utassert(str::Contains(md, StrL("## Page 7")));
    utassert(str::Contains(md, StrL("**Highlight**: important for the exam")));
    utassert(str::Contains(md, StrL("**Note**: check reference")));
    utassert(str::Contains(md, StrL("**Ink**")));
    utassert(!str::Contains(md, StrL("## Page 1")));
    // the collector walks pages in order, so page 3 comes before page 7
    utassert(str::Contains(md, StrL("## Page 3")));
    utassert(str::Contains(md, StrL("## Page 7")));

    // headings and comments must not create Markdown or HTML markup
    Vec<NoteItem> hostile;
    NoteItem hn;
    hn.pageNo = 1;
    hn.kind = kNoteKindText;
    hn.comment = str::Dup(StrL("**bold** [link](javascript:alert(1)) <script> & \\\"quote\\\""));
    VecAppend(hostile, hn);
    str::Builder hostileOut;
    BuildNotesMarkdown(hostile, StrL("# title [x](url)"), 0, &hostileOut);
    Str hostileMd = ToStrTemp(hostileOut);
    utassert(str::Contains(hostileMd, StrL("\\\\# title \\\\[x\\\\]\\\\(url\\\\)")));
    utassert(str::Contains(hostileMd, StrL("\\\\*\\\\*bold\\\\*\\\\*")));
    utassert(str::Contains(hostileMd, StrL("&lt;script&gt;")));
    utassert(!str::Contains(hostileMd, StrL("<script>")));
    NotesExportFreeItems(hostile);

    // empty list gets a clear marker instead of an empty file
    Vec<NoteItem> none;
    str::Builder out2;
    BuildNotesMarkdown(none, StrL("empty.pdf"), 0, &out2);
    utassert(str::Contains(ToStrTemp(out2), StrL("No annotations found")));

    // newlines in a comment must not break the markdown list format
    NoteItem n4;
    n4.pageNo = 1;
    n4.kind = kNoteKindText;
    n4.comment = str::Dup(StrL("line one\nline two"));
    VecAppend(none, n4);
    str::Builder out3;
    BuildNotesMarkdown(none, StrL("x.pdf"), 0, &out3);
    utassert(str::Contains(ToStrTemp(out3), StrL("line one line two")));

    NotesExportFreeItems(notes);
    NotesExportFreeItems(none);
}
