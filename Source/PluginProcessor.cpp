/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicEQAudioProcessor::BasicEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

BasicEQAudioProcessor::~BasicEQAudioProcessor()
{
}

//==============================================================================
const juce::String BasicEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BasicEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BasicEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BasicEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BasicEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BasicEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BasicEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BasicEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BasicEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void BasicEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BasicEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    /* We must prepare any filters before we use them; this is done by using a process
     * spec object for the chains.
     */
    juce::dsp::ProcessSpec spec;
    
    // The spec needs to know the maximum amount of samples it'll process at once.
    spec.maximumBlockSize = samplesPerBlock;
    
    // The spec needs to know the number of channels
    spec.numChannels = 1;
    
    // The spec needs to know the sample rate.
    spec.sampleRate = sampleRate;
    
    // Now we can prepare the left & right mono chains using the spec.
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    // Get our chain settings.
    auto chainSettings = getChainSettings(apvts);
    
    /* Create a coefficient set for our peak filter.
     * The gain parameter expects gain in gain units, not dB, so we will have to convert.
     */
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                                chainSettings.peakFrequency,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainDecibels));
    
    /* You can access links in the chain through the get() function, using an index.
     * We will use an enum to make this easier to use.
     *
     * The coefficients objects are reference-counted objects that each own a juce::Array<float>.
     * The helpfer functions return the instances on the heap, so they must be dereferenced to get
     * the underlying coefficients.
     * Allocating on the heap in an audio callback is generally not a good idea.
     *
     * This step is where audible changes begin to occur, but in order for the plugin to work when
     * changing sliders, we have to have it update these coefficients when the sliders move.
     */
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void BasicEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
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

// This is where you get allocated audio data (buffer) and can work with midi messages.
void BasicEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    /* The processor chain requires a processing context in order to properly
     * run the audio through each link in the chain. To make the context,
     * we need to provide an audio block instance.
     * The processBlock function is called by the host and given a buffer
     * which can have any number of channels; we need to find the left and right
     * channel from this buffer. Left and Right are typically channels 0 and 1,
     * respectively.
     */
    
    // First the block; initialized for processing floats.
    juce::dsp::AudioBlock<float> block(buffer);
    
    // Now we can use helper functions to extract individual channels, wrapped in their own blocks.
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    // Now we set up our processing contexts for each channel's block.
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    // Now we provide these contexts to the filter chains.
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool BasicEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BasicEQAudioProcessor::createEditor()
{
    //return new BasicEQAudioProcessorEditor (*this);
    
    // Before we've added our parameters to the GUI, we can use an Audio Processor Editor to view and edit them.
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BasicEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BasicEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;
    
    /* We don't use the getValue() function here because it returns normalised values,
     * but our program expects real-world values. We will instead use getRawParameterValue().
     * These values are atomic, thanks to the load() call.
     */
    
    settings.lowCutFrequency  = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFrequency = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFrequency    = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality      = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope      = apvts.getRawParameterValue("LowCut Slope")->load();
    settings.highCutSlope     = apvts.getRawParameterValue("HighCut Slope")->load();
    
    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
    BasicEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
        /* Low Cut Frequency
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 20Hz (Min of human hearing).
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"LowCut Freq", 1},
                                                               "LowCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20.f));
        
        /* High Cut Frequency
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 20,000Hz (Max of human hearing).
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"HighCut Freq", 1},
                                                               "HighCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20000.f));
        
        /* Peak Frequency - Adjust decibels around a certain frequency.
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 750Hz
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"Peak Freq", 1},
                                                               "Peak Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               750.f));
        
        /* Peak Gain - The amount to adjust the decibels around our peak frequency.
         * Range: -24dB to 24dB. Incremented by 0.1dB, no skew. Default value of 0dB (flat)
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"Peak Gain", 1},
                                                               "Peak Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f),
                                                               0.0f));
        
        /* Peak Quality - The "width" of the peak. A higher quality represents a thinner peak.
         * Range: 0.1 to 10. Incremented by 0.1, no skew. Default value of 1.
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"Peak Quality", 1},
                                                               "Peak Quality",
                                                               juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                               1.f));
        
        /* Filter Array: AudioParameterChoice objects use an array to store the options.
         * Due to how the math works out (still learning about this) you want the slopes of
         * high / low pass filters to be factors of 6 or 12 dB/Oct for best results.
         */
        juce::StringArray filterArray;
        for(int i = 0; i < 4; ++i) {
            juce::String str;
            // Start at 12dB, then 24dB, then 36dB...
            str << (12 + i*12);
            // Concat " db/Oct" to the end of the string; C++ overloads << and >>, making them tools for working w/ strings & streams.
            str << " dB/Oct";
            filterArray.add(str);
        }
        
        /* Low Cut Slope - The slope for our low cut filter.
         * Uses the filterArray to choose options and has a default starting index of 0 (12 dB/Oct).
         */
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"LowCut Slope", 1},
                                                                "LowCut Slope",
                                                                filterArray, 0));
        
        /* High Cut Slope - The slope for our high cut filter.
         * Uses the filterArray to choose options and has a default starting index of 0 (12 dB/Oct).
         */
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"HighCut Slope", 1},
                                                                "HighCut Slope",
                                                                filterArray, 0));
        
        // These parameters are all added to our APVTS by the createParameterLayout call in PluginProcessor.h
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicEQAudioProcessor();
}
