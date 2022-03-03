/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//#include "../Tests/test_main.cpp"
#include "../Helper/NonAutoParameter.h"

namespace {
    // One audio channel of FFT data over time, really 2-dimensional
    template <typename FloatType>
    using Spectrogram = std::vector<HeapBlock<FloatType>>;

    // Compute the stft on each channel of signal and average the results to produce one spectrogram.
    template <typename FloatType>
    inline void makeSpectrogram (Spectrogram<FloatType>& spectrogram,
                                 juce::AudioBuffer<FloatType>& signal,
                                 juce::dsp::FFT& fft,
                                 size_t hopSize,
                                 juce::dsp::WindowingFunction<FloatType>& window)
    {
        const size_t dataCount = signal.getNumSamples();
        
        // fftSize will be the number of bins we used to initialize the SpectrogramMaker.
        ptrdiff_t fftSize = fft.getSize();
        
        // Calculate number of hops
        ptrdiff_t numHops = 1L + static_cast<long>((dataCount - fftSize) / hopSize);
        
        // Initialize spectrogram
        spectrogram.resize (numHops + 1);
        for (int i = 0; i < spectrogram.size(); ++i)
        {
            spectrogram[i].realloc (fftSize);
            spectrogram[i].clear (fftSize);
        }
        
        // We will discard the negative frequency bins, but leave the center bin.
        size_t numRows = 1UL + (fftSize / 2UL);
        
        int numChannels = signal.getNumChannels();
        FloatType inverseNumChannels = static_cast<FloatType> (1) / numChannels;
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            // fFft works on the data in place, and needs twice as much space as the input size.
            std::vector<FloatType> fftBuffer (fftSize * 2UL);
        
            // While data remains
            const FloatType* signalData = signal.getReadPointer (channel, 0);
            for (int i = 0; i < numHops; ++i)
            {
                // Prepare segment to perform FFT on.
                std::memcpy (fftBuffer.data(), signalData, fftSize * sizeof (FloatType));
                
                // Apply the windowing to the chunk of samples before passing it to the FFT.
                window.multiplyWithWindowingTable (fftBuffer.data(), fftSize);
                
                // performFrequencyOnlyForwardTransform produces a magnitude frequency response spectrum.
                fft.performFrequencyOnlyForwardTransform (fftBuffer.data());
                
                // Add the positive frequency bins (including the center bin) from fftBuffer to the spectrogram.
                juce::FloatVectorOperations::addWithMultiply (spectrogram[i].get(), fftBuffer.data(), inverseNumChannels, numRows);
                
                // Next chunk
                signalData += hopSize;
            }
        }
    }

    // Calculates the average spectrum from a given spectrogram
    template <typename FloatType>
    inline void computeAverageSpectrum (juce::HeapBlock<FloatType>& magSpectrum, Spectrogram<FloatType>& spectrogram, int fftSize)
    {
        magSpectrum.realloc (fftSize);
        magSpectrum.clear (fftSize);
        
        size_t numColumns = spectrogram.size();
        FloatType inverseNumColumns = static_cast<FloatType> (1) / numColumns;

        // Iterate through frequency bins. We only go up to (fftSize / 2 + 1) in order to ignore the negative frequency bins.
        int numBins = fftSize / 2 + 1;
        for (int freqColumn = 0; freqColumn < numColumns; ++freqColumn)
        {
            juce::FloatVectorOperations::addWithMultiply (magSpectrum.get(), spectrogram[freqColumn].get(), inverseNumColumns, numBins);
        }
    }
}

//==============================================================================
SpectralSubtractorAudioProcessor::SpectralSubtractorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
       juce::Thread ("Noise spectrum processing thread") // thread that calculates a noise file's average spectrum for use in the spectral subtractor

#endif
{
    setParams();
    attachSubTrees();
    
    mSpectralSubtractor.updateParameters (FFTSize[mFFTSizeParam->getIndex()],
                                          WindowOverlap[mWindowOverlapParam->getIndex()],
                                          mWindowParam->getIndex(),
                                          nullptr);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    
    // Ensure proper correspondence between the window combobox items and the Spectral Subtractor class window enums.
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeRectangular] == "Rectangular");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeBartlett] == "Bartlett");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHann] == "Hann");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHamming] == "Hamming");

    startThread (8);    // set priority as high as possible without being realtime audio priority
    
