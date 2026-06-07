#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    void setLevel (float dB) { levelDb = dB; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 3.0f);

        const float norm = juce::jlimit (0.0f, 1.0f,
                                         (levelDb + 60.0f) / 60.0f);
        const float h = b.getHeight() * norm;
        juce::Colour c = (levelDb > -3.0f)  ? juce::Colours::red
                       : (levelDb > -9.0f)  ? juce::Colours::yellow
                                             : juce::Colours::limegreen;
        g.setColour (c);
        g.fillRoundedRectangle (b.getX(), b.getBottom() - h,
                                b.getWidth(), h, 3.0f);
    }

private:
    float levelDb = -144.0f;
};

//==============================================================================
class GRMeter : public juce::Component
{
public:
    void setGR (float grDb) { grLevel = grDb; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 3.0f);

        const float norm = juce::jlimit (0.0f, 1.0f, grLevel / 24.0f);
        const float h    = b.getHeight() * norm;
        g.setColour (juce::Colour (0xff00bfff));
        g.fillRoundedRectangle (b.getX(), b.getY(), b.getWidth(), h, 3.0f);
    }

private:
    float grLevel = 0.0f;
};

//==============================================================================
class WaveformDisplay : public juce::Component,
                        private juce::ChangeListener
{
public:
    WaveformDisplay (juce::AudioThumbnailCache& cache,
                     juce::AudioFormatManager& fm)
        : thumbnail (512, fm, cache)
    {
        thumbnail.addChangeListener (this);
    }

    ~WaveformDisplay() override { thumbnail.removeChangeListener (this); }

    void setSource (const juce::File& file)
    {
        thumbnail.setSource (new juce::FileInputSource (file));
    }

    void clearSource()
    {
        thumbnail.setSource (nullptr);
        playPos = 0.0;
        repaint();
    }

    void setPlayPosition (double normalised)
    {
        playPos = juce::jlimit (0.0, 1.0, normalised);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0a0f1a));
        g.fillRoundedRectangle (b, 5.0f);

        if (thumbnail.getTotalLength() > 0.0)
        {
            // Waveform — two channels stacked
            const auto top    = getLocalBounds().removeFromTop (getHeight() / 2).reduced (1);
            const auto bottom = getLocalBounds().removeFromBottom (getHeight() / 2).reduced (1);

            g.setColour (juce::Colour (0xff3a7abf).withAlpha (0.85f));
            thumbnail.drawChannel (g, top,    0.0, thumbnail.getTotalLength(), 0, 0.95f);

            g.setColour (juce::Colour (0xff2a6aaf).withAlpha (0.75f));
            thumbnail.drawChannel (g, bottom, 0.0, thumbnail.getTotalLength(), 1, 0.95f);

            // Playhead
            const float x = (float) getWidth() * (float) playPos;
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.drawLine (x, 2.0f, x, (float) getHeight() - 2.0f, 1.5f);

            // Played region tint
            if (playPos > 0.001)
            {
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.fillRect (0.0f, 0.0f, x, (float) getHeight());
            }
        }
        else
        {
            g.setColour (juce::Colour (0xff445566));
            g.setFont (14.0f);
            g.drawText ("Drop an audio file here or click  Load File",
                        getLocalBounds(), juce::Justification::centred);
        }

        g.setColour (juce::Colour (0xff2a3a5c));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
    }

    bool isInterestedInFileDrag (const juce::StringArray&) { return true; }

    void filesDropped (const juce::StringArray& files, int, int)
    {
        if (onFileDrop && files.size() > 0)
            onFileDrop (juce::File (files[0]));
    }

    std::function<void(juce::File)> onFileDrop;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    juce::AudioThumbnail thumbnail;
    double playPos = 0.0;
};

//==============================================================================
class MasteringCompressorAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::FileDragAndDropTarget
{
public:
    explicit MasteringCompressorAudioProcessorEditor (MasteringCompressorAudioProcessor&);
    ~MasteringCompressorAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

private:
    void timerCallback() override;
    void buildUI();

    // Standalone file transport
    void loadFile (const juce::File& file);
    void openLoadDialog();
    void startPlayback();
    void stopPlayback();
    void exportToFile();
    juce::String formatSeconds (double secs) const;
    void doOfflineExport (const juce::File& outFile);

    MasteringCompressorAudioProcessor& audioProcessor;
    const bool isStandalone;

    // ── Plugin control section ────────────────────────────────────────────────
    LevelMeter meterInL, meterInR, meterOutL, meterOutR;
    GRMeter    meterGRMid, meterGRSide;
    juce::Label lblInMeter, lblGRMeter, lblOutMeter, lblTruePeak;

    juce::Slider slThreshold, slRatio, slAttack, slRelease, slKnee, slMakeup;
    juce::Slider slMidRatio, slSideRatio;
    juce::Slider slRmsBlend, slStereoLink, slScHPF, slLookahead;
    juce::Slider slHarmonicDrive, slCeiling;
    juce::Slider slInputGain, slBlend, slTubeGrit, slSopank, slFlap, slSpoogle;

    juce::Label lblThreshold, lblRatio, lblAttack, lblRelease, lblKnee, lblMakeup;
    juce::Label lblMidRatio, lblSideRatio;
    juce::Label lblRmsBlend, lblStereoLink, lblScHPF, lblLookahead;
    juce::Label lblHarmonicDrive, lblCeiling;
    juce::Label lblSectionComp, lblSectionMS, lblSectionDet, lblSectionOut, lblSectionChar;
    juce::Label lblInputGain, lblBlend, lblTubeGrit, lblSopank, lblFlap, lblSpoogle;

    juce::ToggleButton btnAutoMakeup { "Auto" };
    juce::ToggleButton btnMsMode     { "M/S" };
    juce::ComboBox     cmbHarmonicMode;
    juce::ComboBox     cmbOversampling;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> attThreshold, attRatio, attAttack, attRelease,
                                      attKnee, attMakeup, attMidRatio, attSideRatio,
                                      attRmsBlend, attStereoLink, attScHPF, attLookahead,
                                      attHarmonicDrive, attCeiling,
                                      attInputGain, attBlend, attTubeGrit,
                                      attSopank, attFlap, attSpoogle;
    std::unique_ptr<ButtonAttachment> attAutoMakeup, attMsMode;
    std::unique_ptr<ComboAttachment>  attHarmonicMode, attOversampling;

    // ── Standalone transport (only shown when isStandalone) ───────────────────
    juce::AudioFormatManager  formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    std::unique_ptr<WaveformDisplay> waveform;

    juce::TextButton btnLoad   { "Load File" };
    juce::TextButton btnPlay   { u8"▶  Play" };
    juce::TextButton btnStop   { u8"■  Stop" };
    juce::TextButton btnExport { u8"↓  Export WAV" };
    juce::Label      lblFileName { {}, "Drop a file or click Load File" };
    juce::Label      lblPosition { {}, "0:00 / 0:00" };
    juce::ProgressBar progressBar { exportProgress };
    double           exportProgress = -1.0;

    double loadedFileSampleRate = 44100.0;
    int    loadedFileNumSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasteringCompressorAudioProcessorEditor)
};
