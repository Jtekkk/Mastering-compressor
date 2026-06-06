#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
MasteringCompressorAudioProcessor::MasteringCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

//==============================================================================
APVTS::ParameterLayout MasteringCompressorAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    // ── Compression ──────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "threshold", "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "ratio", "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack",
        juce::NormalisableRange<float> (0.1f, 300.0f, 0.1f, 0.5f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (10.0f, 3000.0f, 1.0f, 0.4f), 200.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "knee", "Knee",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "makeup", "Makeup",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "autoMakeup", "Auto Makeup", false));

    // ── Mid / Side ────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "msMode", "M/S Mode", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "midRatio", "Mid Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "sideRatio", "Side Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 2.0f));

    // ── Detector ──────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "rmsBlend", "RMS/Peak Blend",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "stereoLink", "Stereo Link",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "scHPF", "Sidechain HPF",
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.4f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lookahead", "Lookahead",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.1f), 1.5f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    // ── Harmonics ─────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "harmonicMode", "Harmonic Mode",
        juce::StringArray { "None", "Tube", "Tape", "Transformer" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "harmonicDrive", "Harmonic Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    // ── Output ────────────────────────────────────────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "oversamplingOrder", "Oversampling",
        juce::StringArray { "1x", "2x", "4x", "8x" }, 2)); // default 4x

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "ceiling", "Ceiling",
        juce::NormalisableRange<float> (-6.0f, 0.0f, 0.1f), -0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("dBFS")));

    return layout;
}

//==============================================================================
void MasteringCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    // ── Oversampling ──────────────────────────────────────────────────────────
    const int oversamplingOrder = static_cast<int> (
        apvts.getRawParameterValue ("oversamplingOrder")->load());
    currentOversamplingOrder = oversamplingOrder;

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2,                         // stereo
        oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,                      // maxBlockSize buffering
        true);                     // interleaved for performance

    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));

    const float oversampledRate = static_cast<float> (sampleRate)
                                  * static_cast<float> (1 << oversamplingOrder);

    // ── Lookahead ─────────────────────────────────────────────────────────────
    const int maxLookahead = static_cast<int> (std::ceil (10e-3 * sampleRate)) + 1;
    lookaheadMid.prepare  (maxLookahead);
    lookaheadSide.prepare (maxLookahead);

    const float lookaheadMs = apvts.getRawParameterValue ("lookahead")->load();
    lookaheadSamples = static_cast<int> (lookaheadMs * 0.001f * sampleRate);

    // ── Sidechain filters ─────────────────────────────────────────────────────
    const float hpfFreq = apvts.getRawParameterValue ("scHPF")->load();
    scFilterMid.prepare  (static_cast<float> (sampleRate), hpfFreq);
    scFilterSide.prepare (static_cast<float> (sampleRate), hpfFreq);

    // ── DC blocks ─────────────────────────────────────────────────────────────
    dcBlockL.reset();
    dcBlockR.reset();

    // ── Compressor channels ───────────────────────────────────────────────────
    midComp.reset();
    sideComp.reset();

    // ── Harmonic saturation ───────────────────────────────────────────────────
    harmonics.prepare (oversampledRate);

    // ── True peak limiter ─────────────────────────────────────────────────────
    limiter.prepare (oversampledRate);

    // ── Parameter smoothers ───────────────────────────────────────────────────
    const double rampSec = 0.02; // 20 ms ramp for all smoothers
    smThreshold.reset (sampleRate, rampSec);
    smRatio.reset     (sampleRate, rampSec);
    smKnee.reset      (sampleRate, rampSec);
    smAttack.reset    (sampleRate, rampSec);
    smRelease.reset   (sampleRate, rampSec);
    smMakeup.reset    (sampleRate, rampSec);
    smMidRatio.reset  (sampleRate, rampSec);
    smSideRatio.reset (sampleRate, rampSec);

    smThreshold.setCurrentAndTargetValue (apvts.getRawParameterValue ("threshold")->load());
    smRatio.setCurrentAndTargetValue     (apvts.getRawParameterValue ("ratio")->load());
    smKnee.setCurrentAndTargetValue      (apvts.getRawParameterValue ("knee")->load());
    smAttack.setCurrentAndTargetValue    (apvts.getRawParameterValue ("attack")->load());
    smRelease.setCurrentAndTargetValue   (apvts.getRawParameterValue ("release")->load());
    smMakeup.setCurrentAndTargetValue    (apvts.getRawParameterValue ("makeup")->load());
    smMidRatio.setCurrentAndTargetValue  (apvts.getRawParameterValue ("midRatio")->load());
    smSideRatio.setCurrentAndTargetValue (apvts.getRawParameterValue ("sideRatio")->load());

    // ── Latency ───────────────────────────────────────────────────────────────
    setLatencySamples (lookaheadSamples
                       + static_cast<int> (oversampler->getLatencyInSamples()));
}

