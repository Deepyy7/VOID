cmake_minimum_required(VERSION 3.22)
project(VOID VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# ── JUCE via FetchContent ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        7.0.12
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Binary resources (HTML UI) ────────────────────────────────
juce_add_binary_data(VOIDBinaryData
  SOURCES Resources/void_final.html
)

# ── Plugin target ─────────────────────────────────────────────
juce_add_plugin(VOID
  COMPANY_NAME            "Gravitas Audio"
  COMPANY_WEBSITE         "https://gravaudio.com"
  COMPANY_EMAIL           "info@gravaudio.com"
  BUNDLE_ID               "com.gravitasaudio.void"
  PLUGIN_MANUFACTURER_CODE Grva
  PLUGIN_CODE             Void
  FORMATS                 VST3 AU
  PRODUCT_NAME            "VOID"
  IS_SYNTH                FALSE
  NEEDS_MIDI_INPUT        FALSE
  NEEDS_MIDI_OUTPUT       FALSE
  IS_MIDI_EFFECT          FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES         "Fx" "Reverb"
  AU_MAIN_TYPE            kAudioUnitType_Effect
)

target_sources(VOID PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)

juce_generate_juce_header(VOID)

target_compile_definitions(VOID PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VOID PRIVATE
  VOIDBinaryData
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_gui_extra
  juce::juce_dsp
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)
