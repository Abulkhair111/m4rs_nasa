# Unreal MCP Workflow (how to drive the editor)

MCP server: `http://127.0.0.1:8000/mcp` (plugin ModelContextProtocol). The toolsets need these plugins in the .uproject: PythonScriptPlugin, EditorScriptingUtilities, EditorToolset, UMGToolSet, ConfigSettingsToolset. After enabling a plugin, **fully restart the editor**; `/mcp` reconnect isn't enough.

## Batch everything
Use `ProgrammaticToolset.execute_tool_script` (sandboxed Python; only json/math/re/time/copy/datetime) to chain many tool calls in one round trip.

## Gotchas
- **The editor is Russian, so node type IDs are localized.** `write_graph_dsl` fails (it hardcodes `AddEvent|`) and `read_graph_dsl` returns empty. Build graphs with `create_node` + `get_node_infos` (pin names) + `connect_pins` + `set_pin_value`; use `add_event` for events/overrides.
  Known IDs: Branch = `Утилиты|Контрольисполнения|Ответвление`, CreateWidget = `Пользовательскийинтерфейс|Создатьвиджет`, Self = `Переменные|Получитьссылкунасебя`, OpenLevel = `Игра|OpenLevel(byName)`, member var set = `Переменные|Поумолчанию|Задать<Var>`, external var = `Класс|<BPNameNoUnderscores>|Получить<Var>`. Find others with `find_node_types(graph, filter, context_pins=[])`.
- UMG: `CreateWidgetBlueprint`, `AddWidget`, then ObjectTools set_properties on widget/slot. Bind clicks with `BindToEventProperty` (creates `OnClicked(<Name>)` bound event nodes).
- ObjectTools JSON keys: struct members use **lowerCamel** (`autoExposureBias`) while `bOverride_*` keep their case. A wrong key is **silently ignored**, so read it back to verify.
- `ActorTools.set_actor_transform` resets any field you omit (scale → 1). Always pass the full transform.
- No level-creation tool: duplicate `/Engine/Maps/Templates/Template_Default`, `save_assets`, then `load_level`.
- Import: `StaticMeshTools.import_file`, `TextureTools.import_file`. Complex glTF went through a commandlet: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<py>` with Interchange (deprecated properties raise errors; use `combine_static_meshes_behavior = ALL`).
- Engine BasicShapes spheres are low-poly. Scaled up, their facets visibly bend the ground, so use a Plane for flat ground.
- `CaptureViewport` / `CaptureEditorImage` return large base64 data. Decode it to PNG in the scratchpad and read a downscaled copy.
- StartPIE often reports "PIE ended before warmup" while PIE is in fact running. Check with `IsPIERunning`.
- **Concurrency:** another agent may use the same MCP server. Check `Saved/Logs/проект.log` for `Dispatching toolset tool` lines that aren't yours before editing shared Blueprints.
