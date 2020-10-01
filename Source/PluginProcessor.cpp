/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IDs.h"

//==============================================================================
ExperimentalFilterAudioProcessor::ExperimentalFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters(*this, nullptr, juce::Identifier("ExperimentalFilter"),
                  {
                      std::make_unique<juce::AudioParameterFloat> (ParameterID[kParameter_SubtractionStrength],      // parameterID
                                                                   ParameterLabel[kParameter_SubtractionStrength],   // parameter name
                                                                   0.0f,                                             // minimum value
                                                                   5.0f,                                             // maximum value
                                                                   0.0f)                                             // default value
                  }),
       mSpectrogramMaker(2048, 512)
       // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif
{
    initializeDSP();
    
    test.add(1.0f, 2.0f);
    
    // Set up the ValueTree that holds mNoiseSpectrum
    ValueTree child{IDs::audioData};                                                        // Create a node
    parameters.state.addChild(child, 0, nullptr);                                           // Add node to root ValueTree

//    nonParmStringVal.referTo(child.getPropertyAsValue(IDs::noiseSpectrumID, nullptr));      // Make nonParmStringVal refer to the IDs::noiseSpectrumID property inside child so changes to one are reflected by both
    
    mSubtractionStrengthParameter = parameters.getRawParameterValue(ParameterID[kParameter_SubtractionStrength]);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    mNoiseBuffer = std::make_unique<AudioSampleBuffer>(0, 0);
    mNoiseBuffer->clear();
    
    mPosition = std::unique_ptr<int>(new int (0));
    
//    for (int i = 0; i < 2; i++) {
//        for (int j = 0; j < 2 * kFFTSize; j++) {
//            if (j < kFFTSize) {
//                mFileBufferFifo[i][j] = std::unique_ptr<float>(new float(0));
//            }
//            mFileBufferFFTData[i][j] = std::unique_ptr<float>(new float(0));
//        }
//    }
//    mFileBufferFifoIndex = std::unique_ptr<int>(new int (0));
}

ExperimentalFilterAudioProcessor::~ExperimentalFilterAudioProcessor()
{
}

//==============================================================================
const String ExperimentalFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ExperimentalFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ExperimentalFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ExperimentalFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ExperimentalFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ExperimentalFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ExperimentalFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ExperimentalFilterAudioProcessor::setCurrentProgram (int index)
{
}

const String ExperimentalFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void ExperimentalFilterAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void ExperimentalFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    phase = 0;
    freq = 440.0;       // for creating a 440 Hz sine wave
    phaseDelta = freq/sampleRate; // in radians
    
    // Determine FFT order
    int minWindowLength = MINBLOCKSPERWINDOW * samplesPerBlock;
    int order = 0;
    int windowLength = 1;
    while (windowLength < minWindowLength) {
        order++;
        windowLength *= 2;
    }
    
//    mFilter.updateParameters((int) paramFftSize.getTargetValue(),
//                             (int) paramHopSize.getTargetValue(),
//                             (int) paramWindowType.getTargetValue());
}

void ExperimentalFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    mNoiseBuffer->clear();
    mNoiseBuffer->setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ExperimentalFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if c input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ExperimentalFilterAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
//    nonParmStringVal = "value changed";
//    DBG(nonParmStringVal.toString());
//    DBG((parameters.state.getChild(0).getProperty(IDs::noiseSpectrumID)).toString());
    
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    mFilter.processBlock(buffer, mNoiseSpectrum, *mSubtractionStrengthParameter);
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // If mNoiseBuffer doesn’t hold a valid buffer of data.
    /** if (mNoiseBuffer->getNumChannels() == 0 || mNoiseBuffer->getNumSamples() == 0 || *mPosition >= mNoiseBuffer->getNumSamples()) {
        buffer.clear();
        return;
    }
    
    // Use the getNextAudioBlock() function to fill buffer
    AudioSourceChannelInfo bufferToFill;
    bufferToFill.buffer = &buffer;
    bufferToFill.startSample = 0;
    bufferToFill.numSamples = buffer.getNumSamples();
    getNextAudioBlock(bufferToFill); */
    
