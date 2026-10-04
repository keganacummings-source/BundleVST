#include "PluginProcessor.h"
#include "PluginEditor.h"

DreamDAWProcessor::DreamDAWProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    ring.setSize(2, fifo.getTotalSize());
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("DreamDAW");
    dir.createDirectory();
    auto saved = dir.getChildFile("site-folder.txt");
    if (saved.existsAsFile())
        siteFolder = saved.loadFileAsString().trim();
    auto machine = dir.getChildFile("machine-url.txt");
    if (machine.existsAsFile())
        machineUrl = machine.loadFileAsString().trim();
}

void DreamDAWProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate;
}

bool DreamDAWProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DreamDAWProcessor::pushFromPage(const float* interleaved, int frames)
{
    if (interleaved == nullptr || frames <= 0)
        return;
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
            js << "DreamHost.noteOn(" << m.getNoteNumber() << "," << (m.getFloatVelocity()) << ");";
        else if (m.isNoteOff())
            js << "DreamHost.noteOff(" << m.getNoteNumber() << ");";
    }
    return js;
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

juce::AudioProcessorEditor* DreamDAWProcessor::createEditor()
{
    return new DreamDAWEditor(*this);
}

void DreamDAWProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream os(dest, false);
    os.writeString(machineUrl);
    os.writeString(siteFolder);
}

void DreamDAWProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream is(data, (size_t) sizeInBytes, false);
    auto url = is.readString();
    auto folder = is.readString();
    if (url.isNotEmpty())
        machineUrl = url;
    if (folder.isNotEmpty())
        siteFolder = folder;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DreamDAWProcessor();
}
