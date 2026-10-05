# DREAMDAW

One instrument VST3. FL Studio loads this. Each instance keeps its own WebView alive after the plugin window is closed, so piano-rack MIDI and the HTML instrument keep running.

Site package: https://github.com/keganacummings-source/Site

## What changed (2026-10-04)

- The WebView lives on the processor, not the editor. Closing or minimizing the FL plugin window reparents it to an off-screen host instead of destroying it.
- MIDI is pumped from the processor timer, so piano rack and MIDI keyboards work with the UI closed.
- Each instance gets its own WebView2 profile (`%APPDATA%\DreamDAW\instances\<id>`). A shared profile was locking the second instance and mixing logins.
- Machine URL is saved in the plugin state, not a single global file.
- WebView2 is started with background-throttling disabled. The injected host script resumes AudioContext and does not drop notes when the page is hidden.
- DreamAXE now exposes `noteOn` / `noteOff` and plays the MIDI note pitch (FL piano rack).
- Effect-only and non-instrument HTML removed from this build, including DREAMABC.

## Build

Push this folder to GitHub. `.github/workflows/main.yml` produces the Windows VST3.

Local:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows output: `build/DreamDAW_artefacts/Release/VST3/DREAMDAW.vst3`

Copy that bundle to `C:\Program Files\Common Files\VST3` and rescan in FL Studio. WebView2 runtime is required.

Deploy the Site tree (library.json, index.html, Pluggins Folder) to GitHub Pages / dreamdaw.com so the selector matches this build.


## 1.1.0 update
- Site-backed instrument presets are provided by DREAMDAW's authenticated DreamAPI.
- Presets are saved per account and instrument, so they survive new FL/VST instances.
- DREAMAPI presence is shown by the embedded site and uses the live `presence` response.
- VST editor cleaned up with a larger responsive canvas and clearer DREAMDAW navigation.
