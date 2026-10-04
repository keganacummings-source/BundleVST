#include "PluginEditor.h"

// Minimal shell: login + plugin selector only. No site-folder tooling, no extra chrome.
static const char* kShell = R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>DREAMDAW</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:#0e0608;color:#f4e4ea;font:14px/1.45 ui-sans-serif,system-ui,sans-serif;min-height:100vh}
header{display:flex;align-items:center;gap:12px;padding:12px 16px;border-bottom:1px solid #341820}
h1{font-size:15px;letter-spacing:.2em;margin:0;font-weight:600}
#who{margin-left:auto;color:#9a7884;font-size:12px}
#login{display:flex;flex-wrap:wrap;gap:8px;align-items:center;padding:12px 16px;border-bottom:1px solid #341820}
#login input{flex:1;min-width:120px;background:#1a0c12;color:#f4e4ea;border:1px solid #341820;border-radius:6px;padding:8px 10px}
#login button{background:#c04068;border:1px solid #c04068;color:#fff;border-radius:6px;padding:8px 14px;cursor:pointer;font-weight:600}
#err{color:#e07090;font-size:12px;width:100%}
main{padding:16px}
.section-title{font-size:11px;letter-spacing:.15em;text-transform:uppercase;color:#9a7884;margin:0 0 10px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(160px,1fr));gap:8px}
.card{text-align:left;background:#1a0c12;border:1px solid #341820;color:#f4e4ea;border-radius:8px;padding:12px;cursor:pointer;transition:border-color .15s,background .15s}
.card:hover{border-color:#c04068;background:#221018}
.card strong{display:block;font-size:13px;margin-bottom:4px}
.card small{display:block;color:#9a7884;font-size:11px;word-break:break-all}
#status{color:#9a7884;font-size:12px;padding:8px 0}
</style></head>
<body>
<header>
  <h1>DREAMDAW</h1>
  <span id="who">signed out</span>
</header>
<form id="login">
  <input id="user" placeholder="user" autocomplete="username">
  <input id="pass" placeholder="pass" type="password" autocomplete="current-password">
  <button type="submit">Log in</button>
  <span id="err"></span>
</form>
<main>
  <p class="section-title">Plugin selector</p>
  <div class="grid" id="grid"><div id="status">loading machines…</div></div>
</main>
<script>
const API = "https://dreamshare-api.keganacummings.workers.dev/";
const SITE = "https://www.dreamdaw.com/";

function native(name, payload){
  try {
    if (window.__JUCE__ && window.__JUCE__.backend && window.__JUCE__.backend.emitEvent)
      return window.__JUCE__.backend.emitEvent(name, payload || {});
  } catch (e) {}
  return Promise.resolve(null);
}

async function login(ev){
  ev.preventDefault();
  const user = document.getElementById("user").value.trim();
  const pass = document.getElementById("pass").value;
  const err = document.getElementById("err");
  err.textContent = "";
  try {
    const res = await fetch(API, {
      method: "POST",
      headers: {"content-type": "application/json"},
      body: JSON.stringify({action: "login", user, pass})
    });
    const data = await res.json();
    if (!data.ok) {
      err.textContent = data.error || "login failed";
      return;
    }
    localStorage.setItem("dreamdaw.token", data.token);
    localStorage.setItem("dreamdaw.user", data.user);
    document.getElementById("who").textContent = data.user + " · " + (data.role || "user");
    native("setSession", {user: data.user, token: data.token, role: data.role || "user"});
  } catch (e) {
    err.textContent = String(e);
  }
}

document.getElementById("login").onsubmit = login;

const saved = localStorage.getItem("dreamdaw.user");
if (saved) document.getElementById("who").textContent = saved;

fetch(SITE + "library.json")
  .then(r => r.json())
  .then(lib => {
    const grid = document.getElementById("grid");
    grid.innerHTML = "";
    const list = lib.plugins || lib.machines || [];
    if (!list.length) {
      grid.innerHTML = "<div id=\"status\">no machines in library.json</div>";
      return;
    }
    list.forEach(p => {
      const name = p.name || p.title || "unnamed";
      const file = p.fileName || p.file || "";
      const rel = p.url || ("Pluggins Folder/" + file);
      const b = document.createElement("button");
      b.className = "card";
      b.type = "button";
      b.innerHTML = "<strong>" + name + "</strong><small>" + file + (p.beta ? " · beta" : "") + "</small>";
      b.onclick = () => {
        const url = SITE + String(rel).replace(/ /g, "%20");
        native("openMachine", {url: url, name: name});
      };
      grid.appendChild(b);
    });
  })
  .catch(e => {
    document.getElementById("grid").innerHTML = "<div id=\"status\">" + String(e) + "</div>";
  });
</script>
</body></html>
)HTML";

DreamDAWEditor::DreamDAWEditor(DreamDAWProcessor& p)
    : AudioProcessorEditor(p),
      proc(p),
      browser(juce::WebBrowserComponent::Options{}
                 .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                 .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                             .withUserDataFolder(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                                                     .getChildFile("DreamDAW")
                                                                     .getChildFile("WebView2")))
                 .withNativeIntegrationEnabled()
                 .withResourceProvider([this](const auto& url) { return serve(url); })
                 .withEventListener("openMachine",
                                    [this](const juce::var& payload)
                                    {
                                        auto* obj = payload.getDynamicObject();
                                        if (obj != nullptr)
                                        {
                                            auto url = obj->getProperty("url").toString();
                                            if (url.isNotEmpty())
                                                openMachine(url);
                                        }
                                    })
                 .withEventListener("setSession",
                                    [this](const juce::var& payload)
                                    {
                                        auto* obj = payload.getDynamicObject();
                                        if (obj != nullptr)
                                        {
                                            auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                                           .getChildFile("DreamDAW");
                                            dir.createDirectory();
                                            dir.getChildFile("session.json")
                                                .replaceWithText(juce::JSON::toString(juce::var(obj)));
                                        }
                                    }))
{
    addAndMakeVisible(browser);
    addAndMakeVisible(back);
    back.onClick = [this] { browser.goToURL(juce::String("https://keganacummings-source.github.io/Site/index.html")); };
    setSize(900, 640);
    setResizable(true, true);
    browser.goToURL(juce::String("https://keganacummings-source.github.io/Site/index.html"));
    startTimerHz(15);
}

void DreamDAWEditor::openMachine(const juce::String& url)
{
    proc.machineUrl = url;
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("DreamDAW");
    dir.createDirectory();
    dir.getChildFile("machine-url.txt").replaceWithText(url);
    browser.goToURL(url);
}

std::optional<juce::WebBrowserComponent::Resource> DreamDAWEditor::serve(const juce::String&)
{
    juce::WebBrowserComponent::Resource r;
    r.mimeType = "text/html";
    r.data.resize((size_t) std::strlen(kShell));
    std::memcpy(r.data.data(), kShell, r.data.size());
    return r;
}

void DreamDAWEditor::resized()
{
    auto area = getLocalBounds();
    auto bar = area.removeFromTop(32);
    back.setBounds(bar.removeFromLeft(110).reduced(4));
    browser.setBounds(area);
}

void DreamDAWEditor::timerCallback()
{
    auto js = proc.consumeMidiAsJs();
    if (js.isNotEmpty())
        browser.evaluateJavascript(js, nullptr);
}
