#include "TrafficLevelManager.h"
#include <Arduino.h>

// Constructor
TrafficLevelManager::TrafficLevelManager(const int (&pinNumbers)[3]) {
  for (int i = 0; i < 3; i++) {
    this->pins[i] = pinNumbers[i];  // Copy pin numbers to member array
    this->trafficLevels[i] = false; // Initialize traffic levels to false
  }

  for (int i = 0; i < 7; i++) {
    this->additionalTimes[i] = 0; // Initialize traffic levels to false
  }
  this->additionalTime = 0;                  // Initialize additionalTime
  this->calculateAlgorithmicAproximation(3); // Precompute additional times
}

// Method to calculate additional time
void TrafficLevelManager::calculateAdditionalTime() {
  for (int i = 2; i >= 0; i--) {
    if (this->trafficLevels[i]) {
      this->additionalTime =
          this->additionalTimes[i] * 1000; // Set additionalTime based on levels
      break; // Exit loop after finding the first active level
    }
  }
}

// Reset method
void TrafficLevelManager::init() {
  for (int i = 0; i < 3; i++) {
    pinMode(this->pins[i], INPUT);
  }
}

// Reset method
void TrafficLevelManager::reset() {
  this->additionalTime = 0; // Reset additional time
}

// Method to update traffic levels
void TrafficLevelManager::updateTrafficLevels() {
  for (int i = 0; i < 3; i++) {
    this->trafficLevels[i] =
        digitalRead(this->pins[i]); // Read digital input and update levels
  }
}

// Getter for additional time
int TrafficLevelManager::getAdditionalTime() {
  return this->additionalTime; // Return the calculated additional time
}

// Algorithm approximation
void TrafficLevelManager::calculateAlgorithmicAproximation(int n) {
  for (int i = 0; i <= 2 * n; i++) {
    if (i == 0) {
      this->additionalTimes[i] = 0; // Base case
    } else {
      this->additionalTimes[i] =
          this->additionalTimes[i / 2] + 1; // Recursive relation
    }
  }
}
