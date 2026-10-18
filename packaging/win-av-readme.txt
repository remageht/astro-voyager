Astro Voyager - antivirus / SmartScreen note
============================================

Astro Voyager is an open-source (MIT) space explorer:
  https://github.com/remageht/astro-voyager

The Windows build is **unsigned** (no code-signing certificate yet). Two
consequences, both expected for unsigned software:

1) SmartScreen: "Windows protected your PC - unknown publisher"
   This is not a virus report. The exe has no publisher signature and no
   download reputation yet. To run it:
     * More info  ->  Run anyway, or
     * unblock the file once: right-click the exe -> Properties ->
       "Unblock" (checkbox at the bottom) -> OK.
   The dialog disappears permanently once the release is code-signed.

2) VirusTotal: a few generic engines flag the file
   Typical: MaxSecure "Trojan.Malware.300983.susgen", SecureAge "Malicious",
   occasionally Bkav/Elastic heuristics. Microsoft Defender, Kaspersky, ESET,
   BitDefender and ~65-69 other engines report it clean - the classic
   false-positive profile of a small unsigned game binary.

Why it happens
  Heuristic engines flag unsigned, statically linked binaries with lots of
  embedded data (this game embeds its shaders and audio libs). Nothing in the
  game writes outside its own folders, opens network ports or installs
  anything - it is a single exe plus res/ and shaders/ folders.

How to verify the build yourself
  1. Compare SHA256 from CHECKSUMS.txt with the hash printed in the CI log of
     the release - they must match exactly.
  2. Or build from source (2 commands):
       cmake -S . -B build -DASTROVOYAGER_ENABLE_GL=ON
       cmake --build build --config Release

If your antivirus quarantines the file
  * restore it from quarantine (Windows Security -> Virus and threat
    protection -> Protection history) and add an exclusion, or
  * report the false positive to your vendor (for Microsoft:
    https://www.microsoft.com/en-us/wdsi/filesubmission), or
  * use the VirusTotal page of this exact file and press
    "Report false positive".

How the project is fixing this permanently
  * v3.8.1: icon + VERSIONINFO metadata embedded (no more "no version" flag).
  * v3.8.2: application manifest embedded (asInvoker, Win7..Win11 support,
    long paths) so the loader does not treat the exe as a legacy binary.
  * next: Authenticode signature on every release. packaging/sign-exe.ps1
    already signs + timestamps + verifies with signtool once a certificate
    (ideally EV, for immediate SmartScreen trust) is available.

False-positive submissions are tracked in docs/RELEASE.md (section 4.3).
