#include <frequencyToNote.h>
#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <pitchToFrequency.h>
#include <pitchToNote.h>

#include "MIDIUSB.h"
// MIDIUSB library to be installed

static void MIDI_setup();
static void MIDI_noteOn(int ch, int note, int velocity);
static void MIDI_noteOff(int ch, int note);

const int MIDI_CHANNEL=1;

const int NCHANNELS = 4;
const int inPins[NCHANNELS] = { A0, A1, A2, A3 };
const int midiNotes[NCHANNELS] = 
{
  // We have taken this data from https://www.midi.org/specifications/item/gm-level-1-sound-set
  36, // Kick
  38, // Snare
  42, // Closed hi-hat
  46, // Open hi-hat
};
const int thresholdLevel[NCHANNELS] = { 30, 30, 30, 30 }; // ADC reading to trigger; lower => more sensitive Detection
const long int maxLevel[NCHANNELS] = { 400, 400, 400, 400 }; // ADC reading for full velocity; lower => more sensitive Detection

static unsigned int vmax[NCHANNELS] = { 0 };
static unsigned int trigLevel[NCHANNELS];
static unsigned int counter[NCHANNELS] = { 0 };

static unsigned int CTR_NOTEON = 10; 
static unsigned int CTR_NOTEOFF = CTR_NOTEON + 30; 


static int statusPin = 2;

void setup() {
  Serial.begin(115200);
  analogReference(DEFAULT);
  pinMode(statusPin, OUTPUT);
  digitalWrite(statusPin, LOW);
  
  for (int i = 0; i < NCHANNELS; i++)
  {
    pinMode(inPins[i], INPUT);
    analogRead(inPins[i]);
    trigLevel[i] = thresholdLevel[i];
  }

  MIDI_setup();
}


void loop() {
  int ch;
  for (ch=0; ch < NCHANNELS; ch++)
  {
    unsigned int v = analogRead(inPins[ch]);
    if ( counter[ch] == 0 )
    {
      if ( v >= trigLevel[ch] )
      {
        vmax[ch] = v;
        counter[ch] = 1;
        digitalWrite(statusPin, HIGH);
      }
    }
    else
    {
      if ( v > vmax[ch] )
        vmax[ch] = v;
      counter[ch]++;
      
      if ( counter[ch] == CTR_NOTEON )
      {
        long int vel = ((long int)vmax[ch]*127)/maxLevel[ch];
        //Serial.println(vel);
        if (vel < 5) vel = 5;
        if (vel > 127) vel = 127;
        MIDI_noteOn(MIDI_CHANNEL, midiNotes[ch], vel);
        trigLevel[ch] = vmax[ch];
      }
      else if ( counter[ch] >= CTR_NOTEOFF )
      {
        MIDI_noteOff(MIDI_CHANNEL, midiNotes[ch]);
        counter[ch] = 0;
        digitalWrite(statusPin, LOW);
      }
    }

    
    trigLevel[ch] = ((trigLevel[ch] * 19) + (thresholdLevel[ch] * 1)) / 20;
  }

}

// MIDI Code
//
// reference : https://www.midi.org/specifications/item/table-1-summary-of-midi-message

void MIDI_setup()
{

}

void MIDI_noteOn(int ch, int note, int velocity)
{
  midiEventPacket_t noteOn = {0x09, 0x90 | (ch-1), note & 0x7F, velocity & 0x7F};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void MIDI_noteOff(int ch, int note)
{
  midiEventPacket_t noteOff = {0x08, 0x80 | (ch-1), note, 1};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}
