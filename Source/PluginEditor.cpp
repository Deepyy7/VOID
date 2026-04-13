#include "PluginEditor.h"

static const juce::StringArray ALGO_NAMES { "plate", "hall", "superhall", "infinite" };

bool VOIDEditor::VoidWebView::pageAboutToLoad (const juce::String& url)
{
    if (url.startsWith ("juce://"))
    {
        owner.handleURL (url);
        return false;
    }
    return true;
}

VOIDEditor::VOIDEditor (VOIDProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    addAndMakeVisible (webView);
    setSize (1060, 600);
    setResizable (false, false);

    htmlTempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("VOID_ui_" + juce::String (juce::Random::getSystemRandom().nextInt()) + ".html");

    if (htmlTempFile.replaceWithData (BinaryData::void_final_html,
                                      (size_t) BinaryData::void_final_htmlSize))
        webView.goToURL ("file://" + htmlTempFile.getFullPathName());
}

VOIDEditor::~VOIDEditor() { htmlTempFile.deleteFile(); }

void VOIDEditor::resized() { webView.setBounds (getLocalBounds()); }

bool VOIDEditor::handleURL (const juce::String& url)
{
    auto after = [&](const juce::String& key) {
        return url.fromFirstOccurrenceOf (key, false, false)
                  .upToFirstOccurrenceOf ("&", false, false);
    };
    if (url.startsWith ("juce://param"))
    {
        if (auto* p = processor.apvts.getParameter (after ("id=")))
            p->setValueNotifyingHost (after ("value=").getFloatValue());
    }
    else if (url.startsWith ("juce://bypass"))
        processor.bypassed.store (after ("v=").getIntValue() != 0);
    else if (url.startsWith ("juce://algo"))
    {
        const int idx = ALGO_NAMES.indexOf (after ("v="));
        if (idx >= 0) processor.currentAlgo.store (idx);
    }
    else if (url.startsWith ("juce://frozen"))
        processor.frozen.store (after ("v=").getIntValue() != 0);
    return false;
}
