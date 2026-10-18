<#
  packaging/sign-exe.ps1 - Authenticode-sign a Windows build (the real fix for
  "Windows protected your PC" / SmartScreen and for most generic AV flags).

  The build ships unsigned by default: SmartScreen shows the "Unknown publisher"
  warning until the binary either (a) is signed with a certificate that has
  reputation, ideally an EV code-signing cert (instant trust), or (b) collects
  reputation over many downloads. Signing also clears most heuristic AV hits,
  because the file gains a verified publisher chain instead of "unknown".

  Usage (certificate file):
    pwsh -File packaging/sign-exe.ps1 -Exe dist/astro-voyager.exe `
         -PfxPath C:\certs\astro.pfx -PfxPassword $env:ASTRO_PFX_PASSWORD

  Usage (certificate already installed in the Windows store):
    pwsh -File packaging/sign-exe.ps1 -Exe dist/astro-voyager.exe `
         -Thumbprint 0123456789ABCDEF...

  CI: set WINDOWS_CERT_PFX_BASE64 and WINDOWS_CERT_PASSWORD secrets; the
  release workflow runs this script with -PfxPath after rebuilding the pfx.
#>
param(
  [Parameter(Mandatory = $true)][string]$Exe,
  [string]$PfxPath,
  [string]$PfxPassword,
  [string]$Thumbprint,
  [string]$TimestampUrl = 'http://timestamp.digicert.com'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Exe)) { throw "sign-exe: no such file: $Exe" }
if (-not $PfxPath -and -not $Thumbprint) {
  throw 'sign-exe: pass -PfxPath (+ -PfxPassword) or -Thumbprint'
}

# --- locate signtool.exe (Windows SDK; VS installer ships it) ---
$signtool = $null
$candidates = @(
  "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe",
  "${env:ProgramFiles}\Windows Kits\10\bin\*\x64\signtool.exe"
)
foreach ($pattern in $candidates) {
  $hit = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue |
         Sort-Object FullName -Descending | Select-Object -First 1
  if ($hit) { $signtool = $hit.FullName; break }
}
if (-not $signtool) {
  $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($cmd) { $signtool = $cmd.Source }
}
if (-not $signtool) {
  throw 'sign-exe: signtool.exe not found - install the Windows SDK (Signing Tools)'
}
Write-Host "sign-exe: signtool = $signtool"

$common = @('sign', '/fd', 'SHA256', '/td', 'SHA256', '/tr', $TimestampUrl,
            '/d', 'Astro Voyager - space explorer',
            '/du', 'https://github.com/remageht/astro-voyager')
if ($Thumbprint) {
  $common += @('/sha1', $Thumbprint)
} else {
  $common += @('/f', (Resolve-Path -LiteralPath $PfxPath).Path)
  if ($PfxPassword) { $common += @('/p', $PfxPassword) }
}
$common += $Exe

& $signtool @common
if ($LASTEXITCODE -ne 0) { throw "sign-exe: signtool sign failed ($LASTEXITCODE)" }

& $signtool verify /pa /v $Exe
if ($LASTEXITCODE -ne 0) { throw "sign-exe: signature verification failed ($LASTEXITCODE)" }

$sig = Get-AuthenticodeSignature -LiteralPath $Exe
Write-Host ("sign-exe: OK - status={0}, subject={1}" -f $sig.Status, $sig.SignerCertificate.Subject)
