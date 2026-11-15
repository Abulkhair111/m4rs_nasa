# Packaging and Multiplayer Test

Goal: a Windows **Development** build (not Shipping, so `-log` and the `~` console exist) that a second PC without Unreal can run for a LAN test. Script: `Tools/Package/package.ps1` (written by agent PK, **not yet run**; run it with the editor closed).

## 1. Cyrillic project name: what is risky

Facts from the files:
- `C:\mcproj\проект.uproject` (junction `C:\mcproj` -> `...\Unreal Projects\проект`). Only one C++ module `MissionCrew` (Runtime). `Source/MissionCrew.Target.cs` (class `MissionCrewTarget`, Game) and `MissionCrewEditor.Target.cs` are ASCII, so `-target=MissionCrew` works regardless of the uproject name. Built editor binaries are already `UnrealEditor-MissionCrew.dll`.
- Engine: `A:\epic games\UE_5.8` (5.8.4, registry `HKCU\...\Builds\{2A3A5D7D-...}`), has `Engine/Source`.
- `DefaultEngine.ini` still has the legacy `ActiveGameNameRedirects ... NewGameName="/Script/проект"` (from the Blueprint template; harmless, the module is `/Script/MissionCrew`).
- No `+MapsToCook` / `DirectoriesToAlwaysCook` set anywhere (Config has no `ProjectPackagingSettings`). Maps are reached only by string (`OpenLevel`, `ServerTravel`, the manager's `TransitMapName` = `/Game/MissionCrew/Maps/L_Transit`), so the cooker would NOT discover `L_Lobby`, `L_Mission`, `L_AssemblyBay`, `L_Transit` by itself. Only `GameDefaultMap` (`L_MainMenu`) is cooked automatically. The script passes `-map=` with every `.umap` in `Content/MissionCrew/Maps` (currently L_AssemblyBay, L_Lobby, L_MainMenu, L_Mission, L_Transit).

Risks of packaging with the Cyrillic uproject (from engine behaviour; not tried, the Architecture note already flagged it):
1. **`UE_PROJECT_NAME` / `GInternalProjectName`.** UBT passes the project name to the compiler as a `/D` define; a monolithic Game target stringises it into `GInternalProjectName`. The compiler reads response files in the ANSI codepage (we already hit this with the PCH path), so the name can be mangled to `?`/mojibake. The name then drives the content root (`../../../<name>/Content`), the pak names and `Saved` path of the packaged game. A wrong string = pak not mounted / content not found at runtime, and the failure is silent at build time. Editor (modular) builds were fine, which is why nobody noticed.
2. **Staging names.** UAT stages into `<Archive>/Windows/<ProjectName>/...` and names the paks `<ProjectName>-Windows.pak/.utoc/.ucas`. With a Cyrillic project name every one of these is Cyrillic, and the second PC's unzip tool / path / any ANSI tool may break. The exe is named after the Target (`MissionCrew.exe`) so that part is fine.
3. **UAT command lines.** `-project=C:\mcproj\проект.uproject` crosses PowerShell 5.1 -> cmd (`RunUAT.bat`) -> dotnet -> UnrealEditor-Cmd (cook). The `.bat` layer runs in an OEM codepage, so Cyrillic can be corrupted before UAT sees it. `-utf8output` only fixes the log encoding, not the arguments.
4. **cl.exe response files / PCH path** (the problem already seen): fine through the `C:\mcproj` junction (UBT does not resolve junctions), but only if every path UBT sees is the junction path. Intermediate and the PCH dirs sit under the project folder, so they must come from `C:\mcproj` too.
5. Third-party tools (Crash reporter, Windows Defender, zip tools) handling non-ASCII paths: minor.

### Recommendation
**Package from an ASCII shadow project** (script default `-Mode Shadow`):
- `C:\mcbuild\MissionCrew\MissionCrew.uproject` (ASCII name) plus `Content`, `Config`, `Source` mirrored with robocopy from `C:\mcproj`. Same Target.cs (name already `MissionCrew`), so exe = `MissionCrew.exe`, `UE_PROJECT_NAME=MissionCrew`, paks `MissionCrew-Windows.*`.
- Nothing in the real project is renamed, moved or built: its `Binaries/Intermediate/Saved` are untouched, so it can even be done while the editor is open apart from CPU load (the script still refuses to run with the editor open, override with `-AllowEditorOpen`).
- Side effects: it is a snapshot of what is SAVED on disk (save all assets, close the editor, then run); first run recompiles the game module and editor target in the shadow folder (a few minutes) and cooks everything (about 267 MB of content; shader compile is the slow part); the editor-only AI plugins (`ModelContextProtocol` has a Runtime module, plus the Toolsets, Python, EditorScriptingUtilities) are set to `TargetAllowList: ["Editor"]` in the shadow uproject only, so the packaged game does not carry or start them. If a Blueprint turns out to need one of them at runtime, remove it from `$editorOnly` in the script.
- Alternative (`-Mode Direct`): use the junction `C:\mcproj` + `-target=MissionCrew` straight on the Cyrillic uproject. It may work (the Target name is already ASCII), but items 1-3 stay open. Try it only if Shadow somehow fails; compare `GInternalProjectName` in the packaged log (`LogInit: Project Name:`).
- Permanent fix (user decision, not done): rename the uproject to `MissionCrew.uproject`. Not needed for this test.

## 2. Packaging steps
1. In the editor: **File > Save All**, check no unsaved Blueprints. Close the editor (and any `UnrealEditor*`, Live Coding console).
2. `powershell -ExecutionPolicy Bypass -File C:\mcproj\Tools\Package\package.ps1` (add `-DryRun` first to see the UAT command; params: `-ArchiveDir`, `-ShadowDir`, `-Mode`, `-CleanShadow`, `-SkipZip`, `-SkipBuild`, `-MinFreeGB`).
3. The script checks: editor not running, Visual Studio with MSVC (vswhere), >= 20 GB free on the archive and shadow drives (C: had about 45 GB free when this was written; build+cook+stage+zip want 15-20 GB), ASCII paths, map list.
4. It runs `RunUAT BuildCookRun -platform=Win64 -clientconfig=Development -target=MissionCrew -build -cook -map=<all maps> -stage -pak -iostore -compressed -prereqs -archive -archivedirectory=C:\mcpackage -utf8output`. Log: `C:\mcpackage\logs\package_<stamp>.log` (the engine's own UAT log is in `Engine\Programs\AutomationTool\Saved\Logs`).
5. Output: `C:\mcpackage\Windows\` (contains `MissionCrew.exe`, `MissionCrew\Binaries\Win64\MissionCrew.exe`, `MissionCrew\Content\Paks\*`, `Engine\`, helper files `Firewall_Setup.bat`, `Play_Windowed.bat`, `README_TEST.txt`) and the zip `C:\mcpackage\MissionCrew_Win64_Dev_<stamp>.zip`.
6. Expect, first run: 20-60 min.

### Project Settings to set (so a plain Package from the editor also works)
Do not edit these while another agent has the editor open (the editor rewrites the ini). In the editor: **Project Settings > Packaging**:
- **List of maps to include in a packaged build**: add `L_MainMenu`, `L_Lobby`, `L_Mission`, `L_AssemblyBay`, `L_Transit` (or, in `Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]` with `+MapsToCook=(FilePath="/Game/MissionCrew/Maps/L_Lobby")` etc., or `+DirectoriesToAlwaysCook=(Path="/Game/MissionCrew/Maps")`).
- **Additional Asset Directories to Cook** is a safer net for anything loaded by string/soft path (widgets by `LoadClass`, data assets by path): add `/Game/MissionCrew`. Without it, assets only referenced from C++ strings or the asset registry (`MCP_*` part defs are referenced by the manager's `Catalog` in the level, so they are fine) can be missing in the build. The script does NOT rely on this: it cooks the listed maps and everything they reference; if a runtime log says "Failed to find object / asset not found", add the folder here.
- **Build Configuration**: Development. **Use Pak File** + **Use Io Store** on, **Share Material Shader Code** default. **Include prerequisites** on.
- Not needed: Shipping, Nativize, `bGenerateChunks`.

## 3. Second PC checklist
1. Copy `MissionCrew_Win64_Dev_<stamp>.zip` (USB / shared folder), unzip to an ASCII path, e.g. `C:\MissionCrew_Test`.
2. Run `Firewall_Setup.bat` as Administrator (or the manual commands below). The first launch also shows the Windows Defender Firewall popup: allow on **Private and Public** (private LAN only is enough if the network profile is Private).
   ```
   netsh advfirewall firewall add rule name="MissionCrew UDP 7777" dir=in action=allow protocol=UDP localport=7777 profile=any
   netsh advfirewall firewall add rule name="MissionCrew LAN discovery" dir=in action=allow protocol=UDP localport=14000-14001 profile=any
   netsh advfirewall firewall add rule name="MissionCrew exe" dir=in action=allow program="C:\MissionCrew_Test\MissionCrew\Binaries\Win64\MissionCrew.exe" enable=yes profile=any
   ```
   (14000-14001 is the OnlineSubsystemNull LAN beacon used by the menu's LAN list; the port numbers are from memory of the engine source, verify in `LogOnline` if the list stays empty. Joining by IP only needs 7777.) Do it on **both** PCs; the host needs it most.
3. If the game exits at once with a missing VCRUNTIME/MSVCP/D3D message, run `Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe` (packaged with `-prereqs`). DirectX 12 capable GPU/driver is required (UE 5.8 default RHI).
4. Network: both PCs on the same subnet, network profile **Private**, no Wi-Fi client isolation (guest Wi-Fi blocks peer traffic), VPNs off. A cable or a phone hotspot with both PCs on it works.
5. Start with a console and a window: `MissionCrew.exe -log -windowed -ResX=1280 -ResY=720` (or `Play_Windowed.bat`). Press `~` for the in-game console in a Development build. Log file: `MissionCrew\Saved\Logs\MissionCrew.log` (ask for it after any failure).
6. Host's IP: on the host PC run `ipconfig`, take the IPv4 of the active adapter (e.g. 192.168.1.23); ignore 169.254.x.x and virtual adapters (Hyper-V, WSL, VPN).
7. Join: (a) the host presses **Host** in the main menu (`OpenLevel L_Lobby?listen`) and the client sees it in the **LAN list** and clicks Join; or (b) use the menu's IP join field (if present); or (c) on the client press `~` and type `open 192.168.1.23` (port 7777 is implied). Quick single-PC smoke test: `MissionCrew.exe 127.0.0.1 -log` against a host on the same machine.
8. If joining fails: client log search for `LogNet` / `Connection failed` / `Timed out`; check ping to the host, the host firewall rule, that both builds are the same zip (version mismatch = reject), and that the host really entered the Lobby (`listen` in the URL).

## 4. Multiplayer test checklist (what each player should see)

Flow: `L_MainMenu` -> Host / Join -> `L_Lobby` (roles) -> `L_Mission` / bay -> LAUNCH -> deck `L_Transit` (ServerTravel with `?listen`, non-seamless: clients reconnect automatically, expect a short black screen). The bay game mode `BP_MC_BayGameMode` is also used directly on `L_AssemblyBay`; note: that map is only reachable from the lobby flow if the lobby/mission travels there (check `Status.md`; otherwise open it from the console with `open L_AssemblyBay` on the host before the clients join, or start the host with `MissionCrew.exe L_AssemblyBay?listen -log`).

Menu / lobby
- [ ] Host: Host button works, no error line, lobby opens. Client: join by LAN list or `open <ip>` reaches the same lobby.
- [ ] Both callsigns visible; roles can be picked, unique per player; host START is available only to the host.
- [ ] No text in Cyrillic, no missing-font boxes.

Bay (`L_AssemblyBay`)
- [ ] Each player spawns at a different PlayerStart (4 starts tagged `Bay`), looks around with the mouse, WASD, Space, Tab cursor mode, P / Esc pause menu.
- [ ] Each player sees the other as the astronaut model (Z2 suit), standing on the floor (feet on the floor, no floating), with the role colour once the role is set (neutral grey in the bay test map without a lobby).
- [ ] Remote movement is smooth (no heavy jitter), names/aim prompts only for the local player.
- [ ] Host aims at a lander socket: prompt "[E] Choose a part ..."; E opens the picker; Confirm installs the part. **The client sees the part appear, the HUD mass/cost/T-W values, wall boards and the lander display change within a second.**
- [ ] Client does the same (E / picker / confirm / R remove); the host sees it. If the client's E does nothing: `ServerInteract`/`ServerRelease` are C++ RPCs now, but if the BP child `BP_MC_BayPlayer` overrides them check that Replicates / Reliable is set (see Status "Next").
- [ ] Both players installing different parts at once end with the same totals on both screens (no desync, no double count).
- [ ] Rocket stack on the pad and the wall display show the selected launcher on both PCs; cycling the launcher on one updates the other.
- [ ] Ready x4 / fuel / tools / LAUNCH (interactables are placed in the level only if the placement steps in `S1 Bay Overhaul - Plan` were done; if not, list what is missing instead of failing the test).
- [ ] LAUNCH red/yellow rules: red blocks launch, yellow needs a second press, both players see the same message.

Deck (`L_Transit`)
- [ ] After LAUNCH both PCs arrive on the deck map with the S1 loadout (subsystem result survived the travel): HUD top bar (stage, clock, fuel).
- [ ] Each player is seated at the station of their `CrewRole` (1 Power/Systems, 2 Construction/Equipment, 3 Propulsion/Pilot, 4 Science); the camera looks at the role's panel; the cursor is visible and buttons click.
- [ ] Each player sees only their own role panel; the other roles' station models show the other player seated (replicated `Occupant`).
- [ ] GO FOR LAUNCH poll: each role's flag turns on for everyone; after all 4 (missing seats can be debug-filled by the host) the countdown 10 s, ascent, coast, arrival run in sync (same stage and clock on both PCs within about 0.5 s).
- [ ] Actions (switches, step buttons, pilot nudges, science sliders/LOCK) change shared state for all players: alarms clear on both, displayed miss and fuel match.
- [ ] Arrival report with score visible to all; leaving/rejoining does not crash.

Always record: build zip name, host/client PC, a one-line result per item, and attach both `MissionCrew.log` files for any failure.

## 5. Not verified
Nothing here has been run: the script was parse-checked and written without running UAT because the editor was open. First real run may need fixes (plugin deny list, missing cooked assets, prerequisites). LAN beacon port numbers are from memory.
