#include "PluginProcessor.h"
#include "PluginEditor.h"

static const juce::String ID_PREDELAY="pdly",ID_DIFFUSION="diff",ID_SIZE="size";
static const juce::String ID_MODDEPTH="mdp",ID_MODRATE="mdr",ID_ATTACK="atk";
static const juce::String ID_HIGHCUT="hcut",ID_HIGHMULT="hmlt",ID_HIGHXOVER="hxov";
static const juce::String ID_LOWCUT="lcut",ID_LOWMULT="lmlt",ID_MIX="mix",ID_DECAY="dcy";

VOIDProcessor::VOIDProcessor()
    : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"VOID_STATE",createParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout VOIDProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_PREDELAY,"Pre-Delay",0.0f,750.0f,234.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_DIFFUSION,"Diffusion",0.0f,1.0f,0.60f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_SIZE,"Size",5.0f,100.0f,54.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_MODDEPTH,"Mod Depth",0.0f,100.0f,24.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_MODRATE,"Mod Rate",0.1f,5.0f,1.27f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_ATTACK,"Attack",0.0f,1.0f,1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_HIGHCUT,"High Cut",1000.0f,20000.0f,11092.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_HIGHMULT,"Hi Mult",0.1f,2.0f,0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_HIGHXOVER,"Hi X-Over",500.0f,8000.0f,6000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_LOWCUT,"Low Cut",20.0f,500.0f,287.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_LOWMULT,"Lo Mult",0.1f,2.0f,0.26f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_MIX,"Mix",0.0f,100.0f,42.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID_DECAY,"Decay",0.3f,20.0f,5.1f));
    return {p.begin(),p.end()};
}

void VOIDProcessor::prepareToPlay(double sr,int spb) {
    spec.sampleRate=sr; spec.maximumBlockSize=(juce::uint32)spb; spec.numChannels=2;
    convolution.prepare(spec); fallbackReverb.prepare(spec);
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize=0.85f;rp.damping=0.3f;rp.width=1.0f;rp.wetLevel=0.42f;rp.dryLevel=0.58f;
    fallbackReverb.setParameters(rp);
}
void VOIDProcessor::releaseResources() {}
float VOIDProcessor::getMixNorm() const { return apvts.getRawParameterValue(ID_MIX)->load()/100.0f; }

void VOIDProcessor::loadIR(const juce::String& b64,int irSR,int irLen) {
    juce::MemoryBlock decoded;
    { juce::MemoryOutputStream mos(decoded,false); juce::Base64::convertFromBase64(mos,b64); }
    if(decoded.getSize()<(size_t)(irLen*2*2))return;
    const int16_t* raw=(const int16_t*)decoded.getData();
    juce::AudioBuffer<float> irBuf(2,irLen);
    for(int i=0;i<irLen;++i){irBuf.setSample(0,i,raw[i]/32767.0f);irBuf.setSample(1,i,raw[i+irLen]/32767.0f);}
    juce::ScopedLock sl(irLock);
    convolution.loadImpulseResponse(std::move(irBuf),irSR,juce::dsp::Convolution::Stereo::yes,juce::dsp::Convolution::Trim::no,juce::dsp::Convolution::Normalise::yes);
    irLoaded.store(true);
}

void VOIDProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noD;
    if(bypassed.load()||buffer.getNumChannels()<2)return;
    const float mix=getMixNorm(),dry=1.0f-mix;
    juce::AudioBuffer<float> dryBuf(buffer.getNumChannels(),buffer.getNumSamples());
    for(int c=0;c<buffer.getNumChannels();++c) dryBuf.copyFrom(c,0,buffer,c,0,buffer.getNumSamples());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    if(irLoaded.load()){juce::ScopedTryLock stl(irLock);if(stl.isLocked())convolution.process(ctx);else fallbackReverb.process(ctx);}
    else fallbackReverb.process(ctx);
    for(int c=0;c<buffer.getNumChannels();++c){buffer.applyGain(c,0,buffer.getNumSamples(),mix);buffer.addFrom(c,0,dryBuf,c,0,buffer.getNumSamples(),dry);}
}

void VOIDProcessor::getStateInformation(juce::MemoryBlock& d) {
    auto s=apvts.copyState();s.setProperty("algo",currentAlgo.load(),nullptr);s.setProperty("frozen",(int)frozen.load(),nullptr);
    std::unique_ptr<juce::XmlElement> x(s.createXml());copyXmlToBinary(*x,d);
}
void VOIDProcessor::setStateInformation(const void* d,int s) {
    std::unique_ptr<juce::XmlElement> x(getXmlFromBinary(d,s));if(!x)return;
    auto st=juce::ValueTree::fromXml(*x);apvts.replaceState(st);
    currentAlgo.store((int)st.getProperty("algo",1));frozen.store((bool)st.getProperty("frozen",false));
}
juce::AudioProcessorEditor* VOIDProcessor::createEditor(){return new VOIDEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new VOIDProcessor();}
