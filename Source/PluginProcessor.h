#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

class DreamDAWProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    DreamDAWProcessor();
    ~DreamDAWProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DREAMDAW"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void pushFromPage(const float* interleaved, int frames);
    void openMachine(const juce::String& url);

    // Editor reparents the live WebView. Closing the FL window must not destroy it.
    void attachEditor(juce::Component& parent);
    void detachEditor(juce::Component& parent);
    void layoutBrowser(juce::Rectangle<int> bounds);
    juce::WebBrowserComponent& getBrowser();

    juce::String machineUrl { "https://keganacummings-source.github.io/Site/index.html" };
    juce::String instanceId;

private:
    void timerCallback() override;
    void ensureBrowser();
    void pumpMidi();
    juce::String consumeMidiAsJs();

    struct HostWindow;
    std::unique_ptr<HostWindow> host;
    std::unique_ptr<juce::WebBrowserComponent> browser;
    bool browserReady = false;

    juce::AbstractFifo fifo { 48000 * 8 };
    juce::AudioBuffer<float> ring;
    juce::MidiBuffer pendingMidi;
    juce::CriticalSection midiLock;
    juce::CriticalSection audioLock;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamDAWProcessor)
};
