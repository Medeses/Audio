/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
	// so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
	return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName(int index)
{
	return {};
}

void SimpleEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;

	spec.numChannels = 1;

	spec.sampleRate = sampleRate;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	updateFilters();
}

void SimpleEQAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void SimpleEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	updateFilters();

	juce::dsp::AudioBlock<float> block(buffer);

	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
	return new SimpleEQAudioProcessorEditor(*this);
	//return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.

	juce::MemoryOutputStream mos(destData, true);
	apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid() )
	{
		apvts.replaceState(tree);
		updateFilters();
	}
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings settings;

	// HighPass
	settings.highPassFreq = apvts.getRawParameterValue("HighPass Freq")->load();
	settings.highPassSlope = static_cast<Slope>(apvts.getRawParameterValue("HighPass Slope")->load());
	// LowPass
	settings.lowPassFreq = apvts.getRawParameterValue("LowPass Freq")->load();
	settings.lowPassSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());
	// LowShelf
	settings.lowShelfFreq = apvts.getRawParameterValue("LowShelf Freq")->load();
	settings.lowShelfGainInDecibels = apvts.getRawParameterValue("LowShelf Gain")->load();
	settings.lowShelfQ = apvts.getRawParameterValue("LowShelf Q")->load();
	// HighShelf
	settings.highShelfFreq = apvts.getRawParameterValue("HighShelf Freq")->load();
	settings.highShelfGainInDecibels = apvts.getRawParameterValue("HighShelf Gain")->load();
	settings.highShelfQ = apvts.getRawParameterValue("HighShelf Q")->load();
	// Peak 1
	settings.peakFreq[0] = apvts.getRawParameterValue("Peak 1 Freq")->load();
	settings.peakGainInDecibels[0] = apvts.getRawParameterValue("Peak 1 Gain")->load();
	settings.peakQ[0] = apvts.getRawParameterValue("Peak 1 Q")->load();
	// Peak 2
	settings.peakFreq[1] = apvts.getRawParameterValue("Peak 2 Freq")->load();
	settings.peakGainInDecibels[1] = apvts.getRawParameterValue("Peak 2 Gain")->load();
	settings.peakQ[1] = apvts.getRawParameterValue("Peak 2 Q")->load();
	// Peak 3
	settings.peakFreq[2] = apvts.getRawParameterValue("Peak 3 Freq")->load();
	settings.peakGainInDecibels[2] = apvts.getRawParameterValue("Peak 3 Gain")->load();
	settings.peakQ[2] = apvts.getRawParameterValue("Peak 3 Q")->load();
	
	return settings;
}

// Update Coefficients
void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
	*old = *replacements;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate, int filterNr)
{
	return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		sampleRate,
		chainSettings.peakFreq[filterNr],
		chainSettings.peakQ[filterNr],
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels[filterNr]));
}

// Peak Filters
void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings, int filterNr)
{
	auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate(), filterNr);

	if (filterNr == 0)
	{
		updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peakCoefficients);
		updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peakCoefficients);
	}
	else if (filterNr == 1)
	{
		updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peakCoefficients);
		updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peakCoefficients);
	}
	else if (filterNr == 2)
	{
		updateCoefficients(leftChain.get<ChainPositions::Peak3>().coefficients, peakCoefficients);
		updateCoefficients(rightChain.get<ChainPositions::Peak3>().coefficients, peakCoefficients);
	}

}

// High Pass Filter
void SimpleEQAudioProcessor::updateHighPassFilters(const ChainSettings& chainSettings)
{
	auto highPassCoefficients = makeHighPassFilter(chainSettings, getSampleRate());

	auto& leftHighPass = leftChain.get<ChainPositions::HighPass>();
	auto& rightHighPass = rightChain.get<ChainPositions::HighPass>();
	updateCutFilter(leftHighPass, highPassCoefficients, chainSettings.highPassSlope);
	updateCutFilter(rightHighPass, highPassCoefficients, chainSettings.highPassSlope);
}

// Low Pass Filter
void SimpleEQAudioProcessor::updateLowPassFilters(const ChainSettings& chainSettings)
{	
	auto lowPassCoefficients = makeLowPassFilter(chainSettings, getSampleRate());

	auto& leftLowPass = leftChain.get<ChainPositions::LowPass>();
	auto& rightLowPass = rightChain.get<ChainPositions::LowPass>();
	updateCutFilter(leftLowPass, lowPassCoefficients, chainSettings.lowPassSlope);
	updateCutFilter(rightLowPass, lowPassCoefficients, chainSettings.lowPassSlope);
}

Coefficients makeLowShelfFilter(const ChainSettings& chainSettings, double sampleRate)
{
	return juce::dsp::IIR::Coefficients<float>::makeLowShelf(
		sampleRate,
		chainSettings.lowShelfFreq,
		chainSettings.lowShelfQ,
		juce::Decibels::decibelsToGain(chainSettings.lowShelfGainInDecibels));
}

