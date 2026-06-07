#pragma once

#include <JuceHeader.h>

#include "DSP/DCBlock.h"
#include "DSP/LookaheadBuffer.h"
#include "DSP/SidechainFilter.h"
#include "DSP/CompressorChannel.h"
#include "DSP/HarmonicSaturation.h"
#include "DSP/TruePeakLimiter.h"
#include "DSP/Sopank.h"
#include "DSP/Flap.h"
#include "DSP/Spoogle.h"

class MasteringCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    MasteringCompressorAudioProcessor();
    ~MasteringCompressorAudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()                               override { return 1; }
    int  getCurrentProgram()                            override { return 0; }
    void setCurrentProgram (int)                        override {}
    const juce::String getProgramName (int)             override { return {}; }
    void changeProgramName (int, const juce::String&)   override {}

    void getStateInformation (juce::MemoryBlock& data) override;
    void setStateInformation (const void* data, int size) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters",
                                               createParameterLayout() };

    // Atomic meters — read by the editor on the UI thread
    std::atomic<float> meterInPeakL   { -144.0f };
    std::atomic<float> meterInPeakR   { -144.0f };
    std::atomic<float> meterOutPeakL  { -144.0f };
    std::atomic<float> meterOutPeakR  { -144.0f };
    std::atomic<float> meterGRMid     {    0.0f };
    std::atomic<float> meterGRSide    {    0.0f };
    std::atomic<float> meterTruePeak  { -144.0f };

    // ── File-injection for standalone playback (written on UI thread) ─────────
    juce::AudioBuffer<float>  fileInputBuffer;
    std::atomic<bool>         fileBufferReady  { false };
    std::atomic<int>          filePlayPosition { 0 };
    std::atomic<bool>         filePlaybackActive { false };
    double                    fileInputSampleRate = 44100.0;

    friend class MasteringCompressorAudioProcessorEditor;

private:
    //==========================================================================
    // DSP components
    DCBlock           dcBlockL, dcBlockR;
    LookaheadBuffer   lookaheadMid,  lookaheadSide;
    LookaheadBuffer   dryBufL,       dryBufR;   // delayed dry for parallel blend
    SidechainFilter   scFilterMid,   scFilterSide;
    CompressorChannel midComp,       sideComp;
    HarmonicSaturation harmonics;
    TruePeakLimiter    limiter;
    SopankProcessor    sopankProc;
    FlapProcessor      flapProc;
    SpoogleProcessor   spoogleProc;

    // Oversampling — recreated in prepareToPlay based on parameter
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int currentOversamplingOrder = 2; // 4x default

    double currentSampleRate  = 44100.0;
    int    currentBlockSize   = 512;
    int    lookaheadSamples   = 0;

    // Denormal protection
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> denormalDC;

    // Parameter smoothers (ramp length set in prepareToPlay)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smThreshold;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smRatio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smKnee;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smAttack;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smRelease;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smMakeup;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smMidRatio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smSideRatio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smInputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smBlend;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smTubeGrit;

    //==========================================================================
    inline float linToDb (float lin) const noexcept
    {
        return lin > 1e-7f ? 20.0f * std::log10 (lin) : -144.0f;
    }
    inline float dbToLin (float db) const noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasteringCompressorAudioProcessor)
};
