#include "MainComponent.h"

MainComponent::MainComponent() : state(Stopped), slowBuffer(2, 5644800) //todo make more efficient by resizing later as needed
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (400, 400);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    addAndMakeVisible(&openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };
    
    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
//    playButton.setEnabled(false);
    
    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);
    
    // Configure formatManager to read wav and aiff files
    formatManager.registerBasicFormats();
    
    // listen for when the state of transport changes and call the changeListener callback function
    transport.addChangeListener(this);
    
    //TEMP
    slowBuffer.clear();
    //ENDTEMP
    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!
    
    //TEMP
    // read samples into bufferToFill.buffer
    // need a way to keep track of index to read from
    //ENDTEMP
    
    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
    
    transport.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // Completely fill the background with a solid color
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    
    openButton.setBounds(125, 90, 150, 30);
    playButton.setBounds(125, 130, 150, 30);
    stopButton.setBounds(125, 170, 150, 30);
}

//==============================================================================

void MainComponent::openButtonClicked()
{
    // Choose a file
    juce::FileChooser chooser("Choose a Wav file...", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.wav;*.aiff", true, false, nullptr);
    
    // If the user chooses a file
    if (chooser.browseForFileToOpen())
    {
        // Get the chosen file
        juce::File loadedFile = chooser.getResult();
        // Read the file
        reader = formatManager.createReaderFor(loadedFile);
        
        //TEMP
        //todo determine a good location
        f.open("/Users/andrewking/Desktop/tempslow.txt", std::ios::app);
        //            array of channel out pointers,       number of out channels, start sample, number of samples to read
        reader->read(slowBuffer.getArrayOfWritePointers(), slowBuffer.getNumChannels(), 0, slowBuffer.getNumSamples());
        
        for (int i = 0; i < slowBuffer.getNumSamples(); i++) {
            f << slowBuffer.getSample(0, i) << "   " << slowBuffer.getSample(1, i) << std::endl;
        }
        f.close();
        //TEMP
        
        if (reader != nullptr) {
            // Get the file ready to play
            std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(reader, true));
            // set transport source to the data that tempSource is pointing to
            transport.setSource(tempSource.get());
            // change state to Stopped
            transportStateChanged(Stopped);
            // When the file is loaded, pass the data to self.playSource
            playSource.reset(tempSource.release());
        }

    }
}

void MainComponent::playButtonClicked()
{
    transportStateChanged(Starting);
}

void MainComponent::stopButtonClicked()
{
    transportStateChanged(Stopping);
}

void MainComponent::transportStateChanged(TransportState newState)
{
    if (newState != state)
    {
        state = newState;
        
        switch (state) {
            case Stopped:
                playButton.setEnabled(true);
                transport.setPosition(0.0); // Set playhead at beginning of audio
                break;
            case Starting:
                stopButton.setEnabled(true);
                playButton.setEnabled(false);
                transport.start();
                break;
            case Playing:
                playButton.setEnabled(false);
                stopButton.setEnabled(true);
                break;
            case Stopping:
                playButton.setEnabled(true);
                stopButton.setEnabled(false);
                transport.stop();
                transport.setPosition(0.0); // Set playhead at beginning of audio
                break;
        }
    }
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transport)
    {
        if (transport.isPlaying()) {
            transportStateChanged(Playing);
        } else {
            transportStateChanged(Stopped);
        }
    }
}
