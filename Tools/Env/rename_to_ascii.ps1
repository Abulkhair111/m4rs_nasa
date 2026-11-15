<#
  Renames the Cyrillic project to ASCII: folder -> "MissionCrew", проект.uproject -> MissionCrew.uproject,
  re-points the C:\mcproj junction, fixes DefaultEngine.ini. Git history and the remote are untouched.
  Run with the editor CLOSED and no Claude agents building/editing. Use -DryRun first.
  After it: open C:\mcproj\MissionCrew.uproject (or the new folder), delete Intermediate\ if the build complains.
#>
param([switch]$DryRun)
$ErrorActionPreference = 'Stop'
$ru   = [string]::Join('', [char[]](0x043F,0x0440,0x043E,0x0435,0x043A,0x0442))   # "проект"
$base = Join-Path $env:USERPROFILE 'Documents\Unreal Projects'
$old  = Join-Path $base $ru
$new  = Join-Path $base 'MissionCrew'
function Step($m) { Write-Host ("[{0}] {1}" -f ($(if($DryRun){'dry'}else{'run'})), $m) }

if (-not (Test-Path $old)) { throw "Not found: $old (already renamed?)" }
if (Test-Path $new)        { throw "Target exists: $new" }
$busy = Get-Process | Where-Object { $_.ProcessName -match '^(UnrealEditor|UnrealBuildTool|dotnet|cl|link|Blender|blender)$' }
if ($busy) { throw ("Close these first: " + (($busy | Select-Object -Expand ProcessName -Unique) -join ', ')) }

$uproj = Join-Path $old ($ru + '.uproject')
Step "uproject  $uproj -> MissionCrew.uproject"
if (-not $DryRun) { Rename-Item $uproj 'MissionCrew.uproject' }

$ini = Join-Path $old 'Config\DefaultEngine.ini'
Step "fix ActiveGameNameRedirects in DefaultEngine.ini"
if (-not $DryRun) {
  (Get-Content $ini -Encoding UTF8) -replace ('/Script/' + $ru), '/Script/MissionCrew' | Set-Content $ini -Encoding UTF8
}

Step "folder    $old -> $new"
if (-not $DryRun) { Rename-Item $old 'MissionCrew' }

Step "junction  C:\mcproj -> $new"
if (-not $DryRun) {
  if (Test-Path 'C:\mcproj') { (Get-Item 'C:\mcproj').Delete() }
  New-Item -ItemType Junction -Path 'C:\mcproj' -Target $new | Out-Null
}
Step "done. New project file: $new\MissionCrew.uproject (build via C:\mcproj\MissionCrew.uproject; with ASCII names the junction is optional)"
