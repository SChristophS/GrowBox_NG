import React, { useContext } from 'react';
import { SettingsContext } from '../contexts/SettingsContext';

const SendToGrowbox = () => {
	
	const { growCycleName, totalGrowTime, ledSettings, temperatureSettings, wateringSettings, description, sharingStatus} = useContext(
		SettingsContext
	);
	
	const growPlan = {
      growCycleName,
      growData: {
        totalGrowTime : totalGrowTime,
        ledCycles: ledSettings,
        tempCycles: temperatureSettings,
        wateringCycles: wateringSettings,
      }
	};
	
	console.log(growPlan);
  
  return (
    <div>
      <h1>Noch nicht implementiert</h1>
	  <pre>{JSON.stringify(growPlan, null, 2)}</pre>
    </div>
  );
};

export default SendToGrowbox;
