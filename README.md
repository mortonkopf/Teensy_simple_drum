# Teensy_simple_drum
This is code for a simple Eurorack drum module using the Teensy. All drum sound is synthesised using the Drum component of the Teensy Audio design tool

the audio is created onboard, and output through the DAC on pin14 of Teensy 3.2



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
