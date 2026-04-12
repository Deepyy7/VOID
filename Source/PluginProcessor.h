#pragma once
#include <JuceHeader.h>

class VOIDProcessor : public juce::AudioProcessor
{
public:
    VOIDProcessor();
    ~VOIDProcessor() override = default;

    // ── Audio lifecycle ──────────────────────────────────────
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ── Editor ───────────────────────────────────────────────
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ── Plugin info ──────────────────────────────────────────
    const juce::String getName() const override { return "VOID"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    // ── Programs ─────────────────────────────────────────────
    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    // ── State ────────────────────────────────────────────────
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ── APVTS (public so editor can attach listeners) ────────
    juce::AudioProcessorValueTreeState apvts;

    // ── Algo + Frozen (non-float, managed manually) ──────────
    std::atomic<int>  currentAlgo   { 1 };   // 0=plate 1=hall 2=superhall 3=infinite
    std::atomic<bool> frozen        { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── DSP ──────────────────────────────────────────────────
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessSpec spec;

    void updateReverbParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VOIDProcessor)
};
