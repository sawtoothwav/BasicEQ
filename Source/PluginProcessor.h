/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// A structure to hold settings for a processing chain.
struct ChainSettings
{
    float peakFrequency { 0 };
    float peakGainDecibels { 0 };
    float peakQuality { 1.f };
    float lowCutFrequency { 0 };
    float highCutFrequency { 0 };
    int lowCutSlope { 0 };
    int highCutSlope { 0 };
};

// A function to return a settings struct, given an APVTS.
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class BasicEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BasicEQAudioProcessor();
    ~BasicEQAudioProcessor() override;

    //==============================================================================
    // Called by host when playback is about to begin.
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    /* Called by host when the play button is hit.
     * If this is interrupted, it can cause pops or other problems.
     */
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Static because it does not use any member variables; this provides a list of all the parameters for the APVTS.
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    // This is the APVTS
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
    // Here we will declare some type aliases to simplify namespaces & definitions.
private:
    
    /* This is a filter alias for processing floats. The filters in the IIR class have a response of 12dB/oct when
     * configured as a low / high pass filter.
     */
    using Filter = juce::dsp::IIR::Filter<float>;
    
    /* This is an alias for a chain of 4 of the above filters; you need one filter per 12dB, so a chain of 4 filters
     * = a shift of 48dB/oct.
     * These aliases are flexible and could be used for a variety of different filters.
     */
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    
    /* This chain represents the whole mono signal path; a high pass filter, a peak filter, and a low pass filter.
     * For stereo processing, we can use 2 instances of this mono chain.
     */
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    // Declaring two chains for stereo processing; one per channel.
    MonoChain leftChain, rightChain;
    
    // An enum to store the indexes of different links in the chain for ease of use.
    enum ChainPositions {
        LowCut,
        Peak,
        HighCut
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicEQAudioProcessor)
};
