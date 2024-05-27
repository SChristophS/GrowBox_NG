#ifndef LED_OBSERVER_H
#define LED_OBSERVER_H

#include "IObserver.h"
#include "GrowBoxState.h"

class LedObserver : public IObserver {
private:
    GrowBoxState &state;

public:
    LedObserver(GrowBoxState &state);
    void update(StateChange change) override;

    void turnOnLedHardware();
    void turnOffLedHardware();
};

#endif // LED_OBSERVER_H
