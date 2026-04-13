#include "PluginEditor.h"
static const juce::StringArray ALGO_NAMES{"plate","hall","superhall","infinite"};
static const std::vector<std::pair<juce::String,juce::String>> PARAM_MAP{
    {"pdly","predelay"},{"diff","diffusion"},{"size","size"},{"mdp","moddepth"},
    {"mdr","modrate"},{"atk","attack"},{"hcut","highcut"},{"hmlt","highmult"},
    {"hxov","highxover"},{"lcut","lowcut"},{"lmlt","lowmult"},{"mix","mix"},{"dcy","decay"}};

bool VOIDEditor::VoidWebView::pageAboutToLoad(const juce::String& url) {
    if(url.startsWith("juce://")){owner.handleURL(url);return false;}
    return true;
}
static juce::String buildStateHash(VOIDProcessor& p) {
    juce::String h="#state=";
    h+="algo:"+juce::String(p.currentAlgo.load())+";";
    h+="frozen:"+juce::String((int)p.frozen.load())+";";
    for(auto&[jsId,paramId]:PARAM_MAP)
        if(auto* param=p.apvts.getParameter(jsId))
            h+=jsId+":"+juce::String(param->getValue(),5)+";";
    return h;
}
VOIDEditor::VOIDEditor(VOIDProcessor& p):AudioProcessorEditor(p),processor(p){
    addAndMakeVisible(webView);setSize(1060,600);setResizable(false,false);loadPage();}
void VOIDEditor::loadPage(){
    htmlTempFile=juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("VOID_ui_"+juce::String(juce::Random::getSystemRandom().nextInt())+".html");
    if(htmlTempFile.replaceWithData(BinaryData::void_final_html,(size_t)BinaryData::void_final_htmlSize))
        webView.goToURL("file://"+htmlTempFile.getFullPathName()+buildStateHash(processor));
}
VOIDEditor::~VOIDEditor(){htmlTempFile.deleteFile();}
void VOIDEditor::resized(){webView.setBounds(getLocalBounds());}
bool VOIDEditor::handleURL(const juce::String& url){
    auto after=[&](const juce::String& key){
        return url.fromFirstOccurrenceOf(key,false,false).upToFirstOccurrenceOf("&",false,false);};
    if(url.startsWith("juce://param")){
        if(auto* p=processor.apvts.getParameter(after("id=")))p->setValueNotifyingHost(after("value=").getFloatValue());
    }else if(url.startsWith("juce://bypass"))processor.bypassed.store(after("v=").getIntValue()!=0);
    else if(url.startsWith("juce://algo")){const int i=ALGO_NAMES.indexOf(after("v="));if(i>=0)processor.currentAlgo.store(i);}
    else if(url.startsWith("juce://frozen"))processor.frozen.store(after("v=").getIntValue()!=0);
    else if(url.startsWith("juce://ir")){
        const int sr=after("sr=").getIntValue(),len=after("len=").getIntValue();
        juce::String b64=juce::URL::removeEscapeChars(url.fromFirstOccurrenceOf("data=",false,false));
        if(sr>0&&len>0&&b64.isNotEmpty())
            juce::MessageManager::callAsync([this,b64,sr,len]{processor.loadIR(b64,sr,len);});
    }
    return false;
}
