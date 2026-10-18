Astro Voyager - antivirus note (why 1-2 engines may flag the exe)
=================================================================

Astro Voyager is an open-source (MIT) space explorer:
  https://github.com/remageht/astro-voyager

The Windows build ships unsigned. A few generic antivirus engines
(notably MaxSecure "Trojan.Malware.300983.susgen" and SecureAge
"Malicious" on VirusTotal) heuristically flag small unsigned game
binaries. The other ~69 of 71 engines (including Microsoft Defender,
Kaspersky, ESET, BitDefender) report the file clean - a classic
false-positive profile.

How to verify the build:
  1. Compare the SHA256 in CHECKSUMS.txt with the hashes printed on
     the GitHub Release page (CI logs) - they must match exactly.
  2. Build from source yourself:
     cmake -S . -B build -DASTROVOYAGER_ENABLE_GL=ON

If your antivirus quarantines the file:
  * add an exclusion / restore from quarantine (Windows Security:
    Virus and threat protection -> Protection history), or
  * report the false positive to your AV vendor, or
  * use the VirusTotal page of this file and press "Report false
    positive" - such reports are how the generic engines clear their
    detection lists.

Roadmap: we plan to sign Windows releases with a code signing
certificate (follow the releases page).
