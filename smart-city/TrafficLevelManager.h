#ifndef TRAFFIC_LEVEL_MANAGER_H
#define TRAFFIC_LEVEL_MANAGER_H

class TrafficLevelManager {
private:
  int pins[3];            // Array to hold pin numbers
  bool trafficLevels[3];  // Array to store traffic levels
  int additionalTimes[7]; // Array for precomputed times (size based on
                          // algorithm)
  int additionalTime;     // Additional time in milliseconds

public:
  TrafficLevelManager(const int pinNumbers[3]);

  void updateTrafficLevels();
  void calculateAdditionalTime();
  void reset();
  int getAdditionalTime();

private:
  void calculateAlgorithmicAproximation(int n);
};

#endif // TRAFFIC_LEVEL_MANAGER_H
