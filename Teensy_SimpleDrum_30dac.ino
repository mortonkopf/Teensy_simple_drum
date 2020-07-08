/*
 * this is the simplest drum synthesis module I could make for hte eurorack. 
 * There a virtually external parts to the Teensy 3.* other than some 
 * protection diodes and a Resistor-Capacitor filter for the audio
 * output from the DAC pin. The sketch uses the onboard DAC for output.
 * 
 * THis is Lo-Fi. dont expect complicated sounds from the module, that
 * happens further along the sound chain.
 * 
 * The module has three potentiometers
 * - a patch selection
 * - beat swing (for shifting the incoming second beat for groove feel)
 * - crush (for chopping off the end of the sound to give more staccatto feel)
 * 
 * two buttons - one for sound bank selection, and one for randomness selection - 
 * two random settings, every second beat, or all beats.
 * 
 * one 3.5mm jack input for clock in, one 3.5mm output for audio out.
 * 
 * The sketch has 16 drum sounds make from just the drum. element of the
 * Teensy audio system development tool:
 * https://www.pjrc.com/teensy/gui/
 * 
 * the idea is to show that some core sounds can be made in synthesis, easily.
 */
#include <synth_simple_drum.h>
#include <Bounce2.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthSimpleDrum     drum1;
AudioMixer4              mixer1;
AudioEffectEnvelope      envelope1;
AudioOutputAnalog        dac1;
AudioConnection          patchCord1(drum1, envelope1);
AudioConnection          patchCord2(envelope1, 0, mixer1, 1);
AudioConnection          patchCord3(mixer1, dac1);


const float drumVals[16][4] = { 
//these values fill the drum. calls =frq/lngth/secMix/pitchMod
  {60, 600, 0.0, 1.0},//0 drum sound1 parameters
  {80, 200, 0.1, 0}, //1 drum sound2 parameters, etc
  {80, 138, 0.5, 0.5},//2
  {36, 138, 0.0, 0.4},//3
  {55, 180, 1.0, 0.8},//4
  {600, 10, 0.0, 0},//5
  {58, 250, 1.0, 1.0},//6
  {58, 150, 0.5, 1.0},//6

  {60, 200, 0.0, 1.0},//8
  {49, 125, 0.0, 1.0},//9
  {49, 800, 1.0, 1.0},//10
  {49, 100, 1.0, 0.4},//11
  {49, 125, 0.0, 1.0},//12
  {59, 80, 1.0, 1.0},//13
  {39, 125, 1.0, 0},//14
  {60, 250, 1.0, 1.0},//15
};

//-----------------for push buttons--------//
int button1 = 0; //bank selection
int button2 = 1; //add variable / random(short press long press)

Bounce debouncer1 = Bounce(); // Instantiate a Bounce object
Bounce debouncer2 = Bounce();
Bounce debouncer3 = Bounce();
unsigned long button1Time = 0;
unsigned long button2Time = 0;
unsigned long button1TimeOff = 0;
unsigned long button2TimeOff = 0;

int bankNum = 0;
boolean KickBank1 = true;
int addRand = 0; //holder for level of randomness for sample change

//---------------analogue inputs
const int swingRot = A1;//D15rotary for swing
const int kickSample = A2;//D16rotary to select Kick Sample to be played
const int clockIn = A3; //D17is there a clock pulse
const int crushRot = A9;//D23 rotary for ASDR cruch amount

//----for receiving clock trigger
unsigned long currentClTime = 0;
unsigned long oldClTime = 0;

//----------change clockGap from 500 after debug, to 50!!
int clockGap = 50; //gap for counting only one trigger per pin high
int clockTime = 0; //holder for gap between received clock inputs
int clockCounter = 0; //use clock trigger to count clocks for swing?

unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long swing = 0;

boolean clockBool = false;
int clockStep = 0;
int maxStep = 16; //we will start at this number

//----drum crush envelope
float attack1 = 0.3; //float
int hold1 = 0;
int decay1 = 35;
int sustain1 = 0;
int release1 = 0;

//------crush amount--------//
int crushRead = 0; //instatiate at zero

//-------incation leds
unsigned long newLedTime = millis();
unsigned long ledTime = 0;
unsigned long oldLedTime = 0;

//--------swing function------------------//
int swRead;
int sw;
unsigned long beatGapOld = 0;
unsigned long beatMarker = 0;
unsigned long beatGap = 0;
unsigned int movement = 0; //does this need to be unsigned?


//----------------------start of setup----------------------//
void setup() {

  AudioMemory(12);

  mixer1.gain(0, 2.0); //try lowering these ??
  mixer1.gain(1, 2.0);

  envelope1.attack(attack1);
  envelope1.hold(hold1);
  envelope1.decay(decay1);
  envelope1.sustain(sustain1);

  pinMode(button1, INPUT_PULLUP);//bank selection button
  debouncer1.attach(button1);
  debouncer1.interval(25); // interval in ms
  pinMode(button2, INPUT_PULLUP); //random kick patch level
  debouncer2.attach(button2);
  debouncer2.interval(25); // interval in ms

  pinMode(swingRot, INPUT);//rotary for sample length
  pinMode(clockIn, INPUT_PULLDOWN); //input jack for clock trigger
  pinMode(kickSample, INPUT);//rotary for sample selection
  pinMode(crushRot, INPUT); //rotary forclock division selection

  pinMode(20, OUTPUT);//led for random 1
  pinMode(21, OUTPUT); //led for random 2
  pinMode(22, OUTPUT); //led for trigger in

//---start the clock input timer
  oldClTime = millis();

}//---------------------------end of setup----------------------//