void MasteringCompressorAudioProcessor::releaseResources()
{
    if (oversampler)
        oversampler->reset();
}

//==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations
bool MasteringCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

//==============================================================================
void MasteringCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // ── Update parameter targets ──────────────────────────────────────────────
    smThreshold.setTargetValue (apvts.getRawParameterValue ("threshold")->load());
    smRatio.setTargetValue     (apvts.getRawParameterValue ("ratio")->load());
    smKnee.setTargetValue      (apvts.getRawParameterValue ("knee")->load());
    smAttack.setTargetValue    (apvts.getRawParameterValue ("attack")->load());
    smRelease.setTargetValue   (apvts.getRawParameterValue ("release")->load());
    smMidRatio.setTargetValue  (apvts.getRawParameterValue ("midRatio")->load());
    smSideRatio.setTargetValue (apvts.getRawParameterValue ("sideRatio")->load());

    const bool   autoMakeup  = apvts.getRawParameterValue ("autoMakeup")->load() > 0.5f;
    const bool   msMode      = apvts.getRawParameterValue ("msMode")->load() > 0.5f;
    const float  rmsBlend    = apvts.getRawParameterValue ("rmsBlend")->load();
    const float  stereoLink  = apvts.getRawParameterValue ("stereoLink")->load();
    const float  hpfFreq     = apvts.getRawParameterValue ("scHPF")->load();
    const float  lookaheadMs = apvts.getRawParameterValue ("lookahead")->load();
    const float  ceiling     = dbToLin (apvts.getRawParameterValue ("ceiling")->load());

    const int harmonicModeIdx = static_cast<int> (
        apvts.getRawParameterValue ("harmonicMode")->load());
    harmonics.setMode  (static_cast<HarmonicMode> (harmonicModeIdx));
    harmonics.setDrive (apvts.getRawParameterValue ("harmonicDrive")->load());
    limiter.setCeiling (ceiling);

    // Update sidechain HPF frequency
    scFilterMid.setFrequency  (hpfFreq);
    scFilterSide.setFrequency (hpfFreq);

    // Update lookahead
    lookaheadSamples = static_cast<int> (lookaheadMs * 0.001f * currentSampleRate);

    auto* channelL = buffer.getWritePointer (0);
    auto* channelR = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    // RMS coefficients (50 ms integration time)
    const float rmsCoeff  = std::exp (-1.0f / (0.05f * static_cast<float> (currentSampleRate)));
    // Peak hold decay (~200 ms)
    const float peakCoeff = std::exp (-1.0f / (0.2f  * static_cast<float> (currentSampleRate)));

    // Block-level peak accumulators for meters
    float inPeakL = 0.0f, inPeakR = 0.0f;
    float grMidMax = 0.0f, grSideMax = 0.0f;

    // ── Per-sample compression loop ───────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        // 1. DC block
        float inL = dcBlockL.process (channelL[i]);
        float inR = dcBlockR.process (channelR[i]);

        // 2. Input metering
        inPeakL = std::max (inPeakL, std::abs (inL));
        inPeakR = std::max (inPeakR, std::abs (inR));

        // 3. M/S encode
        const float mid  = (inL + inR) * 0.70710678f;
        const float side = (inL - inR) * 0.70710678f;

        // 4. Write to lookahead buffers (audio path is delayed)
        lookaheadMid.write  (mid);
        lookaheadSide.write (side);

        // 5. Sidechain HPF on un-delayed signal for detection
        const float detMid  = scFilterMid.process  (mid);
        const float detSide = scFilterSide.process (side);

        // 6. Detector levels (linear amplitude)
        float lvlMid  = midComp.detectLevel  (detMid,  rmsCoeff, peakCoeff, rmsBlend);
        float lvlSide = sideComp.detectLevel (detSide, rmsCoeff, peakCoeff, rmsBlend);

        // 7. Stereo linking (blend individual and max)
        const float linked = std::max (lvlMid, lvlSide);
        lvlMid  = lvlMid  * (1.0f - stereoLink) + linked * stereoLink;
        lvlSide = lvlSide * (1.0f - stereoLink) + linked * stereoLink;

        // 8. Smoothed parameters this sample
        const float threshold = smThreshold.getNextValue();
        const float globalRatio = smRatio.getNextValue();
        const float knee       = smKnee.getNextValue();
        const float attackMs   = smAttack.getNextValue();
        const float releaseMs  = smRelease.getNextValue();

        const float midRatio  = msMode ? smMidRatio.getNextValue()  : globalRatio;
        const float sideRatio = msMode ? smSideRatio.getNextValue() : globalRatio;

        // 9. Gain computer
        const float lvlMidDb  = linToDb (lvlMid);
        const float lvlSideDb = linToDb (lvlSide);

        const float targetGrMid  = CompressorChannel::computeGainReduction (
            lvlMidDb,  threshold, midRatio,  knee);
        const float targetGrSide = CompressorChannel::computeGainReduction (
            lvlSideDb, threshold, sideRatio, knee);

        // 10. Attack / release smoothing
        const float sr         = static_cast<float> (currentSampleRate);
        const float attackCoeff = std::exp (-1.0f / (0.001f * attackMs  * sr));

        const float smoothedGrMid  = midComp.applySmoothing  (targetGrMid,  attackCoeff, releaseMs, sr);
        const float smoothedGrSide = sideComp.applySmoothing (targetGrSide, attackCoeff, releaseMs, sr);

        grMidMax  = std::max (grMidMax,  smoothedGrMid);
        grSideMax = std::max (grSideMax, smoothedGrSide);

        // 11. Auto makeup
        float makeupDb = smMakeup.getNextValue();
        if (autoMakeup)
        {
            const float threshold_ = threshold;
            const float ratio_     = (midRatio + sideRatio) * 0.5f;
            float autoMk = (-threshold_) * (1.0f - 1.0f / ratio_) * 0.5f;
            autoMk = juce::jlimit (-12.0f, 24.0f, autoMk);
            makeupDb = autoMk;
        }
        const float makeupLin = dbToLin (makeupDb);

        // 12. Read delayed mid and side (lookahead)
        float delayedMid  = lookaheadMid.read  (lookaheadSamples);
        float delayedSide = lookaheadSide.read (lookaheadSamples);

        // 13. Apply gain reduction + makeup
        delayedMid  *= dbToLin (-smoothedGrMid)  * makeupLin;
        delayedSide *= dbToLin (-smoothedGrSide) * makeupLin;

        // 14. M/S decode
        channelL[i] = (delayedMid + delayedSide) * 0.70710678f;
        channelR[i] = (delayedMid - delayedSide) * 0.70710678f;
    }

    // ── Atomic meter updates ──────────────────────────────────────────────────
    meterInPeakL.store  (linToDb (inPeakL));
    meterInPeakR.store  (linToDb (inPeakR));
    meterGRMid.store    (grMidMax);
    meterGRSide.store   (grSideMax);

    // ── Oversampled stage: harmonics + true peak limiter ──────────────────────
    {
        auto block = juce::dsp::AudioBlock<float> (buffer);
        auto upBlock = oversampler->processSamplesUp (block);

        const int upSamples = static_cast<int> (upBlock.getNumSamples());
        auto* upL = upBlock.getChannelPointer (0);
        auto* upR = upBlock.getChannelPointer (1);

        float truePeakAcc = 0.0f;

        for (int i = 0; i < upSamples; ++i)
        {
            harmonics.processStereo (upL[i], upR[i]);
            limiter.processStereo   (upL[i], upR[i]);
            truePeakAcc = std::max (truePeakAcc, std::max (std::abs (upL[i]), std::abs (upR[i])));
        }

        meterTruePeak.store (linToDb (truePeakAcc));

        oversampler->processSamplesDown (block);
    }

    // ── Output peak meters ────────────────────────────────────────────────────
    float outPeakL = 0.0f, outPeakR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        outPeakL = std::max (outPeakL, std::abs (channelL[i]));
        outPeakR = std::max (outPeakR, std::abs (channelR[i]));
    }
    meterOutPeakL.store (linToDb (outPeakL));
    meterOutPeakR.store (linToDb (outPeakR));
}

//==============================================================================
juce::AudioProcessorEditor* MasteringCompressorAudioProcessor::createEditor()
{
    return new MasteringCompressorAudioProcessorEditor (*this);
}

void MasteringCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& data)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, data);
}

void MasteringCompressorAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MasteringCompressorAudioProcessor();
}
