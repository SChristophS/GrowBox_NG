#ifndef GROWBOXSTATE_H
#define GROWBOXSTATE_H

#include "IObserver.h"
#include <list>

class GrowBoxState {
private:
    std::list<IObserver *> observers;

    // Growbox run status
    bool growboxRunning;

    // Websocket
    bool soll_websocketConnection;
    bool ist_websocketConnection;

    // LED Status
    bool soll_ledState;
    bool ist_ledState;

    // Water Reservoir
    bool soll_waterReservoir;
    bool ist_waterReservoir;

    // air temperature
    float soll_airTemperature;
    float ist_airTemperature;

    // air humidity
    float soll_humidity;
    float ist_humidity;

    // weight
    float ist_weight;

    // pump Status
    bool ist_InflowWaterPump;
    bool ist_OutflowWaterPump;

public:
    void attach(IObserver *observer) {
        observers.push_back(observer);
    }

    void detach(IObserver *observer) {
        observers.remove(observer);
    }

    void notify(StateChange change) {
        for (IObserver* observer : observers) {
            observer->update(change);
        }
    }

    // Growbox Running State
    bool set_growboxRunning(bool state) {
        growboxRunning  = state;
        return growboxRunning;
    }
    bool get_growboxRunning() const {
        return growboxRunning;
    }

    // websocket Connection
    bool set_soll_websocketConnection(bool state) {
        soll_websocketConnection  = state;
        return soll_websocketConnection;
    }
    bool get_soll_websocketConnection() const {
        return soll_websocketConnection;
    }
    bool set_ist_websocketConnection(bool state) {
        ist_websocketConnection  = state;
        return ist_websocketConnection;
    }
    bool get_ist_websocketConnection() const {
        return ist_websocketConnection;
    }

    // LED State
    void set_SollLedState(bool state) {
        soll_ledState = state;
        notify(StateChange::soll_LED);
    }
    bool get_SollLedState() const {
        return soll_ledState;
    }
    void set_IstLedState(bool state) {
        ist_ledState = state;
    }
    bool get_IstLedState() const {
        return ist_ledState;
    }

    // WaterState
    void set_SollWaterReservoirState(bool state) {
        soll_waterReservoir = state;
        notify(StateChange::soll_waterReservoir);
    }
    bool get_SollWaterReservoirState() const {
        return soll_waterReservoir;
    }    
    void set_IstWaterReservoirState(bool state) {
        ist_waterReservoir = state;
        notify(StateChange::ist_waterReservoir);
    }
    bool get_IstWaterReservoirState() const {
        return ist_waterReservoir;
    }

    // Temperature
    void set_soll_airTemperature(float temp) {
        soll_airTemperature = temp;
    }
    float get_soll_airTemperature() const {
        return soll_airTemperature;
    }
    void set_ist_airTemperature(float temp) {
        ist_airTemperature = temp;
    }
    float get_soll_airTemperature() const {
        return soll_airTemperature;
    }

    // Humidity
    void set_Ist_Humidity(float hum) {
        ist_humidity = hum;
    }
    float get_Ist_Humidity() const {
        return ist_humidity;
    }
    void set_Soll_Humidity(float hum) {
        soll_humidity = hum;
    } 
    float get_Soll_Humidity() const {
        return soll_humidity;
    }

    // Pump status
    void set_ist_InflowWaterPump(bool state) {
        ist_InflowWaterPump = state;
    }
    bool get_ist_InflowWaterPump() const {
        return ist_InflowWaterPump;
    }
    void set_ist_OutflowWaterPump(bool state) {
        ist_OutflowWaterPump = state;
    }    
    bool get_ist_OutflowWaterPump() const {
        return ist_OutflowWaterPump;
    }
};

#endif // GROWBOXSTATE_H