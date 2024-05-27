#ifndef IOBSERVER_H
#define IOBSERVER_H

enum class StateChange {
    soll_LED,
    ist_LED,

    soll_waterReservoir,
    ist_waterReservoir,

    soll_airTemperature,
    ist_airTemperature,

    soll_Humidity,
    ist_Humidity,

    ist_InflowWaterPump,
    ist_OutflowWaterPump,
    Temperature
};

class IObserver {
public:
    virtual ~IObserver() {}
    virtual void update(StateChange change) = 0;
};

#endif // IOBSERVER_H