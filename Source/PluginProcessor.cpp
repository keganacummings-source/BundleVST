#include "PluginProcessor.h"
#include "PluginEditor.h"

// Installed before each page. Keeps the AudioContext awake when FL minimizes
// or closes the plugin window, bridges piano-rack MIDI, and taps the page
// output into the plugin buses.
static const char* kHostScript = R"JS(
(function(){
  if (window.__DREAMDAW_HOST) return;
  window.__DREAMDAW_HOST = true;
  window.DreamHost = window.DreamHost || {};
  window.DreamHost.noteOn = function(n, v){
    try { if (typeof window.noteOn === "function") return window.noteOn(n, v); } catch (e) {}
    try {
      if (typeof ensure === "function" && typeof pluck === "function") {
        ensure();
        var f = 440 * Math.pow(2, ((+n) - 69) / 12);
        pluck(ctx, ch.in, f, ctx.currentTime, Math.max(0.18, Math.min(0.9, v == null ? 0.8 : +v)), (typeof P !== "undefined" ? P : {}));
      }
    } catch (e) {}
  };
  window.DreamHost.noteOff = function(n){
    try { if (typeof window.noteOff === "function") return window.noteOff(n); } catch (e) {}
  };
  window.DreamHost.allOff = function(){
    for (var i = 0; i < 128; i++) window.DreamHost.noteOff(i);
  };
  function arm(ctx){
    if (!ctx || ctx.__dreamArmed) return;
    ctx.__dreamArmed = true;
    var tap = ctx.createGain();
    tap.gain.value = 1;
    ctx.__dreamTap = tap;
    var sp = ctx.createScriptProcessor(1024, 2, 2);
    tap.connect(sp);
    var mute = ctx.createGain();
    mute.gain.value = 0;
    sp.connect(mute);
    mute.connect(ctx.destination);
    sp.onaudioprocess = function(ev){
      try {
        var L = ev.inputBuffer.getChannelData(0);
        var R = ev.inputBuffer.numberOfChannels > 1 ? ev.inputBuffer.getChannelData(1) : L;
        var n = L.length;
        var buf = new Float32Array(n * 2);
        for (var i = 0; i < n; i++) { buf[i * 2] = L[i]; buf[i * 2 + 1] = R[i]; }
        var bytes = new Uint8Array(buf.buffer);
        var bin = "";
        for (var i = 0; i < bytes.length; i += 1024) {
          bin += String.fromCharCode.apply(null, bytes.subarray(i, Math.min(bytes.length, i + 1024)));
        }
        if (window.__JUCE__ && window.__JUCE__.backend)
          window.__JUCE__.backend.emitEvent("audioBlock", { b64: btoa(bin), frames: n });
      } catch (e) {}
    };
    try {
      var keep = ctx.createOscillator();
      var kg = ctx.createGain();
      kg.gain.value = 0.00001;
      keep.frequency.value = 18;
      keep.connect(kg);
      kg.connect(ctx.destination);
      keep.start();
    } catch (e) {}
    var resume = function(){ try { if (ctx.state !== "running") ctx.resume(); } catch (e) {} };
    setInterval(resume, 350);
    document.addEventListener("visibilitychange", resume);
    window.addEventListener("blur", resume);
    resume();
  }
  var orig = AudioNode.prototype.connect;
  AudioNode.prototype.connect = function(dest){
    var r = orig.apply(this, arguments);
    try {
      if (dest && dest.context && dest.context.destination === dest) {
        arm(dest.context);
        if (this !== dest.context.__dreamTap)
          orig.call(this, dest.context.__dreamTap);
      }
    } catch (e) {}
    return r;
  };
  setInterval(function(){
    try {
      if (window.ctx && window.ctx.resume && window.ctx.state !== "running") window.ctx.resume();
    } catch (e) {}
  }, 400);
})();
)JS";

struct DreamDAWProcessor::HostWindow : public juce::DocumentWindow
{
    HostWindow()
        : DocumentWindow({}, juce::Colours::black, 0)
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setOpaque(true);
        setName({});
        // Tool window: no taskbar button, no Alt-Tab entry.
        addToDesktop(juce::ComponentPeer::windowIsTemporary);
        setBounds(-24000, -24000, 640, 480);
        setVisible(false);
    }

    void closeButtonPressed() override {}
};

DreamDAWProcessor::DreamDAWProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    ring.setSize(2, fifo.getTotalSize());
    instanceId = juce::Uuid().toString();
#if JUCE_WINDOWS
    _putenv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS=--disable-background-timer-throttling --disable-renderer-backgrounding --disable-backgrounding-occluded-windows --disable-features=CalculateNativeWinOcclusion,IntensiveWakeUpThrottling --autoplay-policy=no-user-gesture-required");
#endif
    juce::MessageManager::callAsync([this] { ensureBrowser(); });
    startTimerHz(30);
}

DreamDAWProcessor::~DreamDAWProcessor()
{
    stopTimer();
    browser.reset();
    host.reset();
}

void DreamDAWProcessor::ensureBrowser()
{
    if (browserReady || browser != nullptr)
        return;
    auto userData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("DreamDAW")
                        .getChildFile("instances")
                        .getChildFile(instanceId);
    userData.createDirectory();

    host.reset();
    browser = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
                                        .withUserDataFolder(userData))
            .withNativeIntegrationEnabled()
            .withUserScript(kHostScript)
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
            .withEventListener("audioBlock",
                               [this](const juce::var& payload)
                               {
                                   auto* obj = payload.getDynamicObject();
                                   if (obj == nullptr)
                                       return;
                                   auto b64 = obj->getProperty("b64").toString();
                                   auto frames = (int) obj->getProperty("frames");
                                   if (b64.isEmpty() || frames <= 0)
                                       return;
                                   juce::MemoryOutputStream bin;
                                   if (! juce::Base64::convertFromBase64(bin, b64))
                                       return;
                                   if ((int) bin.getDataSize() < frames * 2 * (int) sizeof(float))
                                       return;
                                   pushFromPage(static_cast<const float*>(bin.getData()), frames);
                               }));
    // Do not parent the WebView to a floating window here. FL's editor must be
    // the first peer, otherwise WebView2 stays on that window and the plugin is blank.
    browser->setVisible(true);
    browserReady = true;
}

void DreamDAWProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
    juce::MessageManager::callAsync([this] { ensureBrowser(); });
}

bool DreamDAWProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DreamDAWProcessor::pushFromPage(const float* interleaved, int frames)
{
    if (interleaved == nullptr || frames <= 0)
        return;
    juce::ScopedLock sl(audioLock);
    int start1, size1, start2, size2;
    fifo.prepareToWrite(frames, start1, size1, start2, size2);
    auto write = [&](int start, int n, int offset)
    {
        for (int i = 0; i < n; ++i)
        {
            ring.setSample(0, start + i, interleaved[(offset + i) * 2]);
            ring.setSample(1, start + i, interleaved[(offset + i) * 2 + 1]);
        }
    };
    write(start1, size1, 0);
    write(start2, size2, size1);
    fifo.finishedWrite(size1 + size2);
}

juce::String DreamDAWProcessor::consumeMidiAsJs()
{
    juce::MidiBuffer local;
    {
        juce::ScopedLock sl(midiLock);
        local.swapWith(pendingMidi);
    }
    juce::String js;
    for (const auto meta : local)
    {
        auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            js << "try{DreamHost.noteOn(" << m.getNoteNumber() << "," << m.getFloatVelocity() << ");}catch(e){}"
               << "try{if(window.noteOn)window.noteOn(" << m.getNoteNumber() << "," << m.getFloatVelocity() << ");}catch(e){}";
        }
        else if (m.isNoteOff())
        {
            js << "try{DreamHost.noteOff(" << m.getNoteNumber() << ");}catch(e){}"
               << "try{if(window.noteOff)window.noteOff(" << m.getNoteNumber() << ");}catch(e){}";
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            js << "try{DreamHost.allOff();}catch(e){}";
        }
    }
    return js;
}

void DreamDAWProcessor::pumpMidi()
{
    if (browser == nullptr)
        return;
    auto js = consumeMidiAsJs();
    if (js.isNotEmpty())
        browser->evaluateJavascript(js, nullptr);
}

void DreamDAWProcessor::timerCallback()
{
    ensureBrowser();
    pumpMidi();
}

void DreamDAWProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    {
        juce::ScopedLock sl(midiLock);
        pendingMidi.addEvents(midi, 0, buffer.getNumSamples(), 0);
    }
    const int n = buffer.getNumSamples();
    int start1, size1, start2, size2;
    {
        juce::ScopedLock sl(audioLock);
        fifo.prepareToRead(n, start1, size1, start2, size2);
        auto read = [&](int start, int count, int dest)
        {
            for (int i = 0; i < count; ++i)
            {
                buffer.setSample(0, dest + i, ring.getSample(0, start + i));
                buffer.setSample(1, dest + i, ring.getSample(1, start + i));
            }
        };
        read(start1, size1, 0);
        read(start2, size2, size1);
        fifo.finishedRead(size1 + size2);
    }
}

void DreamDAWProcessor::openMachine(const juce::String& url)
{
    machineUrl = url;
    if (browser != nullptr)
        browser->goToURL(url);
}

void DreamDAWProcessor::attachEditor(juce::Component& parent)
{
    ensureBrowser();
    if (browser == nullptr)
        return;
    if (host != nullptr)
        host->setVisible(false);
    if (browser->getParentComponent() != &parent)
        parent.addAndMakeVisible(browser.get());
    browser->setVisible(true);
    browser->toFront(false);
    if (!pageLoaded)
    {
        browser->goToURL(machineUrl);
        pageLoaded = true;
    }
}

void DreamDAWProcessor::detachEditor(juce::Component& parent)
{
    if (browser == nullptr)
        return;
    if (browser->getParentComponent() != &parent && browser->getParentComponent() != nullptr)
        return;
    if (host == nullptr)
        host = std::make_unique<HostWindow>();
    host->setContentNonOwned(browser.get(), false);
    browser->setBounds(0, 0, 640, 480);
    browser->setVisible(true);
    // Keep the parked view off-screen and out of Alt-Tab. It only exists so
    // closing the FL window does not kill the instrument.
    host->setBounds(-24000, -24000, 640, 480);
    host->setVisible(true);
}

void DreamDAWProcessor::layoutBrowser(juce::Rectangle<int> bounds)
{
    if (browser != nullptr && browser->getParentComponent() != nullptr)
        browser->setBounds(bounds);
}

juce::WebBrowserComponent* DreamDAWProcessor::getBrowser()
{
    ensureBrowser();
    return browser.get();
}

juce::AudioProcessorEditor* DreamDAWProcessor::createEditor()
{
    return new DreamDAWEditor(*this);
}

void DreamDAWProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream os(dest, false);
    os.writeString(machineUrl);
}

void DreamDAWProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream is(data, (size_t) sizeInBytes, false);
    auto url = is.readString();
    if (url.isNotEmpty())
        machineUrl = url;
    if (browser != nullptr)
        browser->goToURL(machineUrl);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DreamDAWProcessor();
}
