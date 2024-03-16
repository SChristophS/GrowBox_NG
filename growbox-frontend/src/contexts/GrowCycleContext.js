// Dies ist die GrowCycleContext
//
// Context-File für alles was mit den GrowCycle zu tun hat
import React, { createContext, useState } from 'react';
import GrowPlanServices from '../utility/GrowPlanServices';

// Erstellen des Contexts
export const GrowCycleContext = createContext();

export const GrowCycleProvider = ({ children }) => {
	// Definieren Sie hier die Zustandsvariablen
    const [isCyclePlanLoaded, setIsCyclePlanLoaded] = useState(false);
	const [ledSettings, setLedSettings] = useState([]);
	const [temperatureSettings, setTemperatureSettings] = useState(
		[{ temperature: 0, duration: 0 }]
	);
	const [wateringSettings, setWateringSettings] = useState([]);
	const [growCycleName, setGrowCycleName] = useState('');
	const [description, setDescription] = useState('');
	const [sharingStatus, setSharingStatus] = useState('public');
	const [totalGrowTime, setTotalGrowTime] = useState(null);	
	
	const unloadCycleSettings = () => {
		console.log("GrowCycleContext.js: unloadCycleSettings ausgeführt");
		setLedSettings([]);
		setTemperatureSettings([{ temperature: 0, duration: 0 }]);
		setWateringSettings([]);
		setGrowCycleName('');
		setDescription('');
		setSharingStatus('public');
		setTotalGrowTime(0);
		setIsCyclePlanLoaded(false);
	};

	const createEmptyCycleSettings = () => {
		console.log("GrowCycleContext: Aufruf von createEmptyCycleSettings");
		
		unloadCycleSettings();

		const randomNumber = generateRandomNumber();
		const growCycleName = `Growzyklus_${randomNumber}`;
		console.log("GrowCycleContext: growCycleName : ", growCycleName);
		setGrowCycleName(growCycleName);
		setIsCyclePlanLoaded(true);
	};

	const generateRandomNumber = () => {
		// Erzeugt eine zufällige Nummer zwischen 100000 und 999999
		return Math.floor(100000 + Math.random() * 900000);
	};	
  
	
	// Funktion zum Laden der Cyclepläne
	const loadCyclePlans = async (username) => {
		console.log("GrowCycleContext: Aufruf von loadCyclePlans, username :", username);
	
		const result = await GrowPlanServices.getCyclePlans(username);
		console.log(result);
		
		if (result.success) {
			console.log("GrowCycleContext: Cycle Plans geladen:", result.data);

			const formattedCyclePlans = result.data.map(plan => ({
				id: plan._id,
				content: plan.growCycleName,
				description: plan.description,
				totalGrowTime: plan.growData.totalGrowTime
			}));
		
			//setGrowPlans(formattedCyclePlans);
			console.log("GrowCycleContext: formattedCyclePlans: ", formattedCyclePlans);
		} else {
			console.error(result.message);
		}
	};
	


	return (
		<GrowCycleContext.Provider value={{ 
			ledSettings,
			setLedSettings,
			temperatureSettings,
			setTemperatureSettings,
			wateringSettings,
			setWateringSettings,
			growCycleName,
			setGrowCycleName,
			description,
			setDescription,
			sharingStatus,
			setSharingStatus,
			totalGrowTime,
			setTotalGrowTime,
			isCyclePlanLoaded, 
			setIsCyclePlanLoaded, 
			unloadCycleSettings,
			createEmptyCycleSettings
		}}>
			{children}
		</GrowCycleContext.Provider>
	);
};

export default GrowCycleProvider;
