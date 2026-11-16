/* Copyright 2026 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

struct MainWindow;

// export the annotations of the current document to Markdown (Foxit's
// "comment data exporting" equivalent for the ng port)
void ExportNotesMd(MainWindow* win);
