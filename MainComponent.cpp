#include "MainComponent.h"
#include "FreqSpectrum.h"
#include "StringLamps.h"
#include <limits>
#include <list>


//==============================================================================
MainComponent::MainComponent() : forwardFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    setAudioChannels (2, 0); // Set up audio channels for 2 inputs and 0 outputs
    startTimerHz (30); // Timer callback would be called every 30 ms
    
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    addAndMakeVisible(chooseTuning);
    chooseTuning.setText("Select tuning", juce::dontSendNotification);
    chooseTuning.setFont(textFont);
    tuningSelector.addItem(" --------------- ", 1);
    tuningSelector.addItem("Standard Tuning (E A D G B E)", 2);
    tuningSelector.addItem("Drop D Tuning (D A D G B E)", 3);
    tuningSelector.addItem("Open D Tuning (D A D F# A D)", 4);
    tuningSelector.setSelectedId(1);
    
    
    tuningSelector.onChange = [this]()
    {
        selectedTuning = tuningSelector.getSelectedId();

        switch (selectedTuning)
        {
            case 2: selectedTuningArray = standardTuning; break;
            case 3: selectedTuningArray = dropDTuning; break;
            case 4: selectedTuningArray = openDTuning; break;
            default: selectedTuningArray = nullptr; break;
        }
    };
    selectedTuningArray = nullptr;
  
    addAndMakeVisible(tuningSelector);
    addAndMakeVisible(fspec);
    
    // First turn all lamps OFF
    stringlamp1.setLampState(false);
    stringlamp2.setLampState(false);
    stringlamp3.setLampState(false);
    stringlamp4.setLampState(false);
    stringlamp5.setLampState(false);
    stringlamp6.setLampState(false);
    addAndMakeVisible(stringlamp1);
    addAndMakeVisible(stringlamp2);
    addAndMakeVisible(stringlamp3);
    addAndMakeVisible(stringlamp4);
    addAndMakeVisible(stringlamp5);
    addAndMakeVisible(stringlamp6);
    
  
}

MainComponent::~MainComponent()
{
    
    shutdownAudio(); // Shut down audio
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    currentSampleRate = sampleRate; // store sample rate for calculating tau_max, tau_min
    tau_max = currentSampleRate/f0_min; // 537
    tau_min = currentSampleRate/f0_max; //133
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
//    bufferSize = bufferToFill.numbers; 
//    std::cout << bufferSize << std::endl;
        if (bufferToFill.buffer->getNumChannels()>0)
    {
        auto *channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        for (auto i = 0; i < bufferToFill.numSamples; ++i)
            pushNextSampleIntoFifo(channelData[i]);
            //differenceFunction(channelData[i]);
    }
}

