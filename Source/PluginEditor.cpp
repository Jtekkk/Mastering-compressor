#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupSlider (juce::Slider& s, juce::Label& l,
                         const juce::String& name, juce::Component* parent)
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

static void styleTransportButton (juce::TextButton& b, juce::Colour c)
{
    b.setColour (juce::TextButton::buttonColourId,   c.withAlpha (0.85f));
    b.setColour (juce::TextButton::buttonOnColourId, c.brighter (0.3f));
    b.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
    b.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
}

//==============================================================================
MasteringCompressorAudioProcessorEditor::MasteringCompressorAudioProcessorEditor (
    MasteringCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      isStandalone (juce::JUCEApplicationBase::isStandaloneApp())
{
    setSize (860, isStandalone ? 700 : 540);
    buildUI();
    startTimerHz (30);
}

MasteringCompressorAudioProcessorEditor::~MasteringCompressorAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.filePlaybackActive.store (false);
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::buildUI()
{
    auto& apvts = audioProcessor.apvts;

    // ── Meters ────────────────────────────────────────────────────────────────
    for (auto* m : { &meterInL, &meterInR, &meterOutL, &meterOutR })
        addAndMakeVisible (m);
    addAndMakeVisible (meterGRMid);
    addAndMakeVisible (meterGRSide);

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

    // ── Compression ───────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionComp, "COMPRESSION", this);
    setupSlider (slThreshold, lblThreshold, "Threshold",  this);
    setupSlider (slRatio,     lblRatio,     "Ratio",      this);
    setupSlider (slAttack,    lblAttack,    "Attack",     this);
    setupSlider (slRelease,   lblRelease,   "Release",    this);
    setupSlider (slKnee,      lblKnee,      "Knee",       this);
    setupSlider (slMakeup,    lblMakeup,    "Makeup",     this);
    btnAutoMakeup.setClickingTogglesState (true);
    addAndMakeVisible (btnAutoMakeup);

    // ── M/S ───────────────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionMS, "MID / SIDE", this);
    btnMsMode.setClickingTogglesState (true);
    addAndMakeVisible (btnMsMode);
    setupSlider (slMidRatio,  lblMidRatio,  "Mid Ratio",  this);
    setupSlider (slSideRatio, lblSideRatio, "Side Ratio", this);

    // ── Detector ──────────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionDet, "DETECTOR", this);
    setupSlider (slRmsBlend,   lblRmsBlend,   "RMS/Peak",  this);
    setupSlider (slStereoLink, lblStereoLink, "Link",      this);
    setupSlider (slScHPF,      lblScHPF,      "SC HPF",    this);
    setupSlider (slLookahead,  lblLookahead,  "Lookahead", this);

    // ── Output ────────────────────────────────────────────────────────────────
    setupSectionLabel (lblSectionOut, "OUTPUT", this);
    cmbHarmonicMode.addItemList ({ "None", "Tube", "Tape", "Transformer" }, 1);
    addAndMakeVisible (cmbHarmonicMode);
    setupSlider (slHarmonicDrive, lblHarmonicDrive, "Drive",   this);
    setupSlider (slCeiling,       lblCeiling,       "Ceiling", this);
    cmbOversampling.addItemList ({ "1x", "2x", "4x", "8x" }, 1);
    addAndMakeVisible (cmbOversampling);

    // ── Attachments ───────────────────────────────────────────────────────────
    attThreshold     = std::make_unique<SliderAttachment> (apvts, "threshold",     slThreshold);
    attRatio         = std::make_unique<SliderAttachment> (apvts, "ratio",         slRatio);
    attAttack        = std::make_unique<SliderAttachment> (apvts, "attack",        slAttack);
    attRelease       = std::make_unique<SliderAttachment> (apvts, "release",       slRelease);
    attKnee          = std::make_unique<SliderAttachment> (apvts, "knee",          slKnee);
    attMakeup        = std::make_unique<SliderAttachment> (apvts, "makeup",        slMakeup);
    attMidRatio      = std::make_unique<SliderAttachment> (apvts, "midRatio",      slMidRatio);
    attSideRatio     = std::make_unique<SliderAttachment> (apvts, "sideRatio",     slSideRatio);
    attRmsBlend      = std::make_unique<SliderAttachment> (apvts, "rmsBlend",      slRmsBlend);
    attStereoLink    = std::make_unique<SliderAttachment> (apvts, "stereoLink",    slStereoLink);
    attScHPF         = std::make_unique<SliderAttachment> (apvts, "scHPF",         slScHPF);
    attLookahead     = std::make_unique<SliderAttachment> (apvts, "lookahead",     slLookahead);
    attHarmonicDrive = std::make_unique<SliderAttachment> (apvts, "harmonicDrive", slHarmonicDrive);
    attCeiling       = std::make_unique<SliderAttachment> (apvts, "ceiling",       slCeiling);
    attAutoMakeup    = std::make_unique<ButtonAttachment> (apvts, "autoMakeup",    btnAutoMakeup);
    attMsMode        = std::make_unique<ButtonAttachment> (apvts, "msMode",        btnMsMode);
    attHarmonicMode  = std::make_unique<ComboAttachment>  (apvts, "harmonicMode",  cmbHarmonicMode);
    attOversampling  = std::make_unique<ComboAttachment>  (apvts, "oversamplingOrder", cmbOversampling);

    // ── Standalone transport ──────────────────────────────────────────────────
    if (isStandalone)
    {
        formatManager.registerBasicFormats();

        waveform = std::make_unique<WaveformDisplay> (thumbnailCache, formatManager);
        waveform->onFileDrop = [this] (const juce::File& f) { loadFile (f); };
        addAndMakeVisible (*waveform);

        styleTransportButton (btnLoad,   juce::Colour (0xff2a5298));
        styleTransportButton (btnPlay,   juce::Colour (0xff1a7a40));
        styleTransportButton (btnStop,   juce::Colour (0xff7a2020));
        styleTransportButton (btnExport, juce::Colour (0xff5a3a8a));

        btnLoad.onClick   = [this] { openLoadDialog(); };
        btnPlay.onClick   = [this] { startPlayback();  };
        btnStop.onClick   = [this] { stopPlayback();   };
        btnExport.onClick = [this] { exportToFile();   };

        btnPlay.setEnabled  (false);
        btnStop.setEnabled  (false);
        btnExport.setEnabled (false);

        lblFileName.setFont (juce::Font (12.0f));
        lblFileName.setColour (juce::Label::textColourId, juce::Colour (0xffcccccc));
        lblPosition.setFont  (juce::Font (12.0f, juce::Font::bold));
        lblPosition.setColour (juce::Label::textColourId, juce::Colours::white);
        lblPosition.setJustificationType (juce::Justification::right);

        addAndMakeVisible (btnLoad);
        addAndMakeVisible (btnPlay);
        addAndMakeVisible (btnStop);
        addAndMakeVisible (btnExport);
        addAndMakeVisible (lblFileName);
        addAndMakeVisible (lblPosition);
        addAndMakeVisible (progressBar);
        progressBar.setVisible (false);
    }
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    auto drawCard = [&] (juce::Rectangle<int> r) {
        g.setColour (juce::Colour (0xff16213e));
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff0f3460).withAlpha (0.6f));
        g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.0f);
    };

    const int yo = isStandalone ? 160 : 0;

    if (isStandalone)
    {
        // Transport card
        g.setColour (juce::Colour (0xff0d1525));
        g.fillRoundedRectangle (4.0f, 4.0f, getWidth() - 8.0f, 150.0f, 6.0f);
        g.setColour (juce::Colour (0xff1e3a6e).withAlpha (0.7f));
        g.drawRoundedRectangle (4.5f, 4.5f, getWidth() - 9.0f, 149.0f, 6.0f, 1.0f);
    }

    drawCard ({ 4,          4 + yo,  62, getHeight() - yo - 8 });
    drawCard ({ 70,         4 + yo,  62, getHeight() - yo - 8 });
    drawCard ({ 136,        4 + yo,  62, getHeight() - yo - 8 });
    drawCard ({ 204,        4 + yo, 298, getHeight() - yo - 8 });
    drawCard ({ 506,        4 + yo, 156, getHeight() - yo - 8 });
    drawCard ({ 666,        4 + yo, 190, getHeight() - yo - 8 });
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::resized()
{
    const int yo       = isStandalone ? 160 : 0;
    const int meterH   = getHeight() - yo - 50;
    const int meterY   = yo + 14;
    const int meterW   = 22;
    const int lblH     = 16;
    const int knobSz   = 68;
    const int knobLH   = 16;
    const int padX     = 6;
    const int secLH    = 18;

    // ── Standalone transport ──────────────────────────────────────────────────
    if (isStandalone)
    {
        const int bw = 100, bh = 28, by = 14;
        int bx = 10;
        btnLoad.setBounds   (bx, by, bw,      bh); bx += bw + 6;
        btnPlay.setBounds   (bx, by, bw,      bh); bx += bw + 6;
        btnStop.setBounds   (bx, by, bw,      bh); bx += bw + 6;
        btnExport.setBounds (bx, by, bw + 30, bh); bx += bw + 36;

        lblFileName.setBounds (bx, by,     getWidth() - bx - 120, bh);
        lblPosition.setBounds (getWidth() - 115, by, 108, bh);

        progressBar.setBounds (10, by + bh + 6, getWidth() - 20, 10);
        waveform->setBounds (10, by + bh + 22, getWidth() - 20, 96);
    }

    // ── Input meters ──────────────────────────────────────────────────────────
    lblInMeter.setBounds  (4,  meterY - lblH, 62, lblH);
    meterInL.setBounds    (8,  meterY, meterW, meterH);
    meterInR.setBounds    (8  + meterW + 4, meterY, meterW, meterH);

    // ── GR meters ─────────────────────────────────────────────────────────────
    lblGRMeter.setBounds  (70, meterY - lblH, 62, lblH);
    meterGRMid.setBounds  (74, meterY, meterW, meterH);
    meterGRSide.setBounds (74 + meterW + 4, meterY, meterW, meterH);

    // ── Output meters ─────────────────────────────────────────────────────────
    lblOutMeter.setBounds (136, meterY - lblH, 62, lblH);
    meterOutL.setBounds   (140, meterY, meterW, meterH);
    meterOutR.setBounds   (140 + meterW + 4, meterY, meterW, meterH);
    lblTruePeak.setBounds (136, getHeight() - 28, 62, 20);

    // ── Compression ───────────────────────────────────────────────────────────
    int cx = 210, cy = yo + 8;
    lblSectionComp.setBounds (cx, cy, 290, secLH); cy += secLH + 4;

    auto placeKnob = [&] (juce::Slider& s, juce::Label& l, int x, int y) {
        s.setBounds (x, y, knobSz, knobSz);
        l.setBounds (x, y + knobSz, knobSz, knobLH);
    };

    placeKnob (slThreshold, lblThreshold, cx,                      cy);
    placeKnob (slRatio,     lblRatio,     cx + knobSz + padX,      cy);
    placeKnob (slAttack,    lblAttack,    cx + (knobSz + padX) * 2, cy);
    cy += knobSz + knobLH + 10;
    placeKnob (slRelease, lblRelease, cx,                      cy);
    placeKnob (slKnee,    lblKnee,    cx + knobSz + padX,      cy);
    placeKnob (slMakeup,  lblMakeup,  cx + (knobSz + padX) * 2, cy);
    cy += knobSz + knobLH + 6;
    btnAutoMakeup.setBounds (cx + (knobSz + padX) * 2, cy, knobSz, 22);

    // ── M/S ───────────────────────────────────────────────────────────────────
    int mx = 512, my = yo + 8;
    lblSectionMS.setBounds (mx, my, 148, secLH); my += secLH + 4;
    btnMsMode.setBounds    (mx, my, 60, 22);     my += 30;
    placeKnob (slMidRatio,  lblMidRatio,  mx,               my);
    placeKnob (slSideRatio, lblSideRatio, mx + knobSz + padX, my);

    // ── Detector + Output ─────────────────────────────────────────────────────
    int dx = 672, dy = yo + 8;
    lblSectionDet.setBounds (dx, dy, 180, secLH); dy += secLH + 4;
    placeKnob (slRmsBlend,   lblRmsBlend,   dx,               dy);
    placeKnob (slStereoLink, lblStereoLink, dx + knobSz + padX, dy);
    dy += knobSz + knobLH + 10;
    placeKnob (slScHPF,     lblScHPF,     dx,               dy);
    placeKnob (slLookahead, lblLookahead, dx + knobSz + padX, dy);
    dy += knobSz + knobLH + 10;

    lblSectionOut.setBounds (dx, dy, 180, secLH); dy += secLH + 4;
    cmbHarmonicMode.setBounds (dx, dy, 142, 22);  dy += 28;
    placeKnob (slHarmonicDrive, lblHarmonicDrive, dx,               dy);
    placeKnob (slCeiling,       lblCeiling,       dx + knobSz + padX, dy);
    dy += knobSz + knobLH + 10;
    cmbOversampling.setBounds (dx, dy, 80, 22);
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::timerCallback()
{
    // Meters
    meterInL.setLevel  (audioProcessor.meterInPeakL.load());
    meterInR.setLevel  (audioProcessor.meterInPeakR.load());
    meterGRMid.setGR   (audioProcessor.meterGRMid.load());
    meterGRSide.setGR  (audioProcessor.meterGRSide.load());
    meterOutL.setLevel (audioProcessor.meterOutPeakL.load());
    meterOutR.setLevel (audioProcessor.meterOutPeakR.load());

    const float tp = audioProcessor.meterTruePeak.load();
    lblTruePeak.setText (juce::String::formatted ("TP: %.1f", tp),
                         juce::dontSendNotification);
    lblTruePeak.setColour (juce::Label::textColourId,
                           tp > -0.5f ? juce::Colours::red : juce::Colours::white);

    // Standalone transport
    if (isStandalone && loadedFileNumSamples > 0)
    {
        const bool playing = audioProcessor.filePlaybackActive.load();
        const int  pos     = audioProcessor.filePlayPosition.load();

        if (playing)
        {
            const double norm     = (double) pos / loadedFileNumSamples;
            const double posSecs  = (double) pos / loadedFileSampleRate;
            const double totSecs  = (double) loadedFileNumSamples / loadedFileSampleRate;

            waveform->setPlayPosition (norm);
            lblPosition.setText (formatSeconds (posSecs) + " / " + formatSeconds (totSecs),
                                 juce::dontSendNotification);
        }
        else if (btnStop.isEnabled())
        {
            // Playback just ended
            btnPlay.setEnabled  (true);
            btnStop.setEnabled  (false);
            waveform->setPlayPosition (0.0);
            lblPosition.setText ("0:00 / " + formatSeconds ((double) loadedFileNumSamples
                                                              / loadedFileSampleRate),
                                 juce::dontSendNotification);
        }
    }
}

//==============================================================================
// File drag-and-drop (editor-level)
bool MasteringCompressorAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray&)
{
    return isStandalone;
}

