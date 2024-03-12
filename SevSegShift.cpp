/* SevSegShift Library
 *
 * Copyright 2020 Jens Breidenstein
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * This library enhances the functionality of SevSeg with the possibility 
 * to use it with Shift Registers. 
 * All original functionality of SevSeg is transparent and could be used further
 *
 * See the included readme for instructions.
 * https://github.com/bridystone/SevSegShift
 */
#include "SevSegShift.h"

// constructor implementation
SevSegShift::SevSegShift(byte pinDS, byte pinSHCP, byte pinSTCP,
    const byte numberOfShiftRegisters, // currently const value (not changeable) - maybe in future
    bool
        isDigitsOnController // only Segments are on ShiftRegister - DigitPins are connected to controller
    )
    : SevSeg()
{ // call constructor of base class
    // store data in member variables
    _pinDS = pinDS;
    _pinSHCP = pinSHCP;
    _pinSTCP = pinSTCP;

    // assume that the shift registers have 8 output ports
    //_shiftRegisterMap = new byte[8*numberOfShiftRegisters];
    _numberOfShiftRegisters = numberOfShiftRegisters;
    _isDigitsOnController = isDigitsOnController;
}

void SevSegShift::begin(byte hardwareConfig, byte numDigitsIn, const byte shiftRegisterMapDigits[],
    const byte shiftRegisterMapSegments[], bool resOnSegmentsIn, bool updateWithDelaysIn,
    bool leadingZerosIn, bool disableDecPoint)
{
    // prepare PINs for output
    pinMode(_pinDS, OUTPUT);
    pinMode(_pinSHCP, OUTPUT);
    pinMode(_pinSTCP, OUTPUT);

    // here the digit"pins" of the Shift register is stored
    // pins are here the OUTPUT PINs of the Shift Register
    for (int i = 0; i < 4; ++i) {
        _shiftRegisterMapDigits[i] = shiftRegisterMapDigits[i];
    }
    // here the segment"pins" of the Shift register is stored
    // pins are here the OUTPUT PINs of the Shift Register
    for (int i = 0; i < 8; ++i) {
        _shiftRegisterMapSegments[i] = shiftRegisterMapSegments[i];
    }

    // call begin-function of the base class to prepare everything properly
    SevSeg::begin(hardwareConfig, numDigitsIn, resOnSegmentsIn, updateWithDelaysIn, leadingZerosIn,
        disableDecPoint);
}

// segmentOn
/******************************************************************************/
// Turns a segment on, as well as all corresponding digit pins
// (according to digitCodes[])
// (almost copy from the SevSeg.cpp)
void SevSegShift::segmentOn(byte segmentNum)
{
    _shiftRegisterMap[_shiftRegisterMapSegments[segmentNum]] =
        segmentOnVal; // replace digital Write with prepareing the Shift Register Map
    // digitalWrite(segmentPins[segmentNum], segmentOnVal);
    for (byte digitNum = 0; digitNum < numDigits; digitNum++) {
        if (digitCodes[digitNum] & (1 << segmentNum)) { // Check a single bit
            _shiftRegisterMap[_shiftRegisterMapDigits[digitNum]] =
                digitOnVal; // replace digital Write with prepareing the Shift Register Map
        }
    }

    // NOW perform the actual digitalWrite to the Shift Register(s)
    pushData2ShiftRegister();
}

// segmentOff
/******************************************************************************/
// Turns a segment off, as well as all digit pins
// (almost copy from the SevSeg.cpp)
void SevSegShift::segmentOff(byte segmentNum)
{
    for (byte digitNum = 0; digitNum < numDigits; digitNum++) {
        _shiftRegisterMap[_shiftRegisterMapDigits[digitNum]] =
            digitOffVal; // replace digital Write with prepareing the Shift Register Map
    }
    _shiftRegisterMap[_shiftRegisterMapSegments[segmentNum]] =
        segmentOffVal; // replace digital Write with prepareing the Shift Register Map
    //digitalWrite(segmentPins[segmentNum], segmentOffVal);

    // NOW perform the actual digitalWrite to the Shift Register(s)
    pushData2ShiftRegister();
}

// digitOn
/******************************************************************************/
// Turns a digit on, as well as all corresponding segment pins
// (according to digitCodes[])
// (almost copy from the SevSeg.cpp)
void SevSegShift::digitOn(byte digitNum)
{
    _shiftRegisterMap[_shiftRegisterMapDigits[digitNum]] =
        digitOnVal; // replace digital Write with prepareing the Shift Register Map

    for (byte segmentNum = 0; segmentNum < numSegments; segmentNum++) {
        if (digitCodes[digitNum] & (1 << segmentNum)) { // Check a single bit
            _shiftRegisterMap[_shiftRegisterMapSegments[segmentNum]] =
                segmentOnVal; // replace digital Write with prepareing the Shift Register Map
                              // digitalWrite(segmentPins[segmentNum], segmentOnVal);
        }
    }

    // NOW perform the actual digitalWrite to the Shift Register(s)
    pushData2ShiftRegister();
}

// digitOff
/******************************************************************************/
// Turns a digit off, as well as all segment pins
// (almost copy from the SevSeg.cpp)
void SevSegShift::digitOff(byte digitNum)
{
    _shiftRegisterMap[_shiftRegisterMapDigits[digitNum]] =
        digitOffVal; // replace digital Write with prepareing the Shift Register Map

    for (byte segmentNum = 0; segmentNum < numSegments; segmentNum++) {
        _shiftRegisterMap[_shiftRegisterMapSegments[segmentNum]] =
            segmentOffVal; // replace digital Write with prepareing the Shift Register Map
                           // digitalWrite(segmentPins[segmentNum], segmentOffVal);
    }

    // NOW perform the actual digitalWrite to the Shift Register(s)
    pushData2ShiftRegister();
}

/*
  pushing the data in _shiftRegisterMap to all the Shift Registers
  */
void SevSegShift::pushData2ShiftRegister()
{
    // walk through the ShiftRegisterMap and push eveything to the ShiftRegister(s)
    for (int i = 8 * _numberOfShiftRegisters - 1; i >= 0; i--) {
        // put the data to the data PIN of Arduino
        digitalWrite(_pinDS, _shiftRegisterMap[i]);
        // push it to the next Register (DS -> 0 | 0->1 | 1->2 | ...)
        // this is done by createing a raising flank on SH_CP (ShiftPIN)
        digitalWrite(_pinSHCP, LOW);
        digitalWrite(_pinSHCP, HIGH);
    }
    digitalWrite(_pinSTCP, LOW);
    digitalWrite(_pinSTCP, HIGH);
    // everything in position? - YES
    // now store it in the current state
    // this is done by a raising flank on the ST_CP (Store Pin)
    //digitalWrite(_pinSTCP, LOW);
    //digitalWrite(_pinSTCP, HIGH);
}