void MainComponent::pushNextSampleIntoFifo (float sample) noexcept
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));
            // When fifoIndex and fftData reaches the end of block, make sure the previous fft data is replaced with 0s
            memcpy(fftData, fifo, sizeof(fifo));
            // Copy fifo data to fftData
            nextFFTBlockReady = true;
            // Flag set to true to start filling up the next block
        }
        fifoIndex = 0; // fifoIndex reset back to 0
    }
    fifo[fifoIndex++] = sample;
    
    if (yinIndex == fftSize)
    {
        if (!YinBuffReady)
        {
            juce::zeromem(yinData, sizeof(yinData));
            memcpy(yinData, yinBuffer, sizeof(yinBuffer));
            YinBuffReady = true;
        }
        yinIndex = 0;
    }
    yinBuffer[yinIndex++] = sample;
    
    
    
}
// -------------------------------- Y I N Functions -----------------------------------------
void MainComponent::differenceFunction()
{
    
    auto audio_data = yinBuffer; // pointer to the address of yinBuffer
    int n = sizeof(yinBuffer)/sizeof(yinBuffer[0]); // 2048 (Integration window size of 2048 samples)
    auto lagRangeMax = std::min(tau_max, n); // Picks tau_max (537 samples of lag)
    

    std::vector<float> audio_cumsum;
    float sum = 0.0f;
    for (auto i = 0; i < n ; ++i) // Difference equation cycle between first index and 2048
    {
        sum += audio_data[i] * audio_data[i]; // Calculate sum for each index
        audio_cumsum.push_back(sum); // Sum them up
    }
    audio_cumsum.insert(audio_cumsum.begin(), 0.0f); // Once it reaches 2048 samples, make sure the array starts with 0
    
    
    int size = lagRangeMax + n; // 2585 samples

    
    
    int p2 = 0;
    int value = size/32; // 80 bits used to represent the size at the power of 2
    std::cout << "Size: " << size << std::endl;
    while (value > 0)
    {
        value >>= 1;  // shift right by 1 bit (divide value/2^1) for each cycle; Goes from 80, 40, 20, 10, 5, 2, 1, 0)
        ++p2; // p2 = 1, 2,3, 4, 5, 6, 7
    }
    
    // p2 is 7
    
    std::vector<int> nice_numbers = {16, 18, 20, 24, 25, 27, 30, 32};
    int size_pad = std::numeric_limits<int>::max(); // Set to max?? 2147483647

    // When SR is 44100
    for (int k : nice_numbers)
    {
        int candidate = k * (1 << p2); // Starts at 2048, 2304, 2560, 3072 (44100)
        
        if (candidate >= size && candidate < size_pad) //  2048 is not greater than or equal to 2585 AND 2048 < size pad (max)
            size_pad = candidate; // Sets size pad to 3072; The loop quits once k reaches 32
        
    }
    
    
    
    
    // size_pad after the loop is set to 3072 (SR: 44100)
    
    juce::dsp::FFT fft(log2(size_pad));  // An FFT object is created with the size of 12 (skips 11.58, because it accepts only integers)
   
 

    std::vector<float> fftBuffer(size_pad * 2, 0.0f); //fftBuffer of 6144, zeropadded

    std::copy(audio_data, audio_data + n, fftBuffer.begin());
    //copy all the elements between audio_data and the last value of audio data into fftBuffer

    // Do FFT in-place:
    fft.performRealOnlyForwardTransform(fftBuffer.data());
    
    
    
    // 1) Compute power spectrum:
    for (int k = 0; k < size_pad / 2 + 1; ++k) //For every index in size_pad/2 + 1
    {
        float re = fftBuffer[2 * k]; // Real value at every bin
        float im = fftBuffer[2 * k + 1]; // Imaginary value at every bin
        fftBuffer[2 * k] = re * re + im * im;  // magnitude squared
        fftBuffer[2 * k + 1] = 0.0f;           // imag part = 0 for power (not needed for YIN, set to 0 before IFFT)
    }
    
    // fftBuffer now contains interleaved data of re and im parts. eg: [1, 0, 2, 0....]

   
    fft.performRealOnlyInverseTransform(fftBuffer.data()); // perform IFFT on the fft power spectrum

    
    
    std::vector<float> conv(fftBuffer.begin(), fftBuffer.begin() + lagRangeMax);
    // Initialize conv with the values of fftBuffer from beginning to 6144+537 samples)
    
    std::vector<float> dfResult(lagRangeMax); //dfResult = 537 samples

    for (int tau = 0; tau < lagRangeMax; ++tau) // Start with 0 lag with tau being less than max lag (547 samples)
    {
        // Refer to eq 7 from the paper
        float term1 = audio_cumsum[n] - audio_cumsum[tau]; // energy term1 (r_t)
        float term2 = audio_cumsum[n] - audio_cumsum[n - tau]; // energy term2 (t_t+tau)
        dfResult[tau] = term1 + term2 - 2.0f * conv[tau]; // Equation 7
    }
    
    size_t df_size = dfResult.size(); // 537 different values for each lag
    
    cmndf = cumulativeMeanNormalizedDifferenceEquation(dfResult, df_size);

}


std::vector<float> MainComponent::cumulativeMeanNormalizedDifferenceEquation(const std::vector<float>& differenceFunction, size_t dfSize)
{
    std::vector<float> cmndf(dfSize);
    cmndf[0] = 1.0f; // Set the first element in df equation to 1, refer to Step: 3 in CMNDF

    float cumulativeSum = 0.0f;
    for (size_t tau = 1; tau < dfSize; ++tau) // cycle through every lag
    {
        cumulativeSum += differenceFunction[tau]; // Add that value to the cumulative sum
        cmndf[tau] = differenceFunction[tau] * tau / cumulativeSum; // ALWAYS multiply formulas first because of floating point precision
    
    }
    //std::cout << "CMNDF: " << cmndf[133] << std::endl;
    return cmndf; // 537 total values in cmndf
}

