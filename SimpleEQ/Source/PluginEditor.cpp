/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    // HighPass
    highPassFreqSliderAttachment(audioProcessor.apvts, "HighPass Freq", highPassFreqSlider),
    highPassSlopeSliderAttachment(audioProcessor.apvts, "HighPass Slope", highPassSlopeSlider),
    // LowShelf
    lowShelfFreqSliderAttachment(audioProcessor.apvts, "LowShelf Freq", lowShelfFreqSlider),
    lowShelfGainSliderAttachment(audioProcessor.apvts, "LowShelf Gain", lowShelfGainSlider),
    lowShelfQSliderAttachment(audioProcessor.apvts, "LowShelf Q", lowShelfQSlider),
    // Peak 1
    peak1FreqSliderAttachment(audioProcessor.apvts, "Peak 1 Freq", peak1FreqSlider),
    peak1GainSliderAttachment(audioProcessor.apvts, "Peak 1 Gain", peak1GainSlider),
    peak1QSliderAttachment(audioProcessor.apvts, "Peak 1 Q", peak1QSlider),
    // Peak 2
    peak2FreqSliderAttachment(audioProcessor.apvts, "Peak 2 Freq", peak2FreqSlider),
    peak2GainSliderAttachment(audioProcessor.apvts, "Peak 2 Gain", peak2GainSlider),
    peak2QSliderAttachment(audioProcessor.apvts, "Peak 2 Q", peak2QSlider),
    // Peak 3
    peak3FreqSliderAttachment(audioProcessor.apvts, "Peak 3 Freq", peak3FreqSlider),
    peak3GainSliderAttachment(audioProcessor.apvts, "Peak 3 Gain", peak3GainSlider),
    peak3QSliderAttachment(audioProcessor.apvts, "Peak 3 Q", peak3QSlider),
    // HighShelf
    highShelfFreqSliderAttachment(audioProcessor.apvts, "HighShelf Freq", highShelfFreqSlider),
    highShelfGainSliderAttachment(audioProcessor.apvts, "HighShelf Gain", highShelfGainSlider),
    highShelfQSliderAttachment(audioProcessor.apvts, "HighShelf Q", highShelfQSlider),
    // LowPass
    lowPassFreqSliderAttachment(audioProcessor.apvts, "LowPass Freq", lowPassFreqSlider),
    lowPassSlopeSliderAttachment(audioProcessor.apvts, "LowPass Slope", lowPassSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60);

    setSize (1000, 600);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto w = responseArea.getWidth();

    auto& highpass = monoChain.get<ChainPositions::HighPass>();
    auto& lowshelf = monoChain.get<ChainPositions::LowShelf>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& peak3 = monoChain.get<ChainPositions::Peak3>();
    auto& highshelf = monoChain.get<ChainPositions::HighShelf>();
    auto& lowpass = monoChain.get<ChainPositions::LowPass>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        // HighPass
        if (!highpass.isBypassed<0>())
            mag *= highpass.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highpass.isBypassed<1>())
            mag *= highpass.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highpass.isBypassed<2>())
            mag *= highpass.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highpass.isBypassed<3>())
            mag *= highpass.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // LowShelf
        if (!monoChain.isBypassed<ChainPositions::LowShelf>())
            mag *= lowshelf.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // Peak 1
        if (!monoChain.isBypassed<ChainPositions::Peak1>())
            mag *= peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // Peak 2
        if (!monoChain.isBypassed<ChainPositions::Peak2>())
            mag *= peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // Peak 3
        if (!monoChain.isBypassed<ChainPositions::Peak3>())
            mag *= peak3.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // HighShelf
        if (!monoChain.isBypassed<ChainPositions::HighShelf>())
            mag *= highshelf.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // LowPass
        if (!lowpass.isBypassed<0>())
            mag *= lowpass.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowpass.isBypassed<1>())
            mag *= lowpass.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowpass.isBypassed<2>())
            mag *= lowpass.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowpass.isBypassed<3>())
            mag *= lowpass.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -30.0, 30.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));



}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    // Set up areas
    auto highPassArea = bounds.removeFromLeft(bounds.getWidth() * 1 / 7);
    auto lowShelfArea = bounds.removeFromLeft(bounds.getWidth() * 1 / 6);
    auto peak1Area = bounds.removeFromLeft(bounds.getWidth() * 1 / 5);
    auto peak2Area = bounds.removeFromLeft(bounds.getWidth() * 1 / 4);
    auto peak3Area = bounds.removeFromLeft(bounds.getWidth() * 1 / 3);
    auto highShelfArea = bounds.removeFromLeft(bounds.getWidth() * 1 / 2);
    auto lowPassArea = bounds.removeFromRight(bounds.getWidth() * 1 / 1);

    // HighPass
    highPassFreqSlider.setBounds(highPassArea.removeFromTop(bounds.getHeight() * 1 / 2));
    highPassSlopeSlider.setBounds(highPassArea);
    // LowShelf
    lowShelfFreqSlider.setBounds(lowShelfArea.removeFromTop(bounds.getHeight() * 1 / 3));
    lowShelfGainSlider.setBounds(lowShelfArea.removeFromTop(bounds.getHeight() * 1 / 3));
    lowShelfQSlider.setBounds(lowShelfArea);
    // Peak 1
    peak1FreqSlider.setBounds(peak1Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak1QSlider.setBounds(peak1Area);
    // Peak 2
    peak2FreqSlider.setBounds(peak2Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak2GainSlider.setBounds(peak2Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak2QSlider.setBounds(peak2Area);
    // Peak 3
    peak3FreqSlider.setBounds(peak3Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak3GainSlider.setBounds(peak3Area.removeFromTop(bounds.getHeight() * 1 / 3));
    peak3QSlider.setBounds(peak3Area);
    // HighShelf
    highShelfFreqSlider.setBounds(highShelfArea.removeFromTop(bounds.getHeight() * 1 / 3));
    highShelfGainSlider.setBounds(highShelfArea.removeFromTop(bounds.getHeight() * 1 / 3));
    highShelfQSlider.setBounds(highShelfArea);
    // LowPass
    lowPassFreqSlider.setBounds(lowPassArea.removeFromTop(bounds.getHeight() * 1 / 2));
    lowPassSlopeSlider.setBounds(lowPassArea);
}

void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void SimpleEQAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        // Update the monochain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        // LowShelf
        auto lowShelfCoefficients = makeLowShelfFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get < ChainPositions::LowShelf>().coefficients, lowShelfCoefficients);
        // Peak 1
        auto peakCoefficients1 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 0);
        updateCoefficients(monoChain.get < ChainPositions::Peak1>().coefficients, peakCoefficients1);
        // Peak 2
        auto peakCoefficients2 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 1);
        updateCoefficients(monoChain.get < ChainPositions::Peak2>().coefficients, peakCoefficients2);
        // Peak 3
        auto peakCoefficients3 = makePeakFilter(chainSettings, audioProcessor.getSampleRate(), 2);
        updateCoefficients(monoChain.get < ChainPositions::Peak3>().coefficients, peakCoefficients3);
        // HighShelf
        auto highShelfCoefficients = makeHighShelfFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get < ChainPositions::HighShelf>().coefficients, highShelfCoefficients);

        // Signal a repaint
        repaint();
    }
}


std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QSlider,
        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QSlider,
        &peak3FreqSlider,
        &peak3GainSlider,
        &peak3QSlider,
        &highPassFreqSlider,
        &highPassSlopeSlider,
        &lowPassFreqSlider,
        &lowPassSlopeSlider,
        &lowShelfFreqSlider,
        &lowShelfGainSlider,
        &lowShelfQSlider,
        &highShelfFreqSlider,
        &highShelfGainSlider,
        &highShelfQSlider
    };
}