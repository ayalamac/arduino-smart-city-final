#include "SevenSegmentManager.h"
#include <Arduino.h>

// Constructor to initialize with an array of 7 pin numbers for segments
SevenSegmentManager::SevenSegmentManager(const int (&pinNumbers)[7]) {
  // Initialize the segment pins array with the passed pin numbers
  for (int i = 0; i < 7; i++) {
    this->segmentPins[i] = pinNumbers[i];
  }
}

// Method to print a digit on the 7-segment display
void SevenSegmentManager::print(int digit) {
  // Ensure the digit is between 0 and 9
  if (digit < 0 || digit > 9) {
    digit = 10; // Set to -1 if out of range
  }

  // Loop through each segment pin and set it to the appropriate state (HIGH or
  // LOW)
  for (int i = 0; i < 7; i++) {
    digitalWrite(this->segmentPins[i], this->segments[digit][i]);
  }
}

void SevenSegmentManager::init() {
  for (int i = 0; i < 7; i++) {
    pinMode(this->segmentPins[i], OUTPUT);    // Set the pins as output
    digitalWrite(this->segmentPins[i], HIGH); // Turn off all segments
  }
}
