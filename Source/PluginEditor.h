#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VOIDEditor : public juce::AudioProcessorEditor,
                   public juce::WebBrowserComponent::ResourceProvider
{
public:
    explicit VOIDEditor (VOIDProcessor&);
    ~VOIDEditor() override = default;

    void resized() override;

    // ── ResourceProvider — serves void_final.html ─────────────
    juce::WebBrowserComponent::Resource getResource (const juce::String& url) override;

private:
    VOIDProcessor& processor;

    juce::WebBrowserComponent webView {
        juce::WebBrowserComponent::Options{}
            .withResourceProvider (this)
            .withNativeIntegrationEnabled()
    };

    // ── Send params to JS on init ────────────────────────────
    void syncAllParamsToJS();
    void sendToJS (const juce::String& js);

    // ── Handle URL commands from JS (juce://...) ────────────
    bool pageAboutToLoad (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VOIDEditor)
};
