# OCR language and tessdata configuration

SumatraPDF NG uses the bundled MuPDF PDF-OCR writer and the bundled Tesseract engine. OCR runs locally; the application does not download language models or send document contents to a service.

## Configure the language

Create or edit `ocr-settings.txt` in the application's data directory (the same directory as the main settings file):

```ini
language=eng
tessdata=/usr/share/tesseract-ocr/5/tessdata
```

Supported values:

- `eng` — English
- `ara` — Arabic
- `eng+ara` — bilingual English and Arabic OCR

The language defaults to `eng` if the file or `language` key is absent. `ara+eng` is accepted and normalized to `eng+ara`.

## Manage tessdata

The configured directory must contain one `.traineddata` file for every selected language, for example:

- `eng.traineddata`
- `ara.traineddata`

If `tessdata` is omitted, the application checks `SUMATRA_TESSDATA_PREFIX`, then `TESSDATA_PREFIX`, then common Linux locations (`/usr/share/tesseract-ocr/5/tessdata`, `/usr/share/tesseract-ocr/4.00/tessdata`, `/usr/share/tessdata`, `/usr/local/share/tessdata`) and finally `<app-data>/tessdata`.

Environment overrides:

- `SUMATRA_OCR_LANGUAGE` overrides the file's `language`.
- `SUMATRA_TESSDATA_PREFIX` overrides the file's `tessdata`.
- `TESSDATA_PREFIX` is used when neither an explicit config path nor `SUMATRA_TESSDATA_PREFIX` is set.

The configured path is validated before starting OCR. Missing traineddata is reported instead of silently falling back to English. Paths containing commas are currently rejected because MuPDF's writer options use comma-separated key/value pairs.

## Password-protected and modified PDFs

OCR and Flatten create an in-memory snapshot of the currently open PDF under the document locks. This snapshot includes unsaved annotations/edits and is serialized without encryption only in process memory so the worker can process an already-authenticated document without asking for its password again. The original document, its encryption, and its on-disk file are not changed by snapshot creation. MuPDF's PDF permission flags are checked first: OCR requires copy permission and Flatten requires modify permission. Restricted documents are rejected rather than silently bypassing their permission flags. The in-memory snapshot is released after the worker finishes.

No decrypted snapshot is written to disk. The generated OCR/Flatten output is a separate new PDF and is not re-encrypted with the source password by this implementation; the original protected source remains unchanged. Re-protect the derived output before sharing it if password protection is required.
