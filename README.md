# DREAMDAW

One VST3. FL Studio loads this. The user logs in, picks a machine from the **Plugin selector**, and the WebView opens that HTML file from the site. DSP stays in the HTML. Do not port it.

Site package: https://github.com/keganacummings-source/Site

Machines live in `Pluggins Folder/*.html`. The selector reads `library.json`. Login uses the same worker DreamShare Lite uses (`action: "login"`, fields `user` and `pass`, token comes back).

## UI (optimized)

- Native chrome: only a **Homescreen** button (returns to the selector).
- Embedded shell: login bar + **Plugin selector** grid. Nothing else.
- SITE FOLDER button and related local-folder tooling removed from the editor.

## Boot path

1. FL Studio scans `DREAMDAW.vst3` (instrument, stereo out, MIDI in).
2. Editor opens the room (bundled HTML shell — plugin selector).
3. Log in. Session is written to `%APPDATA%\DreamDAW\session.json`.
4. Click a machine. WebView navigates to `https://www.dreamdaw.com/Pluggins%20Folder/<file>`.
5. Homescreen returns to the selector. The last machine is remembered.

v0.1 loads the live site URL so relative sample paths keep working.

## Build

Push this folder to GitHub. `.github/workflows/build.yml` produces the Windows / macOS VST3 artifacts. Same JUCE 8.0.6 fetch as DreamShare Lite.

Local:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows output: `build/DreamDAW_artefacts/Release/VST3/DREAMDAW.vst3`

Copy that bundle to `C:\Program Files\Common Files\VST3` and rescan in FL Studio. WebView2 runtime is required (already on Windows 11).

JUCE is AGPL or commercial. This repo is the source that goes with the binary.

## What an HTML edit has to expose

The injected user script already forwards FL MIDI to a page-level `noteOn(midi, vel)` / `noteOff(midi)`. BetaDREAMSINE.html already has those. Other machines should add the same two names. No other C++ change.

Hearing the machine inside FL (not only in the plugin window) needs the page to push blocks. That is the next hook, not a DSP rewrite.

## Optimized package notes (2026-10-04)

- No Insomnia / INXOMNIA in the plugin selector (removed from library.json and Pluggins Folder).
- Site download includes DreamShareVst.zip under DownloadVSTFile/Two/ so users can still get the classic DreamShare Lite VST3.
- Absurd GPT/validation helper files stripped from the deploy tree.
- This VST shows only the login + Plugin selector screen; machines load live from the site.
