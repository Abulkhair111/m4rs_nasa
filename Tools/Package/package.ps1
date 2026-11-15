<#
.SYNOPSIS
  Package Mission Crew as a Windows Development build (logs + console) for a PC without Unreal.

.DESCRIPTION
  Run with the Unreal editor CLOSED:
      powershell -ExecutionPolicy Bypass -File C:\mcproj\Tools\Package\package.ps1
  Needs ~20+ GB free (build + cook + stage + zip). Typical time 20-60 min.

  Modes
    Shadow (default, recommended): the project name is Cyrillic. Packaging is done from an ASCII
        shadow project C:\mcbuild\MissionCrew (MissionCrew.uproject; Content/Config/Source mirrored
        with robocopy; editor-only MCP/tooling plugins limited to the Editor target). Nothing in the
        real project folder is renamed, moved or written (Binaries/Intermediate/Saved of the real
        project are not touched). Result: UE_PROJECT_NAME=MissionCrew, MissionCrew.exe,
        MissionCrew-Windows.pak/.utoc/.ucas, staged folder "MissionCrew".
    Direct: UAT is run straight on C:\mcproj\<Cyrillic>.uproject. Risky, see
        Vault/Packaging and Multiplayer Test.md section 1. Use only to try.

  The shadow copy is a SNAPSHOT of what is saved on disk. Save everything in the editor, then close it.

