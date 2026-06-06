#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Simple vertical meter component (peak hold, dB scale)
class LevelMeter : public juce::Component
{
public:
    void setLevel (float dB) { levelDb = dB; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (bounds, 3.0f);

        const float minDb = -60.0f, maxDb = 0.0f;
        const float norm  = juce::jlimit (0.0f, 1.0f,
                                          (levelDb - minDb) / (maxDb - minDb));
        const float h = bounds.getHeight() * norm;

        juce::Colour colour = (levelDb > -3.0f) ? juce::Colours::red
                            : (levelDb > -9.0f) ? juce::Colours::yellow
                                                : juce::Colours::limegreen;

        g.setColour (colour);
        g.fillRoundedRectangle (bounds.getX(),
                                bounds.getBottom() - h,
                                bounds.getWidth(), h, 3.0f);
    }

private:
    float levelDb = -144.0f;
};

//==============================================================================
// Gain-reduction meter (green bar growing downward)
class GRMeter : public juce::Component
{
public:
    void setGR (float grDb) { grLevel = grDb; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (bounds, 3.0f);

        const float maxGR = 24.0f;
        const float norm  = juce::jlimit (0.0f, 1.0f, grLevel / maxGR);
        const float h     = bounds.getHeight() * norm;

        g.setColour (juce::Colour (0xff00bfff));
        g.fillRoundedRectangle (bounds.getX(), bounds.getY(),
                                bounds.getWidth(), h, 3.0f);
    }

private:
    float grLevel = 0.0f;
};

//==============================================================================
class MasteringCompressorAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit MasteringCompressorAudioProcessorEditor (MasteringCompressorAudioProcessor&);
    ~MasteringCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildUI();

    MasteringCompressorAudioProcessor& audioProcessor;

    // ── Meters ────────────────────────────────────────────────────────────────
    LevelMeter meterInL, meterInR;
    LevelMeter meterOutL, meterOutR;
    GRMeter    meterGRMid, meterGRSide;

    // ── Sliders ───────────────────────────────────────────────────────────────
    juce::Slider slThreshold, slRatio, slAttack, slRelease, slKnee, slMakeup;
    juce::Slider slMidRatio, slSideRatio;
    juce::Slider slRmsBlend, slStereoLink, slScHPF, slLookahead;
    juce::Slider slHarmonicDrive, slCeiling;

    // ── Labels ────────────────────────────────────────────────────────────────
    juce::Label lblThreshold, lblRatio, lblAttack, lblRelease, lblKnee, lblMakeup;
    juce::Label lblMidRatio, lblSideRatio;
    juce::Label lblRmsBlend, lblStereoLink, lblScHPF, lblLookahead;
    juce::Label lblHarmonicDrive, lblCeiling;
    juce::Label lblSectionComp, lblSectionMS, lblSectionDet, lblSectionOut;
    juce::Label lblInMeter, lblGRMeter, lblOutMeter;
    juce::Label lblTruePeak;

    // ── Toggles / combo boxes ─────────────────────────────────────────────────
    juce::ToggleButton btnAutoMakeup { "Auto" };
    juce::ToggleButton btnMsMode     { "M/S" };
    juce::ComboBox     cmbHarmonicMode;
    juce::ComboBox     cmbOversampling;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> attThreshold, attRatio, attAttack, attRelease,
                                      attKnee, attMakeup, attMidRatio, attSideRatio,
                                      attRmsBlend, attStereoLink, attScHPF, attLookahead,
                                      attHarmonicDrive, attCeiling;
    std::unique_ptr<ButtonAttachment> attAutoMakeup, attMsMode;
    std::unique_ptr<ComboAttachment>  attHarmonicMode, attOversampling;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasteringCompressorAudioProcessorEditor)
};
