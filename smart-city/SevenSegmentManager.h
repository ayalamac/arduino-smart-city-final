#ifndef SEVEN_SEGMENT_MANAGER_H
#define SEVEN_SEGMENT_MANAGER_H

#include <Arduino.h>

class SevenSegmentManager {
public:
  // Constructor to initialize with an array of 7 pin numbers for segments
  SevenSegmentManager(const int (&pinNumbers)[7]);

  // Method to print a digit on the 7-segment display
  void print(int digit);
  void init();

private:
  // Property for segment control (0-9 for digits, each with 7 segment states)
  const int segments[11][7] = {
      {LOW, LOW, LOW, LOW, LOW, LOW, HIGH},     // 0
      {HIGH, LOW, LOW, HIGH, HIGH, HIGH, HIGH}, // 1
      {LOW, LOW, HIGH, LOW, LOW, HIGH, LOW},    // 2
      {LOW, LOW, LOW, LOW, HIGH, HIGH, LOW},    // 3
      {HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW},   // 4
      {LOW, HIGH, LOW, LOW, HIGH, LOW, LOW},    // 5
      {LOW, HIGH, LOW, LOW, LOW, LOW, LOW},     // 6
      {LOW, LOW, LOW, HIGH, HIGH, HIGH, HIGH},  // 7
      {LOW, LOW, LOW, LOW, LOW, LOW, LOW},      // 8
      {LOW, LOW, LOW, LOW, HIGH, LOW, LOW},     // 9

      {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW} // -1
  };

  int segmentPins[7]; // Array to hold pin numbers for each segment (A to G)
};

#endif // SEVEN_SEGMENT_MANAGER_H
