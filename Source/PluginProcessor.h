#pragma once
#include <JuceHeader.h>

class DreamDAWProcessor : public juce::AudioProcessor
{
public:
    DreamDAWProcessor();
    ~DreamDAWProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void pushFromPage(const float* interleaved, int frames);
    juce::String consumeMidiAsJs();

    juce::String machineUrl { "https://www.dreamdaw.com/" };
    juce::String siteFolder;

private:
    juce::AbstractFifo fifo { 48000 * 4 };
    juce::AudioBuffer<float> ring;
    juce::MidiBuffer pendingMidi;
    juce::CriticalSection midiLock;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamDAWProcessor)
};
