#include "PluginProcessor.h"
#include "PluginEditor.h"

static const juce::String ID_PREDELAY  = "pdly";
static const juce::String ID_DIFFUSION = "diff";
static const juce::String ID_SIZE      = "size";
static const juce::String ID_MODDEPTH  = "mdp";
static const juce::String ID_MODRATE   = "mdr";
static const juce::String ID_ATTACK    = "atk";
static const juce::String ID_HIGHCUT   = "hcut";
static const juce::String ID_HIGHMULT  = "hmlt";
static const juce::String ID_HIGHXOVER = "hxov";
static const juce::String ID_LOWCUT    = "lcut";
static const juce::String ID_LOWMULT   = "lmlt";
static const juce::String ID_MIX       = "mix";
static const juce::String ID_DECAY     = "dcy";

VOIDProcessor::VOIDProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOID_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VOIDProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_PREDELAY, "Pre-Delay", 0.0f, 750.0f, 234.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_DIFFUSION, "Diffusion", 0.0f, 1.0f, 0.60f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_SIZE, "Size", 5.0f, 100.0f, 54.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_MODDEPTH, "Mod Depth", 0.0f, 100.0f, 24.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_MODRATE, "Mod Rate", 0.1f, 5.0f, 1.27f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_ATTACK, "Attack", 0.0f, 1.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_HIGHCUT, "High Cut", 1000.0f, 20000.0f, 11092.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_HIGHMULT, "Hi Mult", 0.1f, 2.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_HIGHXOVER, "Hi X-Over", 500.0f, 8000.0f, 6000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_LOWCUT, "Low Cut", 20.0f, 500.0f, 287.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_LOWMULT, "Lo Mult", 0.1f, 2.0f, 0.26f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_MIX, "Mix", 0.0f, 100.0f, 42.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ID_DECAY, "Decay", 0.3f, 20.0f, 5.1f));
    return { params.begin(), params.end() };
}

void VOIDProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;
    reverb.prepare (spec);
    updateReverbParams();
}

void VOIDProcessor::releaseResources() {}

void VOIDProcessor::updateReverbParams()
{
    juce::dsp::Reverb::Parameters p;
    const float size    = apvts.getRawParameterValue (ID_SIZE)->load()    / 100.0f;
    const float diff    = apvts.getRawParameterValue (ID_DIFFUSION)->load();
    const float mix     = apvts.getRawParameterValue (ID_MIX)->load()     / 100.0f;
    const float hiMult  = apvts.getRawParameterValue (ID_HIGHMULT)->load();
    const float decay   = apvts.getRawParameterValue (ID_DECAY)->load()   / 20.0f;
    p.roomSize   = juce::jlimit (0.0f, 1.0f, size * 0.8f + decay * 0.2f);
    p.damping    = juce::jlimit (0.0f, 1.0f, 1.0f - hiMult * 0.5f);
    p.width      = juce::jlimit (0.0f, 1.0f, diff);
    p.wetLevel   = frozen.load() ? mix * 1.4f : mix;
    p.dryLevel   = 1.0f - mix;
    p.freezeMode = frozen.load() ? 1.0f : 0.0f;
    reverb.setParameters (p);
}

void VOIDProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (bypassed.load() || buffer.getNumChannels() < 2) return;
    updateReverbParams();
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);
}

void VOIDProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("algo",   currentAlgo.load(), nullptr);
    state.setProperty ("frozen", frozen.load(),      nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VOIDProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr) return;
    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);
    currentAlgo.store ((int)   state.getProperty ("algo",   1));
    frozen.store      ((bool)  state.getProperty ("frozen", false));
}

juce::AudioProcessorEditor* VOIDProcessor::createEditor()
{
    return new VOIDEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VOIDProcessor();
}
