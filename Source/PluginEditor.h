#pragma once
#include "PluginProcessor.h"

class DreamDAWEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DreamDAWEditor(DreamDAWProcessor&);
    ~DreamDAWEditor() override;
    void resized() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    DreamDAWProcessor& proc;
    juce::TextButton back { "← DREAMDAW" };
    int attachTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamDAWEditor)
};
