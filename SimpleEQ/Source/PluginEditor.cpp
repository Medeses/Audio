/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    setSize (1000, 600);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    /*g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);*/
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