#if RUN_UNIT_TESTS == 1
    std::cout << "Running unit tests..." << std::endl;
    mUnitTestRunner.runAllTests();
#endif
}

SpectralSubtractorAudioProcessor::~SpectralSubtractorAudioProcessor()
{
    stopThread (10000);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralSubtractorAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;
    
    juce::NormalisableRange<float> subtractionStrengthRange {0.0f, 5.0f};
    float subtractionStrengthDefault = 0.0f;
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID[kParameter_SubtractionStrength],
                                                                   ParameterLabel[kParameter_SubtractionStrength],
                                                                   subtractionStrengthRange,
                                                                   subtractionStrengthDefault));
    
    return { params.begin(), params.end() };
}

void SpectralSubtractorAudioProcessor::setParams()
{
    mSubtractionStrengthParam = apvts.getRawParameterValue (ParameterID[kParameter_SubtractionStrength]);
    jassert (mSubtractionStrengthParam);
    mSpectralSubtractor.setSubtractionStrength (mSubtractionStrengthParam);
    
    mFFTSizeParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_FFTSize],
                                                              ParameterLabel[kParameter_FFTSize],
                                                              FFTSizeItemsUI,
                                                              kFFTSize2048);
    mNonAutoParams.emplace (mFFTSizeParam->getParameterID(), mFFTSizeParam.get());
    
    mWindowOverlapParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_WindowOverlap],
                                                                    ParameterLabel[kParameter_WindowOverlap],
                                                                    WindowOverlapItemsUI,
                                                                    kWindowOverlap4);
    mNonAutoParams.emplace (mWindowOverlapParam->getParameterID(), mWindowOverlapParam.get());
    
    mWindowParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_Window],
                                                             ParameterLabel[kParameter_Window],
                                                             WindowTypeItemsUI,
                                                             SpectralSubtractor<float>::kWindowTypeHann);
    mNonAutoParams.emplace (mWindowParam->getParameterID(), mWindowParam.get());
}

void SpectralSubtractorAudioProcessor::attachSubTrees()
{
    apvts.state.appendChild (mFFTSizeParam->getValueTree(), nullptr);
    apvts.state.appendChild (mWindowOverlapParam->getValueTree(), nullptr);
    apvts.state.appendChild (mWindowParam->getValueTree(), nullptr);
    apvts.state.appendChild (mAudioDataTree, nullptr);
}

NonAutoParameterChoice* SpectralSubtractorAudioProcessor::getNonAutoParameterWithID (const String& parameterID)
{
    jassert (mNonAutoParams.contains (parameterID));
    return mNonAutoParams[parameterID];
}

juce::AudioFormatManager* SpectralSubtractorAudioProcessor::getFormatManager()
{
    return mFormatManager.get();
}

int SpectralSubtractorAudioProcessor::getFFTSize()
{
    return FFTSize[mFFTSizeParam->getIndex()];
}

int SpectralSubtractorAudioProcessor::getWindowOverlap()
{
    return WindowOverlap[mWindowOverlapParam->getIndex()];
}

//==============================================================================
const String SpectralSubtractorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectralSubtractorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectralSubtractorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectralSubtractorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectralSubtractorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectralSubtractorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectralSubtractorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectralSubtractorAudioProcessor::setCurrentProgram (int index)
{
}

const String SpectralSubtractorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SpectralSubtractorAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SpectralSubtractorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSpectralSubtractor.setNumChannels (getTotalNumInputChannels());
    mSpectralSubtractor.updateParameters (FFTSize[mFFTSizeParam->getIndex()],
                                          WindowOverlap[mWindowOverlapParam->getIndex()],
                                          mWindowParam->getIndex(),
                                          nullptr);
    
    {
        const juce::ScopedLock lock (backgroundMutex);
        requiresUpdate = true;
    }
}

void SpectralSubtractorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralSubtractorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SpectralSubtractorAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    mSpectralSubtractor.process (buffer);
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

//==============================================================================
bool SpectralSubtractorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SpectralSubtractorAudioProcessor::createEditor()
{
    return new SpectralSubtractorAudioProcessorEditor (*this);
}

//==============================================================================
// Store the plugin's state in an XML object
void SpectralSubtractorAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    DBG ("GET STATE INFORMATION");
    
//    juce::File xmlFile = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile ("SpectralSubtractor.xml");
    
    if (mNoiseBuffer.getNumChannels() != 0 && mNoiseBuffer.getNumSamples() != 0)
    {
        juce::MemoryBlock memoryBlock;
        juce::WavAudioFormat format;
        {
            // MemoryOutputStream is owned, if the writer was created successfully
            std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (new MemoryOutputStream (memoryBlock, false),
                                                                                     mReader->sampleRate,
                                                                                     mNoiseBuffer.getNumChannels(),
                                                                                     mReader->bitsPerSample,
                                                                                     juce::StringPairArray(),
                                                                                     0));
            writer->writeFromAudioSampleBuffer (mNoiseBuffer, 0, mNoiseBuffer.getNumSamples());
            // End of scope flushes the writer
        }
        apvts.state.getChildWithName (IDs::AudioData).setProperty (IDs::NoiseBuffer,
                                                                   memoryBlock.toBase64Encoding(),
                                                                   nullptr);
    }
    
    juce::ValueTree state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
    
//    xml->writeTo (xmlFile, XmlElement::TextFormat());
}

// Restore the plugin's state from an XML object
void SpectralSubtractorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    DBG ("SET STATE INFORMATION");
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            juce::ValueTree savedState = juce::ValueTree::fromXml (*xmlState);
            
            int numChildren = apvts.state.getNumChildren();
            for (int i = 0; i < numChildren; ++i)   // See NonAutoParameter for why plugin state loading is done like this
            {
                juce::ValueTree stateChild = apvts.state.getChild (i);
                juce::ValueTree savedStateChild = savedState.getChild (i);
                
                jassert (savedStateChild.hasType (stateChild.getType()));
                stateChild.copyPropertiesFrom (savedStateChild, nullptr);
            }
    
            juce::MemoryBlock memoryBlock;
            if (auto noiseBufferAsString = apvts.state.getChildWithName (IDs::AudioData).getProperty (IDs::NoiseBuffer).toString();
                memoryBlock.fromBase64Encoding (noiseBufferAsString))
            {
                juce::WavAudioFormat format;
                mReader.reset (format.createReaderFor (new MemoryInputStream (memoryBlock, false), false));
                mNoiseBuffer.setSize ((int) mReader->numChannels, (int) mReader->lengthInSamples, false, false, false);
                mReader->read (&mNoiseBuffer, 0, (int) mReader->lengthInSamples, 0, true, true);
                
                // Calculate the noise spectrum from the noise buffer that was just loaded
                {
                    const juce::ScopedLock lock (backgroundMutex);
                    requiresUpdate = true;
                }
            }
        }
    }
}

//==============================================================================
// Noise spectrum processing thread functions

void SpectralSubtractorAudioProcessor::run()
{
    while (!threadShouldExit())
    {
        checkForPathToOpen();
        if (threadShouldExit()) return;
        
        checkIfSpectralSubtractorNeedsUpdate();
        if (threadShouldExit()) return;
        
        wait (500);
    }
}

