#pragma once
#include "PluginProcessor.h"

class DreamDAWEditor : public juce::AudioProcessorEditor
{
public:
    explicit DreamDAWEditor(DreamDAWProcessor&);
    ~DreamDAWEditor() override;
    void resized() override;

private:
    DreamDAWProcessor& proc;
    juce::TextButton back { "Homescreen" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamDAWEditor)
};