void MainComponent::getPitch()
{
    // Reset all lamps
    stringlamp1.setLampState(false);
    stringlamp2.setLampState(false);
    stringlamp3.setLampState(false);
    stringlamp4.setLampState(false);
    stringlamp5.setLampState(false);
    stringlamp6.setLampState(false);
    if (cmndf.empty()) return;
    if (selectedTuningArray == nullptr) return;
    
    float threshold = 0.1f;
    size_t tau = tau_min;
    
    while (tau < cmndf.size())
    {
        if (cmndf[tau] < threshold)
        {
            while (tau + 1 < cmndf.size() && cmndf[tau + 1] < cmndf[tau]) // if 134 < 537 and cmndf[134] < cmndf[133]
                ++tau;
            
            float f0 = currentSampleRate / static_cast<float>(tau);
            
            
            if (f0 < f0_min || f0 > f0_max)
                currentF0 = -1.0f;
            else
                currentF0 = f0;
            
            break;
        }
        ++tau;
    }
    

    // Now light up the one closest to currentF0
    for (int i = 0; i < 6; ++i)
    {
        float diff_f0 = std::abs(selectedTuningArray[i] - currentF0);
        if (diff_f0 <= 2.0f)
        {
            switch (i)
            {
                case 0: stringlamp1.setLampState(true); break;
                case 1: stringlamp2.setLampState(true); break;
                case 2: stringlamp3.setLampState(true); break;
                case 3: stringlamp4.setLampState(true); break;
                case 4: stringlamp5.setLampState(true); break;
                case 5: stringlamp6.setLampState(true); break;
            }
        }
    }


    

    DBG("Detected F0: " << currentF0); // Optional debug log
}



// -------------------------------------------------------------------------------------------
void MainComponent::drawNextFrameOfSpectrum() // Maps 2048 fft bins to 512 points of scopedata
{
    // first apply a windowing function to our data
    window.multiplyWithWindowingTable (fftData, fftSize); // [1]
    // then render our FFT data..
    forwardFFT.performFrequencyOnlyForwardTransform (fftData); // [2]
    auto mindB = -30.0f;
    auto maxdB = 0.0f;
    for (int i = 0; i < scopeSize; ++i) // [3]
    {
        auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) scopeSize) * 0.2f); // To skew the x=axos to logarithmic scale
        auto fftDataIndex = juce::jlimit (0, fftSize / 2, (int) (skewedProportionX * (float) fftSize * 0.5f));
        auto level = juce::jmap (juce::jlimit (mindB, maxdB, juce::Decibels::gainToDecibels (fftData[fftDataIndex]) - juce::Decibels::gainToDecibels ((float) fftSize)),
            mindB,
            maxdB,
            0.0f,
            1.0f);
        scopeData[i] = level; // Passed onto FreqSpectrum
    }
    
}

void MainComponent::timerCallback() 
{
    if (nextFFTBlockReady)
     {
         drawNextFrameOfSpectrum();
         nextFFTBlockReady = false;

         fspec.updateSpectrumData(scopeData, scopeSize);
         fspec.setFundamentalFrequency(currentF0);
         fspec.repaint();
     }

     if (YinBuffReady)
     {
         // --- Compute RMS ---
         float rms = 0.0f;
         for (int i = 0; i < fftSize; ++i)
             rms += yinBuffer[i] * yinBuffer[i];

         rms = std::sqrt(rms / (float)fftSize);

         const float volumeThreshold = 0.005f; // Try tuning between 0.005f - 0.02f depending on mic sensitivity

         if (rms > volumeThreshold)
         {
             differenceFunction();
             getPitch();
         }
         else
         {
             currentF0 = -1.0f; // Invalidate pitch when signal is too weak
         }

         YinBuffReady = false;
     }
    
}