void MasteringCompressorAudioProcessorEditor::filesDropped (const juce::StringArray& files,
                                                              int, int)
{
    if (files.size() > 0)
        loadFile (juce::File (files[0]));
}

//==============================================================================
void MasteringCompressorAudioProcessorEditor::openLoadDialog()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Open audio file",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3",
        true);

    chooser->launchAsync (juce::FileBrowserComponent::openMode |
                          juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc) {
            if (fc.getResults().isEmpty()) return;
            loadFile (fc.getResult());
        });
}

void MasteringCompressorAudioProcessorEditor::loadFile (const juce::File& file)
{
    stopPlayback();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (! reader) return;

    const int numSamples = (int) reader->lengthInSamples;
    if (numSamples <= 0) return;

    // Load into processor's injection buffer (written on UI thread; safe because
    // fileBufferReady is false so the audio thread won't read it)
    audioProcessor.fileBufferReady.store (false);
    audioProcessor.fileInputBuffer.setSize (2, numSamples, false, true, false);
    audioProcessor.fileInputBuffer.clear();
    reader->read (&audioProcessor.fileInputBuffer, 0, numSamples, 0, true,
                  reader->numChannels > 1);
    audioProcessor.fileInputSampleRate = reader->sampleRate;
    audioProcessor.filePlayPosition.store (0);
    audioProcessor.fileBufferReady.store (true);

    loadedFileSampleRate = reader->sampleRate;
    loadedFileNumSamples = numSamples;

    waveform->setSource (file);
    lblFileName.setText  (file.getFileName(), juce::dontSendNotification);
    lblPosition.setText  ("0:00 / " + formatSeconds ((double) numSamples / reader->sampleRate),
                          juce::dontSendNotification);

    btnPlay.setEnabled   (true);
    btnStop.setEnabled   (false);
    btnExport.setEnabled (true);
}

