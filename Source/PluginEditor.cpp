#include "PluginEditor.h"
#include <BinaryData.h>

// Algo names (JS uses these strings)
static const juce::StringArray ALGO_NAMES { "plate", "hall", "superhall", "infinite" };

// JS param IDs (must match PARAM_IDS in void_final.html)
static const std::map<juce::String, juce::String> JS_PARAM_IDS {
    { "pdly",  "predelay"  },
    { "diff",  "diffusion" },
    { "size",  "size"      },
    { "mdp",   "moddepth"  },
    { "mdr",   "modrate"   },
    { "atk",   "attack"    },
    { "hcut",  "highcut"   },
    { "hmlt",  "highmult"  },
    { "hxov",  "highxover" },
    { "lcut",  "lowcut"    },
    { "lmlt",  "lowmult"   },
    { "mix",   "mix"       },
    { "dcy",   "decay"     },
};

// ── Constructor ───────────────────────────────────────────────
VOIDEditor::VOIDEditor (VOIDProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    addAndMakeVisible (webView);
    setSize (1060, 600);
    setResizable (false, false);

    // Navigate to the plugin UI
    webView.goToURL ("https://void.plugin/");

    // Sync params to JS once page loads (short delay)
    juce::Timer::callAfterDelay (800, [this] { syncAllParamsToJS(); });
}

// ── Layout ────────────────────────────────────────────────────
void VOIDEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

// ── Resource provider — serve void_final.html ────────────────
juce::WebBrowserComponent::Resource VOIDEditor::getResource (const juce::String& url)
{
    // Serve HTML for root or /void_final.html
    if (url == "/" || url.contains ("void_final.html") || url.isEmpty())
    {
        const char* html = BinaryData::void_final_html;
        const int   size = BinaryData::void_final_htmlSize;

        return { std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (html),
            reinterpret_cast<const std::byte*> (html) + size),
            "text/html" };
    }

    return {};
}

// ── Send JS to page ───────────────────────────────────────────
void VOIDEditor::sendToJS (const juce::String& js)
{
    webView.evaluateJavascript (js, {});
}

// ── Push all current param values to JS ───────────────────────
void VOIDEditor::syncAllParamsToJS()
{
    // Parameters
    for (auto& [jsId, internalId] : JS_PARAM_IDS)
    {
        auto* param = processor.apvts.getParameter (jsId);
        if (param == nullptr) continue;

        const float norm = param->getValue();
        sendToJS ("window.setParamFromHost && window.setParamFromHost('"
                  + internalId + "'," + juce::String (norm, 4) + ")");
    }

    // Algorithm
    const int algoIdx = processor.currentAlgo.load();
    if (algoIdx >= 0 && algoIdx < ALGO_NAMES.size())
        sendToJS ("window.setAlgoFromHost && window.setAlgoFromHost('"
                  + ALGO_NAMES[algoIdx] + "')");

    // Frozen
    const bool frz = processor.frozen.load();
    sendToJS ("window.setFrozenFromHost && window.setFrozenFromHost("
              + juce::String (frz ? "true" : "false") + ")");

    // Bypass
    const bool byp = processor.isBypassed();
    sendToJS ("window.setBypassFromHost && window.setBypassFromHost("
              + juce::String (byp ? "true" : "false") + ")");
}

// ── Handle juce:// URLs from JS ───────────────────────────────
bool VOIDEditor::pageAboutToLoad (const juce::String& url)
{
    if (! url.startsWith ("juce://"))
        return true;  // allow normal navigation

    // ── juce://param?id=<ID>&value=<norm> ─────────────────────
    if (url.startsWith ("juce://param"))
    {
        const auto id    = url.fromFirstOccurrenceOf ("id=",    false, false)
                              .upToFirstOccurrenceOf ("&",      false, false);
        const auto valStr = url.fromFirstOccurrenceOf ("value=", false, false)
                               .upToFirstOccurrenceOf ("&",      false, false);

        auto* param = processor.apvts.getParameter (id);
        if (param != nullptr)
            param->setValueNotifyingHost (valStr.getFloatValue());
    }

    // ── juce://bypass?v=<0|1> ─────────────────────────────────
    else if (url.startsWith ("juce://bypass"))
    {
        const auto v = url.fromFirstOccurrenceOf ("v=", false, false).getIntValue();
        processor.setBypassedStateNotifyingHost (v != 0);
    }

    // ── juce://algo?v=<name> ──────────────────────────────────
    else if (url.startsWith ("juce://algo"))
    {
        const auto name = url.fromFirstOccurrenceOf ("v=", false, false);
        const int idx   = ALGO_NAMES.indexOf (name);
        if (idx >= 0) processor.currentAlgo.store (idx);
    }

    // ── juce://frozen?v=<0|1> ─────────────────────────────────
    else if (url.startsWith ("juce://frozen"))
    {
        const auto v = url.fromFirstOccurrenceOf ("v=", false, false).getIntValue();
        processor.frozen.store (v != 0);
    }

    return false; // block juce:// from actual navigation
}
