#include "LedObserver.h"

LedObserver::LedObserver(GrowBoxState &state) : state(state) {}

void LedObserver::update(StateChange change) {
    if (change == StateChange::soll_LED) {
        if (state.get_SollLedState()) {
            turnOnLedHardware();
        } else {
            turnOffLedHardware();
        }
    }
}

void LedObserver::turnOnLedHardware() {
    // Implementiere die Logik, um die LED über Hardware-Ausgänge einzuschalten
}

void LedObserver::turnOffLedHardware() {
    // Implementiere die Logik, um die LED über Hardware-Ausgänge auszuschalten
}