void MasteringCompressorAudioProcessorEditor::startPlayback()
{
    if (! audioProcessor.fileBufferReady.load()) return;
    audioProcessor.filePlayPosition.store (0);
    audioProcessor.filePlaybackActive.store (true);
    btnPlay.setEnabled (false);
    btnStop.setEnabled (true);
}

void MasteringCompressorAudioProcessorEditor::stopPlayback()
{
    audioProcessor.filePlaybackActive.store (false);
    audioProcessor.filePlayPosition.store  (0);
    btnPlay.setEnabled (audioProcessor.fileBufferReady.load());
    btnStop.setEnabled (false);
    if (waveform) waveform->setPlayPosition (0.0);
}

void MasteringCompressorAudioProcessorEditor::exportToFile()
{
    if (! audioProcessor.fileBufferReady.load()) return;

    auto chooser = std::make_shared<juce::FileChooser> (
        "Export processed audio as WAV",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory)
            .getChildFile ("processed.wav"),
        "*.wav", true);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode |
                          juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser] (const juce::FileChooser& fc) {
            if (fc.getResults().isEmpty()) return;
            doOfflineExport (fc.getResult().withFileExtension ("wav"));
        });
}

void MasteringCompressorAudioProcessorEditor::doOfflineExport (const juce::File& outFile)
{
    btnExport.setEnabled (false);
    exportProgress = 0.0;
    progressBar.setVisible (true);
    resized();

    const int    totalSamples = audioProcessor.fileInputBuffer.getNumSamples();
    const double sr           = loadedFileSampleRate;
    const int    blockSize    = 512;

    // Fresh offline processor — no conflict with the live audio thread
    MasteringCompressorAudioProcessor offlineProc;
    offlineProc.apvts.replaceState (audioProcessor.apvts.copyState());
    offlineProc.prepareToPlay (sr, blockSize);
    const int latency     = offlineProc.getLatencySamples();
    const int feedSamples = totalSamples + latency;

    juce::AudioBuffer<float> outBuf (2, totalSamples);
    outBuf.clear();

    juce::AudioBuffer<float> block (2, blockSize);
    juce::MidiBuffer         midi;

    int inputPos  = 0;
    int outputPos = -latency;

    while (inputPos < feedSamples)
    {
        const int n = std::min (blockSize, feedSamples - inputPos);
        block.setSize (2, n, false, true, true);

        for (int ch = 0; ch < 2; ++ch)
        {
            const int avail = std::max (0, std::min (n, totalSamples - inputPos));
            if (avail > 0)
                block.copyFrom (ch, 0, audioProcessor.fileInputBuffer,
                                ch, inputPos, avail);
        }

        offlineProc.processBlock (block, midi);

        const int destStart = std::max (0, outputPos);
        const int srcStart  = std::max (0, -outputPos);
        const int copyN     = std::min (n - srcStart, totalSamples - destStart);

        if (copyN > 0)
            for (int ch = 0; ch < 2; ++ch)
                outBuf.copyFrom (ch, destStart, block, ch, srcStart, copyN);

        inputPos  += n;
        outputPos += n;
        exportProgress = (double) inputPos / feedSamples;
        progressBar.repaint();
    }

    // Write WAV (24-bit)
    juce::WavAudioFormat wav;
    auto stream = std::make_unique<juce::FileOutputStream> (outFile);
    if (stream->openedOk())
    {
        stream->setPosition (0);
        stream->truncate();
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
        if (writer)
        {
            stream.release(); // writer owns stream now
            writer->writeFromAudioSampleBuffer (outBuf, 0, totalSamples);
        }
    }

    exportProgress = -1.0;
    progressBar.setVisible (false);
    btnExport.setEnabled (true);

    juce::AlertWindow::showMessageBoxAsync (
        juce::AlertWindow::InfoIcon,
        "Export Complete",
        "Saved to:\n" + outFile.getFullPathName());
}

juce::String MasteringCompressorAudioProcessorEditor::formatSeconds (double secs) const
{
    const int m = (int) secs / 60;
    const int s = (int) secs % 60;
    return juce::String::formatted ("%d:%02d", m, s);
}
