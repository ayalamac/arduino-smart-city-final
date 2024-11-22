// class TrafficLevelManager {
//     private:
//         int additionalTime = 0;
//         bool* trafficLevels[2][3] = {
//             {&vS1V1, &vS1V2, &vS1V3},
//             {&vS2V1, &vS2V2, &vS2V3}
//         };
//
//     public:
//         void calculateAdditionalTime(int activeSemaphore) {
//             for (int i = 2; i >= 0; i--) {
//                 if (*this->trafficLevels[activeSemaphore][i] == 1) {
//                     this->additionalTime = this->calculateAlgorithmicAproximation(i+1) * 1000;
//                 }
//             }
//         }
//
//         void reset() {
//             this->additionalTime = 0;
//         }
//
//         int getAdditionalTime() {
//             return this->additionalTime;
//         }
//
//         int calculateAlgorithmicAproximation(int n) {
//             int log2_array[2*n + 1];  // Array to hold log2 values from 1 to n
//
//             // Base case
//             log2_array[1] = 0;  // log2(1) = 0
//
//             // Fill the DP table iteratively
//             for (int i = 2; i <= 2*n; i++) {
//               log2_array[i] = log2_array[i / 2] + 1;  // log2(i) = log2(i//2) + 1
//             }
//
//             return log2_array[2*n];  // Return the log2 of n
//         }
// };
//
