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
	//return new SimpleEQAudioProcessorEditor(*this);
	return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

void SimpleEQAudioProcessor::updatePeakFilter1(const ChainSettings& chainSettings)
{
	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		getSampleRate(),
		chainSettings.peakFreq1,
		chainSettings.peakQ1,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels1));

	updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peakCoefficients);
	updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peakCoefficients);
}

void SimpleEQAudioProcessor::updatePeakFilter2(const ChainSettings& chainSettings)
{
	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		getSampleRate(),
		chainSettings.peakFreq2,
		chainSettings.peakQ2,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels2));

	updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peakCoefficients);
	updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peakCoefficients);
}

void SimpleEQAudioProcessor::updatePeakFilter3(const ChainSettings& chainSettings)
{
	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		getSampleRate(),
		chainSettings.peakFreq3,
		chainSettings.peakQ3,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels3));

	updateCoefficients(leftChain.get<ChainPositions::Peak3>().coefficients, peakCoefficients);
	updateCoefficients(rightChain.get<ChainPositions::Peak3>().coefficients, peakCoefficients);
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings settings;

	settings.highPassFreq = apvts.getRawParameterValue("HighPass Freq")->load();
	settings.lowPassFreq = apvts.getRawParameterValue("LowPass Freq")->load();
	settings.peakFreq1 = apvts.getRawParameterValue("Peak 1 Freq")->load();
	settings.peakGainInDecibels1 = apvts.getRawParameterValue("Peak 1 Gain")->load();
	settings.peakQ1 = apvts.getRawParameterValue("Peak 1 Q")->load();
	settings.peakFreq2 = apvts.getRawParameterValue("Peak 2 Freq")->load();
	settings.peakGainInDecibels2 = apvts.getRawParameterValue("Peak 2 Gain")->load();
	settings.peakQ2 = apvts.getRawParameterValue("Peak 2 Q")->load();
	settings.peakFreq3 = apvts.getRawParameterValue("Peak 3 Freq")->load();
	settings.peakGainInDecibels3 = apvts.getRawParameterValue("Peak 3 Gain")->load();
	settings.peakQ3 = apvts.getRawParameterValue("Peak 3 Q")->load();
	settings.highPassSlope = static_cast<Slope>(apvts.getRawParameterValue("HighPass Slope")->load());
	settings.lowPassSlope = static_cast<Slope>(apvts.getRawParameterValue("LowPass Slope")->load());

	return settings;
}

void SimpleEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
	*old = *replacements;
}

void SimpleEQAudioProcessor::updateHighPassFilters(const ChainSettings& chainSettings)
{
	// High Pass Filter
	auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
		chainSettings.highPassFreq,
		getSampleRate(),
		2 * (chainSettings.highPassSlope + 1));

	auto& leftHighPass = leftChain.get<ChainPositions::HighPass>();
	auto& rightHighPass = rightChain.get<ChainPositions::HighPass>();
	updateCutFilter(leftHighPass, cutCoefficients, chainSettings.highPassSlope);
	updateCutFilter(rightHighPass, cutCoefficients, chainSettings.highPassSlope);
}

void SimpleEQAudioProcessor::updateLowPassFilters(const ChainSettings& chainSettings)
{
	// Low Pass
	auto LowPassCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
		chainSettings.lowPassFreq,
		getSampleRate(),
		2 * (chainSettings.lowPassSlope + 1));

	auto& leftLowPass = leftChain.get<ChainPositions::LowPass>();
	auto& rightLowPass = rightChain.get<ChainPositions::LowPass>();
	updateCutFilter(leftLowPass, LowPassCoefficients, chainSettings.lowPassSlope);
	updateCutFilter(rightLowPass, LowPassCoefficients, chainSettings.lowPassSlope);
}

void SimpleEQAudioProcessor::updateFilters()
{
	auto chainSettings = getChainSettings(apvts);

	updateHighPassFilters(chainSettings);
	updatePeakFilter1(chainSettings);
	updatePeakFilter2(chainSettings);
	updatePeakFilter3(chainSettings);
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