void SpectralSubtractorAudioProcessor::checkForPathToOpen()
{
    juce::String pathToOpen;
     
    {
        const juce::ScopedLock lock (pathMutex);
        pathToOpen.swapWith (chosenPath);
    }

    // If pathToOpen is an empty string then we know there isn't a new file to open.
    if (pathToOpen.isNotEmpty())
    {
        juce::File noiseFile (pathToOpen);
        juce::String errorMessage;
        
        mReader.reset (mFormatManager->createReaderFor (noiseFile));
        if (mReader.get() != nullptr)
        {
            if (mReader->numChannels == 1 || mReader->numChannels == 2)
            {
                auto duration = (float) mReader->lengthInSamples / mReader->sampleRate;
                
                if (true)   // TODO: restrict how long the noise file can be?
                {
                    mNoiseBuffer.setSize ((int) mReader->numChannels,
                                          (int) mReader->lengthInSamples,
                                          false,
                                          false,
                                          false);
                                                              
                    mReader->read (&mNoiseBuffer,
                                   0,
                                   (int) mReader->lengthInSamples,
                                   0,
                                   true,
                                   true);
                    
                    {
                        const juce::ScopedLock lock (backgroundMutex);
                        requiresUpdate = true;
                    }
                }
                else
                    errorMessage = juce::String ("Noise file must be under _____ seconds!");
            }
            else
                errorMessage = juce::String ("Noise file must be either mono or stereo!");
        }
        else
            errorMessage = juce::String ("Unable to read ") + noiseFile.getFileName();
        
        if (errorMessage.isNotEmpty() && getActiveEditor() != nullptr)
        {
            juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                               .withIconType (MessageBoxIconType::InfoIcon)
                                               .withMessage (errorMessage),
                                               nullptr);
        }
    }
}

void SpectralSubtractorAudioProcessor::checkIfSpectralSubtractorNeedsUpdate()
{
    backgroundMutex.enter();
    if (requiresUpdate)
    {
        updateBackgroundThread();
        backgroundMutex.exit();
        
        if (threadShouldExit()) return;
        
        if (mNoiseBuffer.getNumChannels() != 0 && mNoiseBuffer.getNumSamples() != 0)
        {
            // Compute spectrogram of noise signal
            Spectrogram<float> noiseSpectrogram;
            makeSpectrogram (noiseSpectrogram, mNoiseBuffer, *(mBG_FFT.get()), mBG_HopSize, *(mBG_Window.get()));
            
            if (threadShouldExit()) return;

            // Compute noise spectrum
            computeAverageSpectrum (mTempNoiseSpectrum, noiseSpectrogram, mBG_FFT->getSize());
            
            if (threadShouldExit()) return;
            
            // Update the FFT settings and load the new noise spectrum
            mSpectralSubtractor.updateParameters (mBG_FFT->getSize(),
                                                  mBG_overlap,
                                                  mBG_WindowIndex,
                                                  &mTempNoiseSpectrum);
            
            if (threadShouldExit()) return;
            
            if (getActiveEditor() != nullptr)
                juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                                   .withIconType (MessageBoxIconType::InfoIcon)
                                                   .withMessage ("Successfully loaded noise spectrum!"),
                                                   nullptr);
        }
        else
        {
            // Update FFT settings and allocate an empty noise spectrum
            mSpectralSubtractor.updateParameters (mBG_FFT->getSize(),
                                                  mBG_overlap,
                                                  mBG_WindowIndex,
                                                  nullptr);
        }
    }
    else
    {
        backgroundMutex.exit();
    }
}

void SpectralSubtractorAudioProcessor::updateBackgroundThread()
{
    const juce::ScopedLock lock (backgroundMutex);
    
    int fftSize = FFTSize[mFFTSizeParam->getIndex()];
    mBG_overlap = WindowOverlap[mWindowOverlapParam->getIndex()];
    mBG_WindowIndex = mWindowParam->getIndex();
    
    mBG_FFT.reset (new juce::dsp::FFT (static_cast<int> (std::log2 (fftSize))));
    if (mBG_overlap != 0) mBG_HopSize = fftSize / mBG_overlap;
    mBG_Window.reset (new juce::dsp::WindowingFunction<float> (static_cast<size_t> (fftSize + 1), juce::dsp::WindowingFunction<float>::hann, false));
    
    DBG ("Background thread FFT size: " << mBG_FFT->getSize());
    DBG ("Background thread hop size: " << mBG_HopSize);
    DBG ("");
    
    requiresUpdate = false;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralSubtractorAudioProcessor();
}
