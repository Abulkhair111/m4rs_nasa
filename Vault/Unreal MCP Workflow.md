# Unreal MCP Workflow (how to drive the editor)

MCP server: `http://127.0.0.1:8000/mcp` (plugin ModelContextProtocol). The toolsets need these plugins in the .uproject: PythonScriptPlugin, EditorScriptingUtilities, EditorToolset, UMGToolSet, ConfigSettingsToolset. After enabling a plugin, **fully restart the editor**; `/mcp` reconnect isn't enough.

## Batch everything
Use `ProgrammaticToolset.execute_tool_script` (sandboxed Python; only json/math/re/time/copy/datetime) to chain many tool calls in one round trip.

## Gotchas
- **Update 2026-11-14: node IDs now come back in English** (e.g. `AddEvent|EventBeginPlay`, `Utilities|FlowControl|Branch`) and `write_graph_dsl` **works**. DSL event names drop the `AddEvent|` prefix: `(event EventBeginPlay …)`, `(event UserInterface|EventConstruct …)`, `(event Custom|MyEvent …)`, `(event OnClicked(SkipButton) …)`. Call a custom event with `(CallFunction|MyEvent)`. Var get/set IDs drop the `b` prefix (`Variables|Default|SetFinished` for `bFinished`). DSL reuses existing event nodes. It can't express delegate binding, so add `BindEventto<X>` / `Assign<X>` nodes by hand. If the editor language flips back, the note below applies.
- `set_pin_value` does **not** mark the package dirty, so `save_assets` silently skips it. Force-dirty it (add and remove a temp variable), then save, and verify on disk.
- While **PIE runs**, asset tools fail ("Asset does not exist" / "Editor is in play mode") and levels can't load. Stop PIE first.
- `find_node_types` in another BP can stay stale after you add a new event to a widget (the new function isn't listed even after compile and save). Work around it inside the widget.
- `RemoveFromParent` inside a widget's Construct does nothing when Construct runs during `AddToViewport`. Defer with Delay 0.
- FileMediaSource `./` paths resolve against the **project** folder in 5.8 (use `./Content/Movies/x.mp4`). OpenSource on a missing file returns false **without** firing OnMediaOpenFailed.
- Media assets (MediaPlayer/MediaTexture/FileMediaSource) have no MCP factory. Create them with the commandlet (`Tools/Media/create_mission_video_assets.py`); the editor picks up the new files live.
- **DSL lessons (menu/art, construction phase):**
  - `write_graph_dsl` on a **function** graph adds new nodes but leaves the old ones as orphans (sometimes still wired by data pins, which causes compile errors). After rewriting, delete nodes not reachable from entry points (exec-forward + data-backward walk).
  - An impure call used only inside `(return …)` gets pruned ("Exec pin is not connected"). `bind` it first, then return.
  - Positional args: function call = target first (`(Class|X|Func target a b)`); variable **setter** = value first (`(Class|X|SetVar value target)`); calling your own function with only params needs keywords (`(CallFunction|Tri :Good a :Ok b)`), otherwise arg 1 goes to `self`. Same for self-target engine nodes (`AddControllerYawInput :Val x`, `SetActorLocationAndRotation :NewLocation …`).
  - `(if (Utilities|IsValid x) …)` resolves to the **macro** and drops the body. Use the multi-exec form or a `Valid(Obj)→bool` helper function.
  - `MaxOfIntArray` returns (IndexOfMaxValue, MaxValue): `(bind (idx mx) …)`.
  - `if` without else falls through to the next statement; `for` continues after the loop.
  - Multi-output binds work: `(bind (hit ok) (Collision|LineTraceByChannel …))`, `(bind (r g b a) (Utilities|Struct|BreakLinearColor c))`.
  - Ambiguous IDs (`Appearance|SetColorandOpacity`, `Rendering|Material|SetVectorParameterValue`) pick the wrong class. Use `create_node` with `declaring_class` and wire it by hand, or pick another node.
  - Widget variables: `Variables|<WBPName>|Get<Widget>`; external BP vars `Class|<BPNameNoUnderscore>|Get<Var>`. Bool vars drop the `b`.
  - Enhanced Input: poll `Input|EnhancedActionValues|IA_X` in Tick (pure). Event nodes have five exec outputs, which DSL can't express well.
  - Custom-event **replication flags can't be set** via MCP (FunctionFlags isn't exposed). Tick "Run on Server" by hand.
  - `compile_blueprint` returns null even on error. Grep the log for `LogBlueprint: Error` / `[Compiler]`.
  - `get_node_type_pins` is safe (no lasting nodes).
  - **Rewriting a function:** don't remove and re-add it in one call, because the new graph gets a `_0` suffix. Instead delete every node except `FunctionEntry`/`FunctionResult` (find_nodes + delete_node), then `write_graph_dsl`. To replace an event body, use `get_connected_subgraph(event)` and delete everything except the event node.
  - `write_graph_dsl` throws if the BP has **any** compile error, even one in another graph, but it has already written the nodes. Check node counts before retrying.
  - `try/except` inside `execute_tool_script` does not catch tool errors (the whole script aborts). There's no `exec` in the sandbox. For long DSL, generate a JSON file locally with the engine's Python (`Engine/Binaries/ThirdParty/Python3/Win64/python.exe -I gen.py`) into `Saved/`, then `AssetTools.read_file` it in the script.
  - A BP can't read **another instance of its own class's** variables via DSL (`Class|BPPart|GetX` doesn't exist inside BP_Part, and `Variables|Default|GetX` has no target pin). Put that logic in another class (we used `BP_ConstructionManager.ApplyTool`).
  - Pure nodes are re-evaluated per consumer: `(bind nb (not (GetBolted s)))` then `SetBolted nb` makes `nb` flip for later uses. Read the new state back after a setter.
  - DSL `(bind x false)` (literal) is an error. Integer modulo is `Math|Integer|%(Integer)`. Own-function node type in create_node is `CallFunction|Name`. `SelectInt`/`SelectFloat` evaluate both inputs, so an array `Get` behind them still warns about out-of-range indices. Give arrays correct default lengths.
  - `ObjectTools.set_properties` on an array can't change its length and values in one call: keep the existing values and append.
  - `StartPIE` needs `options` (`bSimulate`, `playMode`, `warmupSeconds`). `CaptureViewport` needs `annotations` (pass zeros). `AssetTools` auto-clean sweeps that include shared BPs may be blocked by the permission classifier; keep them to your own folders.
- 3D widgets in a dim room render washed out (auto-exposure gain on the unlit widget). Keep the widget background transparent (Masked) over a dark mesh.
- `AssetTools.write_file` takes absolute paths under Content/Saved only, extensions .txt/.json/.md/.csv/.py/.html (handy for dumping capture base64).
- **The editor is Russian, so node type IDs are localized.** `write_graph_dsl` fails (it hardcodes `AddEvent|`) and `read_graph_dsl` returns empty. Build graphs with `create_node` + `get_node_infos` (pin names) + `connect_pins` + `set_pin_value`; use `add_event` for events/overrides.
  Known IDs: Branch = `Утилиты|Контрольисполнения|Ответвление`, CreateWidget = `Пользовательскийинтерфейс|Создатьвиджет`, Self = `Переменные|Получитьссылкунасебя`, OpenLevel = `Игра|OpenLevel(byName)`, member var set = `Переменные|Поумолчанию|Задать<Var>`, external var = `Класс|<BPNameNoUnderscores>|Получить<Var>`. Find others with `find_node_types(graph, filter, context_pins=[])`.
- UMG: `CreateWidgetBlueprint`, `AddWidget`, then ObjectTools set_properties on widget/slot. Bind clicks with `BindToEventProperty` (creates `OnClicked(<Name>)` bound event nodes).
- ObjectTools JSON keys: struct members use **lowerCamel** (`autoExposureBias`) while `bOverride_*` keep their case. A wrong key is **silently ignored**, so read it back to verify.
- `ActorTools.set_actor_transform` resets any field you omit (scale → 1). Always pass the full transform.
- No level-creation tool: duplicate `/Engine/Maps/Templates/Template_Default`, `save_assets`, then `load_level`.
- Import: `StaticMeshTools.import_file`, `TextureTools.import_file`. Complex glTF went through a commandlet: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<py>` with Interchange (deprecated properties raise errors; use `combine_static_meshes_behavior = ALL`).
- To put a mesh on uneven ground: move it away, `SceneTools.trace_world` down at its wheel/foot points, fit pitch/roll, then place it. UE rotator: +pitch raises local +X; +roll lowers local +Y.
- Engine BasicShapes spheres are low-poly. Scaled up, their facets visibly bend the ground, so use a Plane for flat ground.
- `CaptureViewport` / `CaptureEditorImage` return large base64 data. Decode it to PNG in the scratchpad and read a downscaled copy.
- StartPIE often reports "PIE ended before warmup" while PIE is in fact running. Check with `IsPIERunning`.
- **Concurrency:** another agent may use the same MCP server. Check `Saved/Logs/проект.log` for `Dispatching toolset tool` lines that aren't yours before editing shared Blueprints.
- If saving fails with 'file is read-only': Git LFS `lockable` makes files read-only. It has been removed from .gitattributes; clear the flag with `attrib -R`.
