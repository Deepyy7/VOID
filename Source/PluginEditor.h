#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VOIDEditor : public juce::AudioProcessorEditor
{
public:
    explicit VOIDEditor (VOIDProcessor&);
    ~VOIDEditor() override;
    void resized() override;
    bool handleURL (const juce::String& url);

private:
    VOIDProcessor& processor;

    struct VoidWebView : public juce::WebBrowserComponent
    {
        explicit VoidWebView (VOIDEditor& e) : owner (e) {}
        bool pageAboutToLoad (const juce::String& url) override;
        VOIDEditor& owner;
    };

    VoidWebView webView { *this };
    juce::File  htmlTempFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VOIDEditor)
};
