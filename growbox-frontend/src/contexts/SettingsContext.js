import React, { createContext, useState, useEffect } from 'react';

export const SettingsContext = createContext();

const SettingsContextProvider = (props) => {
  const [isGrowPlanLoaded, setIsGrowPlanLoaded] = useState(false);
	
  const [ledSettings, setLedSettings] = useState([]);
  
  const updateLedSettings = (newSettings) => {
    setLedSettings(newSettings);
    saveSettings();
  };
  
  const [temperatureSettings, setTemperatureSettings] = useState([
    { temperature: 0, duration: 0 },
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
    console.log(settings);
	console.log('Settings in die Anwendung geladen');
	
  };
  
  const generateRandomNumber = () => {
  // Erzeugt eine zufällige Nummer zwischen 100000 und 999999
  return Math.floor(100000 + Math.random() * 900000);
};

 
  const createSettings = () => {
    unloadSettings();
	
	const randomNumber = generateRandomNumber();
	const growCycleName = `Growzyklus_${randomNumber}`;
	setGrowCycleName(growCycleName);
	setIsGrowPlanLoaded(true);
	console.log("createSettings ausgeführt");
  };  
  
  const unloadSettings = () => {
	  console.log("unloadSettings ausgeführt");
		  
	  setLedSettings([]);
	  setTemperatureSettings([{ temperature: 0, duration: 0 }]);
	  setWateringSettings([]);
	  setGrowCycleName('');
	  setDescription('');
	  setSharingStatus('public');
	  setTotalGrowTime(0);
	  
	  console.log("unloadSettings ausgeführt, vor setIsGrowPlanLoaded:", isGrowPlanLoaded);
	  setIsGrowPlanLoaded(false);

      console.log("unloadSettings ausgeführt, nach setIsGrowPlanLoaded:", isGrowPlanLoaded);

  
};


  const loadSettings = () => {
	  console.log("LoadSettings aufgerufen");
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
	  console.log(settings.growCycleName)
    }
  };
  

  useEffect(() => {
    loadSettings();
	console.log("Einstellungen beim Start geladen");
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
        setTotalGrowTime,
        isGrowPlanLoaded,
        setIsGrowPlanLoaded,
		unloadSettings,
		createSettings
      }}
    >
      {props.children}
    </SettingsContext.Provider>
  );
};

export default SettingsContextProvider;