// Low Shelf Filter
void SimpleEQAudioProcessor::updateLowShelfFilters(const ChainSettings& chainSettings)
{	
	auto lowShelfCoefficients = makeLowShelfFilter(chainSettings, getSampleRate());

	updateCoefficients(leftChain.get<ChainPositions::LowShelf>().coefficients, lowShelfCoefficients);
	updateCoefficients(rightChain.get<ChainPositions::LowShelf>().coefficients, lowShelfCoefficients);
}

Coefficients makeHighShelfFilter(const ChainSettings& chainSettings, double sampleRate)
{
	return juce::dsp::IIR::Coefficients<float>::makeHighShelf(
		sampleRate,
		chainSettings.highShelfFreq,
		chainSettings.highShelfQ,
		juce::Decibels::decibelsToGain(chainSettings.highShelfGainInDecibels));
}

// High Shelf
void SimpleEQAudioProcessor::updateHighShelfFilters(const ChainSettings& chainSettings)
{
	auto highShelfCoefficients = makeHighShelfFilter(chainSettings, getSampleRate());

	updateCoefficients(leftChain.get<ChainPositions::HighShelf>().coefficients, highShelfCoefficients);
	updateCoefficients(rightChain.get<ChainPositions::HighShelf>().coefficients, highShelfCoefficients);
}

void SimpleEQAudioProcessor::updateFilters()
{
	auto chainSettings = getChainSettings(apvts);

	updateHighPassFilters(chainSettings);
	updateLowShelfFilters(chainSettings);
	updatePeakFilter(chainSettings, 0);
	updatePeakFilter(chainSettings, 1);
	updatePeakFilter(chainSettings, 2);
	updateHighShelfFilters(chainSettings);
	updateLowPassFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	// HighPass Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"HighPass Freq",
		"HighPass Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		20.f));

	// LowPass Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"LowPass Freq",
		"LowPass Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		20000.f));

	// LowShelf Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"LowShelf Freq",
		"LowShelf Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		200.f));

	// LowShelf Gain
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"LowShelf Gain",
		"LowShelf Gain",
		juce::NormalisableRange<float>(-24.f, 24.f, 0.2f, 1.f),
		0.0f));

	// LowShelf Q
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"LowShelf Q",
		"LowShelf Q",
		juce::NormalisableRange<float>(0.1f, 5.f, 0.05f, 0.5f),
		1.f));

	// Peak 1 Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 1 Freq",
		"Peak 1 Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		250.f));

	// Peak 1 Gain
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 1 Gain",
		"Peak 1 Gain",
		juce::NormalisableRange<float>(-24.f, 24.f, 0.2f, 1.f),
		0.0f));

	// Peak 1 Q
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 1 Q",
		"Peak 1 Q",
		juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f),
		1.f));

	// Peak 2 Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 2 Freq",
		"Peak 2 Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		720.f));

	// Peak 2 Gain
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 2 Gain",
		"Peak 2 Gain",
		juce::NormalisableRange<float>(-24.f, 24.f, 0.2f, 1.f),
		0.0f));

	// Peak 2 Q
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 2 Q",
		"Peak 2 Q",
		juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f),
		1.f));

	// Peak 3 Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 3 Freq",
		"Peak 3 Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		2000.f));

	// Peak 3 Gain
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 3 Gain",
		"Peak 3 Gain",
		juce::NormalisableRange<float>(-24.f, 24.f, 0.2f, 1.f),
		0.0f));

	// Peak 3 Q
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"Peak 3 Q",
		"Peak 3 Q",
		juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 0.5f),
		1.f));

	// HighShelf Freq
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"HighShelf Freq",
		"HighShelf Freq",
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
		2000.f));

	// HighShelf Gain
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"HighShelf Gain",
		"HighShelf Gain",
		juce::NormalisableRange<float>(-24.f, 24.f, 0.2f, 1.f),
		0.0f));

	// HighShelf Q
	layout.add(std::make_unique < juce::AudioParameterFloat >(
		"HighShelf Q",
		"HighShelf Q",
		juce::NormalisableRange<float>(0.1f, 5.f, 0.05f, 0.5f),
		1.f));

	// LowPass/HighPass Options
	juce::StringArray stringArray;
	for (int i = 0; i < 4; i++)
	{
		juce::String str;
		str << (12 + i * 12);
		str << " db/Oct";
		stringArray.add(str);
	}

	// HighPass Slope
	layout.add(std::make_unique < juce::AudioParameterChoice >(
		"HighPass Slope",
		"HighPass Slope",
		stringArray,
		0));

	// LowPass Slope
	layout.add(std::make_unique < juce::AudioParameterChoice >(
		"LowPass Slope",
		"LowPass Slope",
		stringArray,
		0));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new SimpleEQAudioProcessor();
}
