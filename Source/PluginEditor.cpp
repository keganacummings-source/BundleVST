#include "PluginEditor.h"

DreamDAWEditor::DreamDAWEditor(DreamDAWProcessor& p)
    : AudioProcessorEditor(p),
      proc(p)
{
    addAndMakeVisible(back);
    back.onClick = [this]
    {
        proc.openMachine("https://keganacummings-source.github.io/Site/index.html");
    };
    setSize(900, 640);
    setResizable(true, true);
    proc.attachEditor(*this);
    startTimerHz(5);
}

DreamDAWEditor::~DreamDAWEditor()
{
    stopTimer();
    proc.detachEditor(*this);
}

void DreamDAWEditor::parentHierarchyChanged()
{
    proc.attachEditor(*this);
    resized();
}

void DreamDAWEditor::visibilityChanged()
{
    if (isShowing())
    {
        proc.attachEditor(*this);
        resized();
    }
}

void DreamDAWEditor::timerCallback()
{
    // FL gives the editor its native window after the constructor. Re-parent
    // until that peer exists so the site draws in the plugin, not a side window.
    proc.attachEditor(*this);
    resized();
    if (++attachTicks > 20)
        stopTimer();
}

void DreamDAWEditor::resized()
{
    auto area = getLocalBounds();
    auto bar = area.removeFromTop(32);
    back.setBounds(bar.removeFromLeft(110).reduced(4));
    proc.layoutBrowser(area);
}
