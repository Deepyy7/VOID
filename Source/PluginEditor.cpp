#include "PluginEditor.h"

static const juce::StringArray ALGO_NAMES { "plate", "hall", "superhall", "infinite" };

// JS internal IDs → APVTS parameter IDs
static const std::vector<std::pair<juce::String,juce::String>> PARAM_MAP {
    { "pdly", "predelay"  }, { "diff", "diffusion" }, { "size", "size"      },
    { "mdp",  "moddepth"  }, { "mdr",  "modrate"   }, { "atk",  "attack"    },
    { "hcut", "highcut"   }, { "hmlt", "highmult"  }, { "hxov", "highxover" },
    { "lcut", "lowcut"    }, { "lmlt", "lowmult"   }, { "mix",  "mix"       },
    { "dcy",  "decay"     },
};

// ── WebView: intercept juce:// URLs ──────────────────────────
bool VOIDEditor::VoidWebView::pageAboutToLoad (const juce::String& url)
{
    if (url.startsWith ("juce://"))
    {
        owner.handleURL (url);
        return false; // block actual navigation
    }
    return true;
}

// ── Constructor ───────────────────────────────────────────────
VOIDEditor::VOIDEditor (VOIDProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    addAndMakeVisible (webView);
    setSize (1060, 600);
    setResizable (false, false);

    // Write HTML to a temp file and load it
    htmlTempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("VOID_plugin_ui_" + juce::String (juce::Random::getSystemRandom().nextInt()) + ".html");

    htmlTempFile.replaceWithData (BinaryData::void_final_html,
                                  (size_t) BinaryData::void_final_htmlSize);

    webView.goToURL (htmlTempFile.getURL().toString (false));

    // Sync params to JS after page loads
    juce::Timer::callAfterDelay (1200, [this] { syncAllParamsToJS(); });
}

VOIDEditor::~VOIDEditor()
{
    htmlTempFile.deleteFile();
}

// ── Layout ────────────────────────────────────────────────────
void VOIDEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

// ── Send JS string to page ────────────────────────────────────
void VOIDEditor::sendToJS (const juce::String& js)
{
    webView.evaluateJavascript (js);
}

// ── Sync all param values to JS on load ──────────────────────
void VOIDEditor::syncAllParamsToJS()
{
    for (auto& [jsId, paramId] : PARAM_MAP)
    {
        auto* param = processor.apvts.getParameter (jsId);
        if (param == nullptr) continue;
        const float norm = param->getValue();
        sendToJS ("window.setParamFromHost && window.setParamFromHost('"
                  + paramId + "'," + juce::String (norm, 4) + ")");
    }

    const int algoIdx = processor.currentAlgo.load();
    if (algoIdx >= 0 && algoIdx < ALGO_NAMES.size())
        sendToJS ("window.setAlgoFromHost && window.setAlgoFromHost('"
                  + ALGO_NAMES[algoIdx] + "')");

    sendToJS ("window.setFrozenFromHost && window.setFrozenFromHost("
              + juce::String (processor.frozen.load() ? "true" : "false") + ")");

    sendToJS ("window.setBypassFromHost && window.setBypassFromHost("
              + juce::String (processor.bypassed.load() ? "true" : "false") + ")");
}

// ── Handle juce:// commands from JS ──────────────────────────
bool VOIDEditor::handleURL (const juce::String& url)
{
    auto after = [&](const juce::String& key) {
        return url.fromFirstOccurrenceOf (key, false, false)
                  .upToFirstOccurrenceOf ("&", false, false);
    };

    if (url.startsWith ("juce://param"))
    {
        const auto id  = after ("id=");
        const auto val = after ("value=").getFloatValue();
        if (auto* param = processor.apvts.getParameter (id))
            param->setValueNotifyingHost (val);
    }
    else if (url.startsWith ("juce://bypass"))
    {
        const bool v = after ("v=").getIntValue() != 0;
        processor.bypassed.store (v);
    }
    else if (url.startsWith ("juce://algo"))
    {
        const int idx = ALGO_NAMES.indexOf (after ("v="));
        if (idx >= 0) processor.currentAlgo.store (idx);
    }
    else if (url.startsWith ("juce://frozen"))
    {
        processor.frozen.store (after ("v=").getIntValue() != 0);
    }

    return false;
}