void loop() {
  newLedTime = millis();

  if (newLedTime > (ledTime + 50)) {
    digitalWrite(22, LOW);
  }

  // Update the Bounce instances :
  debouncer1.update();
  debouncer2.update();

  //---check if button has been pressed--//
  if ( debouncer1.fell() ) { //button one is switch bank selection
    //this is a state change button
    if ( bankNum == 0 ) {
      bankNum = 8; //add eight to move to next bank of sounds (no arrays used this time)
    } else {
      bankNum = 0;
    }
  }
  if ( debouncer2.fell() ) { //button two is add random sample
    //this is a short/med/long press button
    button2Time = millis();
  }

  if (debouncer2.rose()) {
    button2TimeOff = millis();
    if (button2TimeOff - button2Time < 450)
    {
      addRand = 1;
      digitalWrite(20, HIGH);
    }
    if ((button2TimeOff - button2Time > 450) && (button2TimeOff - button2Time < 1000))
    {
      addRand = 2;
      digitalWrite(20, HIGH);
      digitalWrite(21, HIGH);
      Serial.println("two");
    }
    if (button2TimeOff - button2Time >= 1000)
    {
      addRand = 0;
      digitalWrite(20, LOW);
      digitalWrite(21, LOW);
    }
  }

  if (analogRead(clockIn) > 250) { ////??or 450? Debug
     Serial.println(analogRead(clockIn) ); //Debug
    addClock();
  }

  crush();

}//---------------------------end of loop()---------------------//

void crush() {
  crushRead = analogRead(crushRot) + 1; // value is 0-1023
  hold1 = map(crushRead, 0, 1023, 0, 250); // map to range of 1-250
  envelope1.decay(hold1);
}

//--------swing function------------------//
void swingDrum() {
  beatGap = (oldClTime - beatMarker) / 8; //divide gap between two clocks by 8
  swRead = analogRead(swingRot) + 1; // value is 0-1023
  sw = map(swRead, 0, 1023, 0, 8); // none, plus 7 delays
  movement = (sw * beatGap); //*1000 as using micros
  if (movement >= (beatGap * 8)) {
    movement = (beatGap * 7);
  }
  swing = movement;
}

unsigned long currentSWTime = 0;
unsigned long oldSWTime = 0;

void addClock() {
  //if a cv/clock input is registered, is it the same one?,
  currentClTime = millis();
  swingDrum();


  if ((currentClTime > (oldClTime + (clockGap))) &&
      ((!(clockStep % 2) == 0)))
  {
    digitalWrite(22, HIGH);
    ledTime = millis();
    clockBool = true;
    checkRotSample();
    oldClTime = currentClTime;
    clockStep++;
  }
  else {
    clockBool = false;
  }

  if ((currentClTime > (oldClTime + (clockGap))) &&
      ((clockStep % 2) == 0))
  {

    oldClTime = currentClTime;
    clockBool = true;
    delay(movement);//<<<<<<<ooops, temp, should use without delay...
    digitalWrite(22, HIGH);
    ledTime = millis();
    checkRotSample();
    beatMarker = currentClTime;
    clockStep++;
  }
  else {
    clockBool = false;
  }

  if (clockStep >= maxStep) {
    clockStep = 0;
  }

}//end of if clock is true


//--------------------------------------------------
int banks = 0;
int possRead;
int poss = 0;

void checkRotSample() {
  // A number is "even" if the least significant bit is zero.
  //if ( (i & 0x01) == 0) { do_something(); }

  banks = bankNum;
  possRead = analogRead(kickSample) + 1; // value is 0-1023
  poss = map(possRead, 0, 1023, 0, 7); //gives eight sounds locations on the pot

  if (addRand == 0) {
    playSample(banks, poss);
  }

  if ((addRand == 1) && ((clockStep & 0x01) == 0)) {
    playSample(banks, random(0, 7));
  }

  if ((addRand == 1) && (!(clockStep % 2) == 0)) {
    playSample(banks, poss);
  }

  if (addRand == 2) {
    playSample(banks, random(0, 7));
  }
}


void playSample(int bank, int pos) {
//pos is 0-7, giving eight locations on the pot, bank is 0 or 8, giving locations
//8 to 15 in the sound array if button is true
  drum1.frequency(drumVals[pos + bank][0]);
  drum1.length(   drumVals[pos + bank][1]);
  drum1.secondMix(drumVals[pos + bank][2]);
  drum1.pitchMod( drumVals[pos + bank][3]);

  envelope1.noteOn();
  drum1.noteOn();

}
