#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupSlider (juce::Slider& s, juce::Label& l, const juce::String& name,
                         juce::Component* parent)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    s.setTextBoxIsEditable (true);
    parent->addAndMakeVisible (s);

    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (11.0f));
    parent->addAndMakeVisible (l);
}

static void setupSectionLabel (juce::Label& l, const juce::String& text,
                               juce::Component* parent)
{
    l.setText (text, juce::dontSendNotification);
    l.setFont (juce::Font (12.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
    l.setJustificationType (juce::Justification::left);
    parent->addAndMakeVisible (l);
}

//==============================================================================
MasteringCompressorAudioProcessorEditor::MasteringCompressorAudioProcessorEditor (
    MasteringCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (860, 540);
    buildUI();
    startTimerHz (30);
}

MasteringCompressorAudioProcessorEditor::~MasteringCompressorAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::buildUI()
{
    auto& apvts = audioProcessor.apvts;

    // ── Meters ────────────────────────────────────────────────────────────────
    addAndMakeVisible (meterInL);
    addAndMakeVisible (meterInR);
    addAndMakeVisible (meterGRMid);
    addAndMakeVisible (meterGRSide);
    addAndMakeVisible (meterOutL);
    addAndMakeVisible (meterOutR);

    auto setupML = [this] (juce::Label& l, const juce::String& t) {
        l.setText (t, juce::dontSendNotification);
        l.setFont (juce::Font (10.0f));
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };
    setupML (lblInMeter,  "IN");
    setupML (lblGRMeter,  "GR");
    setupML (lblOutMeter, "OUT");
    setupML (lblTruePeak, "TP: ---");

    // ── Compression section ───────────────────────────────────────────────────
    setupSectionLabel (lblSectionComp, "COMPRESSION", this);
    setupSlider (slThreshold, lblThreshold, "Threshold",  this);
    setupSlider (slRatio,     lblRatio,     "Ratio",      this);
    setupSlider (slAttack,    lblAttack,    "Attack",     this);
    setupSlider (slRelease,   lblRelease,   "Release",    this);
    setupSlider (slKnee,      lblKnee,      "Knee",       this);
    setupSlider (slMakeup,    lblMakeup,    "Makeup",     this);

    btnAutoMakeup.setClickingTogglesState (true);
    addAndMakeVisible (btnAutoMakeup);

    // ── M/S section ───────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionMS, "MID / SIDE", this);
    btnMsMode.setClickingTogglesState (true);
    addAndMakeVisible (btnMsMode);
    setupSlider (slMidRatio,  lblMidRatio,  "Mid Ratio",  this);
    setupSlider (slSideRatio, lblSideRatio, "Side Ratio", this);

    // ── Detector section ──────────────────────────────────────────────────────
    setupSectionLabel (lblSectionDet, "DETECTOR", this);
    setupSlider (slRmsBlend,   lblRmsBlend,   "RMS/Peak",  this);
    setupSlider (slStereoLink, lblStereoLink, "Link",      this);
    setupSlider (slScHPF,      lblScHPF,      "SC HPF",    this);
    setupSlider (slLookahead,  lblLookahead,  "Lookahead", this);

    // ── Output section ────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionOut, "OUTPUT", this);

    cmbHarmonicMode.addItemList ({ "None", "Tube", "Tape", "Transformer" }, 1);
    addAndMakeVisible (cmbHarmonicMode);
    setupSlider (slHarmonicDrive, lblHarmonicDrive, "Drive", this);

    cmbOversampling.addItemList ({ "1x", "2x", "4x", "8x" }, 1);
    addAndMakeVisible (cmbOversampling);

    setupSlider (slCeiling, lblCeiling, "Ceiling", this);

    // ── APVTS attachments ─────────────────────────────────────────────────────
    attThreshold    = std::make_unique<SliderAttachment> (apvts, "threshold",     slThreshold);
    attRatio        = std::make_unique<SliderAttachment> (apvts, "ratio",         slRatio);
    attAttack       = std::make_unique<SliderAttachment> (apvts, "attack",        slAttack);
    attRelease      = std::make_unique<SliderAttachment> (apvts, "release",       slRelease);
    attKnee         = std::make_unique<SliderAttachment> (apvts, "knee",          slKnee);
    attMakeup       = std::make_unique<SliderAttachment> (apvts, "makeup",        slMakeup);
    attMidRatio     = std::make_unique<SliderAttachment> (apvts, "midRatio",      slMidRatio);
    attSideRatio    = std::make_unique<SliderAttachment> (apvts, "sideRatio",     slSideRatio);
    attRmsBlend     = std::make_unique<SliderAttachment> (apvts, "rmsBlend",      slRmsBlend);
    attStereoLink   = std::make_unique<SliderAttachment> (apvts, "stereoLink",    slStereoLink);
    attScHPF        = std::make_unique<SliderAttachment> (apvts, "scHPF",         slScHPF);
    attLookahead    = std::make_unique<SliderAttachment> (apvts, "lookahead",     slLookahead);
    attHarmonicDrive= std::make_unique<SliderAttachment> (apvts, "harmonicDrive", slHarmonicDrive);
    attCeiling      = std::make_unique<SliderAttachment> (apvts, "ceiling",       slCeiling);
    attAutoMakeup   = std::make_unique<ButtonAttachment> (apvts, "autoMakeup",    btnAutoMakeup);
    attMsMode       = std::make_unique<ButtonAttachment> (apvts, "msMode",        btnMsMode);
    attHarmonicMode = std::make_unique<ComboAttachment>  (apvts, "harmonicMode",  cmbHarmonicMode);
    attOversampling = std::make_unique<ComboAttachment>  (apvts, "oversamplingOrder", cmbOversampling);
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::timerCallback()
{
    meterInL.setLevel   (audioProcessor.meterInPeakL.load());
    meterInR.setLevel   (audioProcessor.meterInPeakR.load());
    meterGRMid.setGR    (audioProcessor.meterGRMid.load());
    meterGRSide.setGR   (audioProcessor.meterGRSide.load());
    meterOutL.setLevel  (audioProcessor.meterOutPeakL.load());
    meterOutR.setLevel  (audioProcessor.meterOutPeakR.load());

    const float tp = audioProcessor.meterTruePeak.load();
    lblTruePeak.setText (juce::String::formatted ("TP: %.1f dBFS", tp),
                         juce::dontSendNotification);
    lblTruePeak.setColour (juce::Label::textColourId,
                           tp > -0.5f ? juce::Colours::red : juce::Colours::white);
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    // Section background cards
    auto drawCard = [&] (juce::Rectangle<int> r) {
        g.setColour (juce::Colour (0xff16213e));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff0f3460).withAlpha (0.6f));
        g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.0f);
    };

    drawCard ({ 4,   4,  62, getHeight() - 8 });  // input meters
    drawCard ({ 70,  4,  62, getHeight() - 8 });  // GR meters
    drawCard ({ 136, 4,  62, getHeight() - 8 });  // output meters
    drawCard ({ 204, 4, 298, getHeight() - 8 });  // compression
    drawCard ({ 506, 4, 156, getHeight() - 8 });  // M/S
    drawCard ({ 666, 4, 190, getHeight() - 8 });  // detector + output
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    const int meterW  = 22;
    const int meterH  = H - 50;
    const int meterY  = 14;
    const int lblH    = 16;
    const int knobSz  = 68;
    const int knobLH  = 16;
    const int padX    = 6;
    const int sectionLabelH = 18;

    // ── Input meters ──────────────────────────────────────────────────────────
    lblInMeter.setBounds (4, meterY - lblH, 62, lblH);
    meterInL.setBounds   (8,       meterY,           meterW, meterH);
    meterInR.setBounds   (8 + meterW + 4, meterY,   meterW, meterH);

    // ── GR meters ─────────────────────────────────────────────────────────────
    lblGRMeter.setBounds (70, meterY - lblH, 62, lblH);
    meterGRMid.setBounds  (74,          meterY, meterW, meterH);
    meterGRSide.setBounds (74 + meterW + 4, meterY, meterW, meterH);

    // ── Output meters ─────────────────────────────────────────────────────────
    lblOutMeter.setBounds (136, meterY - lblH, 62, lblH);
    meterOutL.setBounds   (140,          meterY, meterW, meterH);
    meterOutR.setBounds   (140 + meterW + 4, meterY, meterW, meterH);

    lblTruePeak.setBounds (136, H - 30, 62, 20);

    // ── Compression section ───────────────────────────────────────────────────
    int cx = 210, cy = 8;
    lblSectionComp.setBounds (cx, cy, 290, sectionLabelH);
    cy += sectionLabelH + 4;

    auto placeKnob = [&] (juce::Slider& s, juce::Label& l, int x, int y) {
        s.setBounds (x, y, knobSz, knobSz);
        l.setBounds (x, y + knobSz, knobSz, knobLH);
    };

    // Row 1: Threshold, Ratio, Attack
    placeKnob (slThreshold, lblThreshold, cx,               cy);
    placeKnob (slRatio,     lblRatio,     cx + knobSz + padX, cy);
    placeKnob (slAttack,    lblAttack,    cx + (knobSz + padX) * 2, cy);
    // Row 2: Release, Knee, Makeup
    cy += knobSz + knobLH + 10;
    placeKnob (slRelease, lblRelease, cx,               cy);
    placeKnob (slKnee,    lblKnee,    cx + knobSz + padX, cy);
    placeKnob (slMakeup,  lblMakeup,  cx + (knobSz + padX) * 2, cy);
    cy += knobSz + knobLH + 6;
    btnAutoMakeup.setBounds (cx + (knobSz + padX) * 2, cy, knobSz, 22);

    // ── M/S section ───────────────────────────────────────────────────────────
    int mx = 512, my = 8;
    lblSectionMS.setBounds (mx, my, 148, sectionLabelH);
    my += sectionLabelH + 4;
    btnMsMode.setBounds (mx, my, 60, 22);
    my += 30;
    placeKnob (slMidRatio,  lblMidRatio,  mx,               my);
    placeKnob (slSideRatio, lblSideRatio, mx + knobSz + padX, my);

    // ── Detector section ──────────────────────────────────────────────────────
    int dx = 672, dy = 8;
    lblSectionDet.setBounds (dx, dy, 180, sectionLabelH);
    dy += sectionLabelH + 4;
    placeKnob (slRmsBlend,   lblRmsBlend,   dx,               dy);
    placeKnob (slStereoLink, lblStereoLink, dx + knobSz + padX, dy);
    dy += knobSz + knobLH + 10;
    placeKnob (slScHPF,     lblScHPF,     dx,               dy);
    placeKnob (slLookahead, lblLookahead, dx + knobSz + padX, dy);

    // ── Output section ────────────────────────────────────────────────────────
    dy += knobSz + knobLH + 10;
    lblSectionOut.setBounds (dx, dy, 180, sectionLabelH);
    dy += sectionLabelH + 4;

    cmbHarmonicMode.setBounds (dx, dy, 140, 22);
    dy += 28;
    placeKnob (slHarmonicDrive, lblHarmonicDrive, dx, dy);
    placeKnob (slCeiling,       lblCeiling,       dx + knobSz + padX, dy);
    dy += knobSz + knobLH + 10;
    cmbOversampling.setBounds (dx, dy, 80, 22);
}

//==============================================================================
// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
