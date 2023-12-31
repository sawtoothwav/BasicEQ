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
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
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

juce::AudioProcessorValueTreeState::ParameterLayout
    BasicEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
        /* Low Cut Frequency
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 20Hz (Min of human hearing).
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", 
                                                               "LowCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20.f));
        
        /* High Cut Frequency
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 20,000Hz (Max of human hearing).
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                               "HighCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               20000.f));
        
        /* Peak Frequency - Adjust decibels around a certain frequency.
         * Range: 20Hz - 20,000Hz. Incremented by 1Hz, no skew. Default value of 750Hz
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                               "Peak Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                               750.f));
        
        /* Peak Gain - The amount to adjust the decibels around our peak frequency.
         * Range: -24dB to 24dB. Incremented by 0.1dB, no skew. Default value of 0dB (flat)
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                               "Peak Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f),
                                                               0.0f));
        
        /* Peak Quality - The "width" of the peak. A higher quality represents a thinner peak.
         * Range: 0.1 to 10. Incremented by 0.1, no skew. Default value of 1.
         */
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                               "Peak Gain",
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
        layout.add(std::make_unique<juce::AudioParameterChoice>("Low Cut Slope", "Low Cut Slope", filterArray, 0));
        
        /* High Cut Slope - The slope for our high cut filter.
         * Uses the filterArray to choose options and has a default starting index of 0 (12 dB/Oct).
         */
        layout.add(std::make_unique<juce::AudioParameterChoice>("Low Cut Slope", "Low Cut Slope", filterArray, 0));
        
        // These parameters are all added to our APVTS by the createParameterLayout call in PluginProcessor.h
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicEQAudioProcessor();
}