//    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
//        float* channelData = buffer.getWritePointer(channel);
//        mFilters[channel]->processBlock(channelData, channelData, buffer.getNumSamples());
//    }
}

// Performs a forward FFT on the signal then an inverse FFT on it to recover the original signal.
/** void ExperimentalFilterAudioProcessor::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
    jassert(bufferToFill.numSamples == 512);
    int numSamples = bufferToFill.numSamples;

    // Create a FFT object for the forward transform and a FFT object for the inverse transform.
    // The number of points the FFT objects will operate on is 2^9 = 512.
    dsp::FFT forwardFFT(9);
    dsp::FFT reverseFFT(9);

    // DBG(forwardFFT.getSize());       // returns 512

    // Iterate over all channels
    for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); ++chan) {
        float* const channelData = bufferToFill.buffer->getWritePointer(chan, bufferToFill.startSample);
        
        // The size of the array passed into performRealOnlyForwardTransform() must be 2 * forwardFFT.getSize()
        float tempData[2 * forwardFFT.getSize()];
        // Fill the first half of tempData with the signal in channelData
        std::memcpy(tempData, channelData, numSamples * sizeof(float));

        forwardFFT.performRealOnlyForwardTransform(tempData, true);     // in-place forward FFT
        reverseFFT.performRealOnlyInverseTransform(tempData);           // in-place inverse FFT

        // Fill channelData with the recovered original signal.
        std::memcpy(channelData, tempData, numSamples * sizeof(float));
    }
 } */

// Generates a sine signal, performs a forward FFT on it then an inverse FFT on it to recover the original signal.
/** void ExperimentalFilterAudioProcessor::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) {
    jassert(bufferToFill.numSamples == 512);
    int numSamples = bufferToFill.numSamples;
    
    // Create a FFT object for the forward transform and a FFT object for the inverse transform.
    // The number of points the FFT objects will operate on is 2^9 = 512.
    dsp::FFT forwardFFT(9);
    dsp::FFT reverseFFT(9);
    
    // DBG(forwardFFT.getSize());       // returns 512
    
    float level = 0.5;      // volume level
    double initialPhase = phase;
    // Iterate over all channels
    for (int chan = 0; chan < bufferToFill.buffer->getNumChannels(); ++chan) {
        // phase will have to be reset to its initial value in the function for every channel, that is,
        // phase should evolve the same way across all channels.
        phase = initialPhase;
        
        // The size of the array passed into performRealOnlyForwardTransform() must be 2 * forwardFFT.getSize()
        float sineData[2 * forwardFFT.getSize()];
        // Fill the first half of sineData with a sine time series signal.
        for (int i = 0; i < numSamples; ++i)
        {
            sineData[i] = (float) std::sin(double_Pi * 2.0 * phase) * level;
            phase += phaseDelta;
        }
        
        forwardFFT.performRealOnlyForwardTransform(sineData, true);     // in-place forward FFT
        reverseFFT.performRealOnlyInverseTransform(sineData);           // in-place inverse FFT
        
        float* const channelData = bufferToFill.buffer->getWritePointer(chan, bufferToFill.startSample);
        // Fill channelData with the sine signal.
        std::memcpy(channelData, sineData, numSamples * sizeof(float));
    }
} */

void ExperimentalFilterAudioProcessor::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) {
    auto numInputChannels = mNoiseBuffer->getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    
    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = bufferToFill.startSample;
    
    while (outputSamplesRemaining > 0) {
        auto bufferSamplesRemaining = mNoiseBuffer->getNumSamples() - *mPosition;
        auto samplesThisTime = jmin(outputSamplesRemaining, bufferSamplesRemaining);
        
        for (auto channel = 0; channel < numOutputChannels; ++channel) {
            bufferToFill.buffer->copyFrom (channel,
                                           outputSamplesOffset,
                                           *mNoiseBuffer,
                                           channel % numInputChannels,
                                           *mPosition,
                                           samplesThisTime);
        }
        
        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        *mPosition += samplesThisTime;
        
        if (*mPosition >= mNoiseBuffer->getNumSamples()) {
            return;
        }
    }
}

