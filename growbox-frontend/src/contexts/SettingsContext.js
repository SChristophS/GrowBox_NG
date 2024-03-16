import React, { createContext, useState, useEffect } from 'react';

export const SettingsContext = createContext();

const SettingsContextProvider = (props) => {
  const [isGrowPlanLoaded, setIsGrowPlanLoaded] = useState(false);
	
  
  const updateLedSettings = (newSettings) => {
    //setLedSettings(newSettings);
    //saveSettings();
  };
  

  

  
  const unloadSettings = () => {
	  console.log("unloadSettings ausgeführt");
		  

	  
	  console.log("unloadSettings ausgeführt, vor setIsGrowPlanLoaded:", isGrowPlanLoaded);
	  //setIsGrowPlanLoaded(false);

      console.log("unloadSettings ausgeführt, nach setIsGrowPlanLoaded:", isGrowPlanLoaded);

  
};



  

  useEffect(() => {
	console.log("Einstellungen beim Start geladen");
  }, []);

  return (
    <SettingsContext.Provider
      value={{
      }}
    >
      {props.children}
    </SettingsContext.Provider>
  );
};

export default SettingsContextProvider;
