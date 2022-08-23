/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }
};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    CustomRotarySlider
        peak1FreqSlider,
        peak1GainSlider,
        peak1QSlider,
        peak2FreqSlider,
        peak2GainSlider,
        peak2QSlider,
        peak3FreqSlider,
        peak3GainSlider,
        peak3QSlider,
        highPassFreqSlider,
        highPassSlopeSlider,
        lowPassFreqSlider,
        lowPassSlopeSlider,
        lowShelfFreqSlider,
        lowShelfGainSlider,
        lowShelfQSlider,
        highShelfFreqSlider,
        highShelfGainSlider,
        highShelfQSlider;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