void ExperimentalFilterAudioProcessor::storeNoiseSpectrum(const AudioSampleBuffer& noiseSignal) {
    Spectrogram spectrogram;
    mSpectrogramMaker.perform(noiseSignal, spectrogram);
    averageSpectrum(spectrogram, mNoiseSpectrum, 2048);
    // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

//==============================================================================
bool ExperimentalFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* ExperimentalFilterAudioProcessor::createEditor()
{
    return new ExperimentalFilterAudioProcessorEditor (*this);
}

//==============================================================================
// Store the plugin's state in an XML object
void ExperimentalFilterAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    File file("/Users/zach/Desktop/toDAW.xml");
    
    if (mNoiseSpectrum != nullptr) {
        var mNoiseSpectrumAsString = var(varArrayToDelimitedString(heapBlockToArray(mNoiseSpectrum)));
        parameters.state.getChild(0).setProperty(IDs::noiseSpectrumID, mNoiseSpectrumAsString, nullptr);
    }

//    parameters.state.getChild(0).setProperty(IDs::noiseSpectrumID, var(heapBlockToArray(mNoiseSpectrum)), nullptr);
    
    ValueTree state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());      // Creates an XmlElement with a tag name of "ExperimentalFilter" that holds a complete image of state and all its children
    copyXmlToBinary (*xml, destData);
    
    if (xml->writeTo(file, XmlElement::TextFormat())) DBG("toDAW written");
    else DBG("toDAW not written");
}

// Restore the plugin's state from an XML object
void ExperimentalFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    File file("/Users/zach/Desktop/fromDAW.xml");
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
    
            mNoiseSpectrum.realloc(2048);
            mNoiseSpectrum.clear(2048);
            Array<var> mNoiseSpectrumAsArray = delimitedStringToVarArray(parameters.state.getChild(0).getProperty(IDs::noiseSpectrumID).toString());
            arrayToHeapBlock(mNoiseSpectrumAsArray, mNoiseSpectrum);
//            memcpy(mNoiseSpectrum, &mNoiseSpectrumAsArray, 2048 * sizeof(float));
            DBG(mNoiseSpectrum[0]);
            // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
            if (xmlState->writeTo(file, XmlElement::TextFormat())) DBG("fromDAW written");
            else DBG("fromDAW not written");
}

void ExperimentalFilterAudioProcessor::initializeDSP() {
    // Initialize filter
    mFilter.setup(getTotalNumInputChannels());
    mFilter.updateParameters(2048,
                             4,
                             2);
    // ^2048 fft size, 1/4 hop size, Hann window
    
    mNoiseSpectrum.realloc(2048);
    mNoiseSpectrum.clear(2048);
    
    // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    
    
    
    
//    for (int i = 0; i < 2; i++) {
//        mFilters[i] = std::make_unique<Filter>();
//    }
}

Array<var> ExperimentalFilterAudioProcessor::heapBlockToArray(HeapBlock<float>& heapBlock) {
    Array<var> array;
    
    for (int i = 0; i < 2048; ++i) {
        array.add (heapBlock[i]);
    }
    // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return array;
}

void ExperimentalFilterAudioProcessor::arrayToHeapBlock(Array<var>& array, HeapBlock<float>& heapBlock) {
    for (int i = 0; i < 2048; i++) {
        heapBlock[i] = array[i];
    }
    // ^TODO: ALLOW USER TO SELECT PARAMETERS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

String ExperimentalFilterAudioProcessor::varArrayToDelimitedString (const Array<var>& input) {
    // if you are trying to control a var that is an array then you need to
    // set a delimiter string that will be used when writing to XML!
    StringArray elements;

    for (auto& v : input)
        elements.add (v.toString());

    return elements.joinIntoString (",");
}

Array<var> ExperimentalFilterAudioProcessor::delimitedStringToVarArray (StringRef input) {
    Array<var> arr;
    
    for (auto t : StringArray::fromTokens (input, ",", {}))
        arr.add (t);
    
    return arr;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExperimentalFilterAudioProcessor();
}
