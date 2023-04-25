import React, { createContext, useState } from 'react';

export const SettingsContext = createContext();

const SettingsContextProvider = (props) => {
  const [ledSettings, setLedSettings] = useState([]);
  const [temperatureSettings, setTemperatureSettings] = useState([]);
  const [wateringSettings, setWateringSettings] = useState([]);

  const saveSettings = () => {
    const settings = {
      ledSettings,
      temperatureSettings,
      wateringSettings,
    };
    localStorage.setItem('settings', JSON.stringify(settings));
    console.log('Settings saved');
  };

  const loadSettings = () => {
    const storedSettings = localStorage.getItem('settings');
    if (storedSettings) {
      const settings = JSON.parse(storedSettings);
      setLedSettings(settings.ledSettings);
      setTemperatureSettings(settings.temperatureSettings);
      setWateringSettings(settings.wateringSettings);
      console.log('Settings loaded');
    }
  };

  return (
    <SettingsContext.Provider
      value={{
        ledSettings,
        setLedSettings,
        temperatureSettings,
        setTemperatureSettings,
        wateringSettings,
        setWateringSettings,
        saveSettings,
        loadSettings,
      }}
    >
      {props.children}
    </SettingsContext.Provider>
  );
};

export default SettingsContextProvider;
