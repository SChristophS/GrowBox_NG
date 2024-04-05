#include <iostream>
#include <vector>
#include "Globals.h"

struct LedCycle {
    int durationOn;
    int durationOff;
    int repetitions;
};

class GrowCycle {
public:
    std::vector<LedCycle> cycles;

    void addCycle(int on, int off, int rep) {
        cycles.push_back({on, off, rep});
    }


    void executeCycle(int startFromHere) {
        std::cout << "Starting at second " << startFromHere << std::endl;
        int elapsedTime = startFromHere;
        
        for (size_t cycleIndex = 0; cycleIndex < cycles.size(); ++cycleIndex) {
            const auto& cycle = cycles[cycleIndex];
            int totalCycleDuration = (cycle.durationOn + cycle.durationOff) * cycle.repetitions;
            
            if (elapsedTime >= totalCycleDuration) {
                elapsedTime -= totalCycleDuration;
                continue;
            }
            
            int timeInCurrentCycle = elapsedTime % (cycle.durationOn + cycle.durationOff);
            int currentRepetition = elapsedTime / (cycle.durationOn + cycle.durationOff);
            
            for (int rep = currentRepetition; rep < cycle.repetitions; ++rep) {
                if (timeInCurrentCycle < cycle.durationOn) {
                    int onTime = cycle.durationOn - timeInCurrentCycle;
                    LedStatus = true; // LED einschalten
                    std::cout << "[Cycle " << cycleIndex + 1 << ", Repetition " << rep + 1 << "] LED on for " << onTime << " seconds." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(onTime));
                    timeInCurrentCycle = 0;
                } else {
                    timeInCurrentCycle -= cycle.durationOn;
                }

                if (rep == currentRepetition && timeInCurrentCycle > 0) {
                    int offTime = cycle.durationOff - timeInCurrentCycle;
                    LedStatus = false; // LED ausschalten
                    std::cout << "[Cycle " << cycleIndex + 1 << ", Repetition " << rep + 1 << "] LED off for " << offTime << " seconds." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(offTime));
                } else {
                    LedStatus = false;
                    std::cout << "[Cycle " << cycleIndex + 1 << ", Repetition " << rep + 1 << "] LED off for " << cycle.durationOff << " seconds." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(cycle.durationOff));
                }
            }
            elapsedTime = 0;
        }
    }



};


