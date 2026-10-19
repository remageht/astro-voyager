# AV false-positive submission kit

What this is: a ready-to-paste pack for reporting VirusTotal false positives
to antivirus vendors, and for tracking them release by release.

Current state (v3.8.3, 2026-10-19):

- File: astro-voyager.exe (2 456 064 bytes)
- SHA256: D0655F6603C99BE87E25FAD40E6C714E1DE1823AB20DC356DCF0C873454315E7
- VirusTotal: 2/71 detections, both generic labels on an unsigned binary:
  - MaxSecure — Trojan.Malware.300983.susgen
  - SecureAge — Malicious
  - Microsoft Defender, Kaspersky, ESET, BitDefender and ~67 other engines: clean
- Release: https://github.com/remageht/astro-voyager/releases/tag/v3.8.3

Long-term fix (in progress): a code signing certificate. Once releases are
Authenticode-signed (Publisher: <developer name>), most generic heuristics stop
firing entirely and SmartScreen stops treating the file as unknown-publisher.
Until then, per-vendor FP reports chip away at the 2/71.

## How to file a report

1. Download the release exe (`astro-voyager.exe` from the release page) and
   verify the SHA256 against the value above (`Get-FileHash astro-voyager.exe`).
2. Upload the file to https://www.virustotal.com once per release so vendors
   can rescan against a live sample; then use the vendor's own feedback form:
   - Microsoft Defender: https://www.microsoft.com/wdsi/filesubmission
     (currently clean - only needed if Defender starts flagging)
   - MaxSecure, SecureAge and others: open the file page on VirusTotal, click
     the engine name that flags the file - most vendors list a
     "report a false positive" link on the engine detail page.
3. Paste the template below (fill in the engine name). Attach the exe.

Vendor responses are typically silent but the signature catalogs usually
update within a few days to a couple of weeks. Track the outcome by re-uploading
the same file to VirusTotal after two weeks.

## Report template (EN)

---
Subject: False positive report - astro-voyager v3.8.3 (open-source space exploration demo)

Hello,

Your engine flagged the following file as [ENGINE-NAME-LABEL]:

- File: astro-voyager.exe
- SHA256: D0655F6603C99BE87E25FAD40E6C714E1DE1823AB20DC356DCF0C873454315E7
- Size: 2456064 bytes
- Detection name: [EXACT-LABEL-FROM-VIRUSTOTAL]
- VirusTotal page: https://www.virustotal.com/gui/file/D0655F6603C99BE87E25FAD40E6C714E1DE1823AB20DC356DCF0C873454315E7
- Source: official release asset, https://github.com/remageht/astro-voyager/releases/tag/v3.8.3

This is a false positive. astro-voyager is an open-source (MIT) space
exploration demo that renders black holes and quasars with OpenGL; the source
is fully auditable at https://github.com/remageht/astro-voyager. Microsoft
Defender, Kaspersky, ESET, BitDefender and ~67 other VirusTotal engines report
the file as clean. The file is currently unsigned; a code signing certificate
is planned for the next release.

Please whitelist the hash. Thank you.
---

## Report template (RU)

---
Тема: Ложное срабатывание - astro-voyager v3.8.3 (open-source)

Здравствуйте,

Ваш движок помечает файл astro-voyager.exe как [ТЕКСТ ДЕТЕКТА].

- SHA256: D0655F6603C99BE87E25FAD40E6C714E1DE1823AB20DC356DCF0C873454315E7
- Размер: 2456064 байт
- Страница на VirusTotal:
  https://www.virustotal.com/gui/file/D0655F6603C99BE87E25FAD40E6C714E1DE1823AB20DC356DCF0C873454315E7
- Источник: официальный релиз,
  https://github.com/remageht/astro-voyager/releases/tag/v3.8.3

Это ложное срабатывание. astro-voyager - открытая (MIT) демо-игра про
исследование космоса (чёрные дыры, квазары), исходники полностью открыты:
https://github.com/remageht/astro-voyager. Microsoft Defender, Kaspersky, ESET,
BitDefender и ещё ~67 движков VirusTotal считают файл чистым. Файл пока не
подписан кодовой подписью - сертификат будет в следующем релизе.

Просьба добавить хэш в белый список. Спасибо.
---

## Updating this kit per release

- Refresh the SHA256 and the release URL after every tagged release.
- Re-check VirusTotal; note the count here.
- After the first signed release, expect 0-1 detections; keep filing reports
  until clean.
