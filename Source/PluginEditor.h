#pragma once
#include "PluginProcessor.h"

class DreamDAWEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DreamDAWEditor(DreamDAWProcessor&);
    ~DreamDAWEditor() override = default;
    void resized() override;
    void timerCallback() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> serve(const juce::String& path);
    void openMachine(const juce::String& url);

    DreamDAWProcessor& proc;
    juce::WebBrowserComponent browser;
    juce::TextButton back { "Homescreen" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamDAWEditor)
};
