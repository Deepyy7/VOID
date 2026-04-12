#pragma once
#include <JuceHeader.h>

class VOIDProcessor : public juce::AudioProcessor
{
public:
    VOIDProcessor();
    ~VOIDProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VOID"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<int>  currentAlgo   { 1 };
    std::atomic<bool> frozen        { false };
    std::atomic<bool> bypassed      { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::dsp::Reverb reverb;
    juce::dsp::ProcessSpec spec;

    void updateReverbParams();

    JUCE_DECLARE_NON_COPYABLEWITH_LEAK_DETECTOR (VOIDProcessor)
};