void MainComponent::drawFrame (juce::Graphics& g)
{
    for (int i = 1; i < scopeSize; ++i)
    {
        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();
        g.drawLine ({ (float) juce::jmap (i - 1, 0, scopeSize - 1, 0, width),
            juce::jmap (scopeData[i - 1], 0.0f, 1.0f, (float) height, 0.0f),
            (float) juce::jmap (i, 0, scopeSize - 1, 0, width),
            juce::jmap (scopeData[i], 0.0f, 1.0f, (float) height, 0.0f) });
    }
}

void FreqSpectrum::setFundamentalFrequency(float freq)
{
    fundamentalFreq = freq;
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
//    auto area = getLocalBounds().reduced(getWidth() * 0.05f);
//
//    auto topbar = area.removeFromTop(proportionOfHeight(0.01f));
//    chooseTuning.setBounds(topbar.withHeight(30).withSizeKeepingCentre(getWidth()*0.9f, 30));
//
//    // Take next 10% for dropdown
//    auto dropdownArea = area.removeFromTop(proportionOfHeight(0.1f));
//    tuningSelector.setBounds(dropdownArea.withHeight(30).withSizeKeepingCentre(getWidth() * 0.5f, 30));
//
//    // Centered box for spectrum
//    auto spectrumArea = area.removeFromTop(proportionOfHeight(0.6f));
//    fspec.setBounds(spectrumArea.withSizeKeepingCentre(getWidth() * 0.6f, getHeight() * 0.5f));
    
    chooseTuning.setBounds(
        getWidth() * 0.45f,   // 45% from the left
        getHeight() * 0.05f,  // 5% from the top
        getWidth() * 0.5f,    // 50% width
        30                    // fixed height
    );
    
    tuningSelector.setBounds(
        getWidth() * 0.1875f,   // 18.75% from the left
        getHeight() * 0.13f,    // 13% from the top
        getWidth() * 0.625f,    // 62.5% width
        30                      // fixed height
    );
    
    fspec.setBounds(
        getWidth() * 0.1870f,   // horizontally aligned with dropdown
        getHeight() * 0.25f,    // starts lower to give space to label & dropdown
        getWidth() * 0.625f,    // same width as dropdown
        getHeight() * 0.4f      // takes 60% of height (adjust as needed)
    );
    
    stringlamp1.setBounds(
        getWidth() * 0.2370f,   // horizontally aligned with dropdown
        getHeight() * 0.70f,    // starts lower to give space to label & dropdown
        getWidth() * 0.05f,    // same width as dropdown
        getHeight() * 0.05f      // takes 60% of height (adjust as needed)
    );
    stringlamp2.setBounds(
        getWidth() * 0.3370f,   // horizontally aligned with dropdown
        getHeight() * 0.70f,    // starts lower to give space to label & dropdown
        getWidth() * 0.05f,    // same width as dropdown
        getHeight() * 0.05f
    );
     stringlamp3.setBounds(
        getWidth() * 0.4370f,   // horizontally aligned with dropdown
        getHeight() * 0.70f,    // starts lower to give space to label & dropdown
        getWidth() * 0.05f,    // same width as dropdown
        getHeight() * 0.05f
    );
    stringlamp4.setBounds(
       getWidth() * 0.5370f,   // horizontally aligned with dropdown
       getHeight() * 0.70f,    // starts lower to give space to label & dropdown
       getWidth() * 0.05f,    // same width as dropdown
       getHeight() * 0.05f
   );
    stringlamp5.setBounds(
       getWidth() * 0.6370f,   // horizontally aligned with dropdown
       getHeight() * 0.70f,    // starts lower to give space to label & dropdown
       getWidth() * 0.05f,    // same width as dropdown
       getHeight() * 0.05f
   );
    stringlamp6.setBounds(
       getWidth() * 0.7370f,   // horizontally aligned with dropdown
       getHeight() * 0.70f,    // starts lower to give space to label & dropdown
       getWidth() * 0.05f,    // same width as dropdown
       getHeight() * 0.05f
   );
     
                           
                           
    
    
                           
//    chooseTuning.setBounds(350, 10, 600, 30);
//    tuningSelector.setBounds(150, 50, 500, 30);
//    fspec.setBounds(150, 100, 500, 500);
}

