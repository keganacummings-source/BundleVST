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
}

DreamDAWEditor::~DreamDAWEditor()
{
    proc.detachEditor(*this);
}

void DreamDAWEditor::resized()
{
    auto area = getLocalBounds();
    auto bar = area.removeFromTop(32);
    back.setBounds(bar.removeFromLeft(110).reduced(4));
    proc.layoutBrowser(area);
}
