import React, { createContext, useState, useEffect } from 'react';

export const SettingsContext = createContext();

const SettingsContextProvider = (props) => {
  const [ledSettings, setLedSettings] = useState([]);
  
  const updateLedSettings = (newSettings) => {
    setLedSettings(newSettings);
    saveSettings();
  };
  
  const [temperatureSettings, setTemperatureSettings] = useState([
    { temperature1: 0, duration1: 0 },
  ]);
  
  const [wateringSettings, setWateringSettings] = useState([]);
  const [growCycleName, setGrowCycleName] = useState('');
  const [description, setDescription] = useState('');
  const [sharingStatus, setSharingStatus] = useState('public');
  const [totalGrowTime, setTotalGrowTime] = useState(null);

  const saveSettings = () => {
    const settings = {
      ledSettings,
      temperatureSettings,
      wateringSettings,
      growCycleName,
      description,
      sharingStatus,
      totalGrowTime,
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
      setGrowCycleName(settings.growCycleName);
      setDescription(settings.description);
      setSharingStatus(settings.sharingStatus);
      setTotalGrowTime(settings.totalGrowTime);
      console.log('Settings loaded');
    }
  };
  

  useEffect(() => {
    loadSettings();
  }, []);

  return (
    <SettingsContext.Provider
      value={{
        ledSettings,
        setLedSettings,
        setLedSettings,
        updateLedSettings,
        temperatureSettings,
        setTemperatureSettings,
        wateringSettings,
        setWateringSettings,
        saveSettings,
        loadSettings,
        growCycleName,
        setGrowCycleName,
        description,
        setDescription,
        sharingStatus,
        setSharingStatus,
        totalGrowTime,
        setTotalGrowTime
      }}
    >
      {props.children}
    </SettingsContext.Provider>
  );
};

export default SettingsContextProvider;