.PARAMETER ArchiveDir   Output root. Staged build ends in <ArchiveDir>\Windows, zip in <ArchiveDir>.
.PARAMETER Mode         Shadow (default) or Direct.
.PARAMETER ShadowDir    ASCII shadow project folder (Shadow mode).
.PARAMETER ProjectRoot  Folder with the .uproject (the junction C:\mcproj, never the Cyrillic path).
.PARAMETER EnginePath   Override engine root; default is read from the registry via EngineAssociation.
.PARAMETER MinFreeGB    Abort if a target drive has less free space.
.PARAMETER AllowEditorOpen  Skip the "editor must be closed" check (not recommended).
.PARAMETER SkipZip      Do not create the zip.
.PARAMETER SkipBuild    Pass no -build (reuse binaries already built in the shadow folder).
.PARAMETER CleanShadow  Delete the shadow Intermediate/Saved/Binaries first (forces full rebuild + recook).
.PARAMETER DryRun       Print the UAT command line and exit (after the shadow copy is prepared).
#>
[CmdletBinding()]
param(
    [string]$ArchiveDir  = 'C:\mcpackage',
    [ValidateSet('Shadow','Direct')][string]$Mode = 'Shadow',
    [string]$ShadowDir   = 'C:\mcbuild\MissionCrew',
    [string]$ProjectRoot = 'C:\mcproj',
    [string]$EnginePath  = '',
    [int]$MinFreeGB      = 20,
    [switch]$AllowEditorOpen,
    [switch]$SkipZip,
    [switch]$SkipBuild,
    [switch]$CleanShadow,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$ProjectName = 'MissionCrew'          # ASCII target / exe / staged-folder name
$Port        = 7777
$stamp       = Get-Date -Format 'yyyyMMdd_HHmmss'

function Info($m){ Write-Host "[package] $m" -ForegroundColor Cyan }
function Warn($m){ Write-Host "[package] WARNING: $m" -ForegroundColor Yellow }
function Fail($m){ Write-Host "[package] ERROR: $m" -ForegroundColor Red; exit 1 }

[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)

# ---------------------------------------------------------------- 1. locate the .uproject (wildcard, no Cyrillic literal)
$uprojFile = Get-ChildItem -Path $ProjectRoot -Filter '*.uproject' -File | Select-Object -First 1
if (-not $uprojFile) { Fail "No .uproject in $ProjectRoot" }
$uproject = $uprojFile.FullName
Info "Project file : $uproject"
$uj = Get-Content -Raw -Encoding UTF8 $uproject | ConvertFrom-Json

# ---------------------------------------------------------------- 2. engine
if (-not $EnginePath) {
    $assoc = $uj.EngineAssociation
    $val = $null
    try { $val = (Get-ItemProperty -Path 'HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds' -ErrorAction Stop).$assoc } catch {}
    if (-not $val) { try { $val = (Get-ItemProperty -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc" -ErrorAction Stop).InstalledDirectory } catch {} }
    if (-not $val) { Fail "Cannot resolve engine for EngineAssociation '$assoc'. Pass -EnginePath." }
    $EnginePath = ($val -replace '/', '\')
}
$uat = Join-Path $EnginePath 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path $uat)) { Fail "RunUAT.bat not found: $uat" }
Info "Engine       : $EnginePath"

# ---------------------------------------------------------------- 3. pre-checks
Info 'Pre-checks...'
if (-not $AllowEditorOpen) {
    $running = Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match '^(UnrealEditor|UnrealEditor-Cmd|UnrealBuildTool|ShaderCompileWorker)' }
    if ($running) { Fail ("Close the editor first. Running: " + (($running | Select-Object -ExpandProperty ProcessName -Unique) -join ', ')) }
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { Fail 'vswhere.exe not found: Visual Studio 2022 (MSVC + Windows SDK) is not installed.' }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { Fail 'Visual Studio found but without the MSVC x64 toolset (component VC.Tools.x86.x64).' }
Info "Visual Studio: $vs"
$checkPaths = @($ArchiveDir)
if ($Mode -eq 'Shadow') { $checkPaths += $ShadowDir } else { $checkPaths += $ProjectRoot }
foreach ($p in $checkPaths) {
    $drive = (Split-Path -Qualifier $p).TrimEnd(':')
    $free = [math]::Round((Get-PSDrive -Name $drive).Free / 1GB, 1)
    Info "Free on ${drive}: $free GB (need $MinFreeGB)"
    if ($free -lt $MinFreeGB) { Fail "Less than $MinFreeGB GB free on ${drive}:. Free space or change -ArchiveDir/-ShadowDir/-MinFreeGB." }
}
foreach ($p in @($ArchiveDir, $ShadowDir, $ProjectRoot)) { if ($p -match '[^\x00-\x7F]') { Fail "Path contains non-ASCII characters: $p" } }

# ---------------------------------------------------------------- 4. map list
$mapDir = Join-Path $ProjectRoot 'Content\MissionCrew\Maps'
$maps = Get-ChildItem -Path $mapDir -Filter '*.umap' -File | ForEach-Object { '/Game/MissionCrew/Maps/' + $_.BaseName } | Sort-Object
$required = 'L_MainMenu','L_Lobby','L_Mission','L_AssemblyBay','L_Transit'
foreach ($r in $required) {
    if ($maps -notcontains "/Game/MissionCrew/Maps/$r") { Warn "Map $r was expected but is not in $mapDir (the game travels to maps by name)." }
}
$strayMaps = Get-ChildItem -Path (Join-Path $ProjectRoot 'Content') -Filter '*.umap' -File -Recurse | Where-Object { $_.DirectoryName -ne $mapDir }
if ($strayMaps) { Warn ("Maps outside Maps/ are NOT cooked unless referenced: " + (($strayMaps | ForEach-Object Name) -join ', ')) }
if ($maps.Count -eq 0) { Fail 'No maps found.' }
$mapArg = '-map=' + ($maps -join '+')
Info "Maps         : $($maps -join ', ')"

# ---------------------------------------------------------------- 5. shadow project (ASCII name)
$buildProject = $uproject
if ($Mode -eq 'Shadow') {
    Info "Preparing ASCII shadow project: $ShadowDir"
    New-Item -ItemType Directory -Force -Path $ShadowDir | Out-Null
    if ($CleanShadow) {
        foreach ($d in 'Intermediate','Saved','Binaries') {
            $t = Join-Path $ShadowDir $d
            if (Test-Path $t) { Info "Cleaning $t"; Remove-Item -Recurse -Force $t }
        }
    }
    foreach ($d in 'Content','Config','Source','Plugins') {
        $src = Join-Path $ProjectRoot $d
        $dst = Join-Path $ShadowDir $d
        if (-not (Test-Path $src)) { continue }
        # /MIR mirrors; Developers/Collections are per-user editor data. robocopy exit codes < 8 are success.
        & robocopy $src $dst /MIR /XD Developers Collections /NFL /NDL /NJH /NJS /NP /R:2 /W:2 | Out-Null
        if ($LASTEXITCODE -ge 8) { Fail "robocopy failed for $d (exit $LASTEXITCODE)" }
    }
    $global:LASTEXITCODE = 0
    # Editor/AI-tooling plugins restricted to the Editor target so they are not packaged (ModelContextProtocol has a Runtime module).
    $editorOnly = 'ModelContextProtocol','MCPClientToolset','EditorToolset','UMGToolSet','ConfigSettingsToolset','PythonScriptPlugin','EditorScriptingUtilities'
    foreach ($pl in $uj.Plugins) {
        if (($editorOnly -contains $pl.Name) -and -not ($pl.PSObject.Properties.Name -contains 'TargetAllowList')) {
            $pl | Add-Member -NotePropertyName TargetAllowList -NotePropertyValue @('Editor')
        }
    }
    $buildProject = Join-Path $ShadowDir "$ProjectName.uproject"
    $json = ConvertTo-Json -InputObject $uj -Depth 12
    [System.IO.File]::WriteAllText($buildProject, $json, (New-Object System.Text.UTF8Encoding($false)))
    Info "Shadow uproject: $buildProject"
} else {
    Warn 'Direct mode: the Cyrillic project name goes into UE_PROJECT_NAME / GInternalProjectName. Expect problems; Shadow is recommended.'
}

# ---------------------------------------------------------------- 6. UAT BuildCookRun
$logDir = Join-Path $ArchiveDir 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$log = Join-Path $logDir "package_$stamp.log"

$uatArgs = @(
    'BuildCookRun',
    "-project=$buildProject",
    '-noP4', '-utf8output', '-unattended', '-nosplash',
    '-platform=Win64', '-targetplatform=Win64',
    '-clientconfig=Development', '-serverconfig=Development',   # Development, NOT Shipping: keeps -log and the console
    "-target=$ProjectName",
    '-cook', $mapArg, '-unversionedcookedcontent',
    '-stage', '-pak', '-iostore', '-compressed',
    '-prereqs',
    '-archive', "-archivedirectory=$ArchiveDir"
)
if (-not $SkipBuild) { $uatArgs += '-build' }
# Not used on purpose: -compile (only compiles the UAT scripts; not needed for an installed engine),
# -nocompileeditor (the cook needs the editor module built), -allmaps (would cook everything).

Info "UAT log      : $log"
Info ("UAT command  : `"$uat`" " + ($uatArgs -join ' '))
if ($DryRun) { Info 'DryRun: stopping here.'; exit 0 }

$sw = [Diagnostics.Stopwatch]::StartNew()
$ErrorActionPreference = 'Continue'   # PS 5.1 turns native stderr lines into errors; do not abort on them
& $uat @uatArgs 2>&1 | ForEach-Object { "$_" } | Tee-Object -FilePath $log
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
$sw.Stop()
Info ("UAT finished with exit code $code in {0:N1} min" -f $sw.Elapsed.TotalMinutes)
if ($code -ne 0) {
    Warn "Packaging failed. Search the log for 'Error:' / 'error C' / 'LogCook: Error':  $log"
    Warn ('Engine UAT log: ' + (Join-Path $EnginePath 'Engine\Programs\AutomationTool\Saved\Logs\Log.txt'))
    exit $code
}

# ---------------------------------------------------------------- 7. verify output
$stage  = Join-Path $ArchiveDir 'Windows'
$exe    = Join-Path $stage "$ProjectName.exe"
$exeBin = Join-Path $stage "$ProjectName\Binaries\Win64\$ProjectName.exe"
if (-not (Test-Path $exeBin)) { Fail "Expected $exeBin was not produced. See $log" }
$runExe = if (Test-Path $exe) { $exe } else { $exeBin }
Info "Packaged exe : $runExe"
$paks = Get-ChildItem -Path $stage -Recurse -Include '*.pak','*.utoc','*.ucas' | Select-Object -ExpandProperty Name
Info ("Pak files    : " + ($paks -join ', '))

# ---------------------------------------------------------------- 8. helper files for the test PC
$rel   = $runExe.Substring($stage.Length).TrimStart('\')
$fwExe = $exeBin.Substring($stage.Length).TrimStart('\')

$fw = @"
@echo off
rem Run as Administrator once on EVERY test PC. Opens the game port and LAN discovery for Mission Crew.
set "ROOT=%~dp0"
netsh advfirewall firewall delete rule name="MissionCrew UDP $Port" >nul 2>&1
netsh advfirewall firewall delete rule name="MissionCrew LAN discovery" >nul 2>&1
netsh advfirewall firewall delete rule name="MissionCrew exe" >nul 2>&1
netsh advfirewall firewall add rule name="MissionCrew UDP $Port" dir=in action=allow protocol=UDP localport=$Port profile=any
netsh advfirewall firewall add rule name="MissionCrew LAN discovery" dir=in action=allow protocol=UDP localport=14000-14001 profile=any
netsh advfirewall firewall add rule name="MissionCrew exe" dir=in action=allow program="%ROOT%$fwExe" enable=yes profile=any
echo Done. Also set the network profile to Private (Settings, Network, Properties) on both PCs.
pause
"@
Set-Content -Path (Join-Path $stage 'Firewall_Setup.bat') -Value $fw -Encoding ASCII

$play = @"
@echo off
rem Windowed 1280x720 with the log console. Edit ResX/ResY if you like.
start "" "%~dp0$rel" -log -windowed -ResX=1280 -ResY=720
"@
Set-Content -Path (Join-Path $stage 'Play_Windowed.bat') -Value $play -Encoding ASCII

$hostBay = @"
@echo off
rem Host straight into the assembly bay (skips the lobby). Other PCs: Join_Bay.bat <host-IP>
start "" "%~dp0$rel" /Game/MissionCrew/Maps/L_AssemblyBay?listen -log -windowed -ResX=1280 -ResY=720
"@
Set-Content -Path (Join-Path $stage 'Host_Bay.bat') -Value $hostBay -Encoding ASCII

$joinBay = @"
@echo off
if "%~1"=="" (echo Usage: Join_Bay.bat 192.168.1.23 & pause & exit /b 1)
start "" "%~dp0$rel" %~1:$Port -log -windowed -ResX=1280 -ResY=720
"@
Set-Content -Path (Join-Path $stage 'Join_Bay.bat') -Value $joinBay -Encoding ASCII

$readme = @"
MISSION CREW - multiplayer test build (Windows 64, Development)
Built: $stamp

FIRST TIME ON EACH PC
 1. Unzip anywhere with an ASCII path (for example C:\MissionCrew_Test). No Unreal needed.
 2. If the game complains about VCRUNTIME / MSVCP: run Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe once.
 3. Right-click Firewall_Setup.bat -> Run as administrator (opens UDP $Port and LAN discovery 14000-14001).
 4. Both PCs on the same network, network profile = Private. Guest Wi-Fi with client isolation will not work.

RUN
  $rel -log                                    (log console window; ~ opens the in-game console)
  $rel -log -windowed -ResX=1280 -ResY=720     (or double-click Play_Windowed.bat)

HOST (PC 1)
  Quick path (the lobby does not reach the bay yet): double-click Host_Bay.bat; on PC 2 run Join_Bay.bat <host IP>.
  Start the game, type a callsign, click HOST. Find your IP with  ipconfig  (IPv4 Address of the active adapter, e.g. 192.168.1.23).

JOIN (PC 2)
  Start the game. Pick the host in the LAN list (if it appears) or use the IP field of the menu,
  or press ~ and type:  open 192.168.1.23     (the host's real IPv4; default port $Port)
  Quick test on one PC:  open 127.0.0.1

LOGS
  <unzipped folder>\$ProjectName\Saved\Logs\$ProjectName.log   (send this file back after a crash or a failed join)
"@
Set-Content -Path (Join-Path $stage 'README_TEST.txt') -Value $readme -Encoding ASCII

# ---------------------------------------------------------------- 9. zip
$zip = $null
if (-not $SkipZip) {
    $zip = Join-Path $ArchiveDir "MissionCrew_Win64_Dev_$stamp.zip"
    Info "Zipping to $zip ..."
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    if (Test-Path $zip) { Remove-Item -Force $zip }
    [System.IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip, [System.IO.Compression.CompressionLevel]::Optimal, $false)
    Info ("Zip size     : {0:N0} MB" -f ((Get-Item $zip).Length / 1MB))
}

# ---------------------------------------------------------------- 10. what to do next
Write-Host ''
Write-Host '=============================== DONE ===============================' -ForegroundColor Green
Write-Host "Build folder : $stage"
if ($zip) { Write-Host "Zip for PC 2 : $zip" }
Write-Host "Log          : $log"
Write-Host ''
Write-Host 'On BOTH PCs (once, as Administrator): Firewall_Setup.bat, or manually:'
Write-Host "  netsh advfirewall firewall add rule name=`"MissionCrew UDP $Port`" dir=in action=allow protocol=UDP localport=$Port profile=any"
Write-Host "  netsh advfirewall firewall add rule name=`"MissionCrew exe`" dir=in action=allow program=`"<unzipped dir>\$fwExe`" enable=yes profile=any"
Write-Host ''
Write-Host 'HOST (PC 1):'
Write-Host "  $rel -log -windowed -ResX=1280 -ResY=720"
Write-Host '  main menu -> HOST.  ipconfig -> note the IPv4 address.'
Write-Host 'CLIENT (PC 2):'
Write-Host "  $rel -log -windowed -ResX=1280 -ResY=720"
Write-Host '  main menu -> LAN list / IP field, or press ~ and type:  open <host IPv4>'
Write-Host 'Single-PC check:'
Write-Host "  $rel 127.0.0.1 -log      (connects straight to a host on the same PC)"
