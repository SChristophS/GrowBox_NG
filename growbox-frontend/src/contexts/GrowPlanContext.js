import React, { createContext, useState } from 'react';
import GrowPlanServices from '../utility/GrowPlanServices';

// Erstellen des Contexts
export const GrowPlanContext = createContext();

export const GrowPlanProvider = ({ children }) => {
  const [growPlans, setGrowPlans] = useState([]);
  const [loadedGrowPlan, setLoadedGrowPlan] = useState(null); // Neuer Zustand für geladenen GrowPlan

  // Funktion zum Laden der Growpläne
const loadGrowPlans = async (username) => {
	console.log("loadGrowPlans aufgerufen mit username: ", username);
  const result = await GrowPlanServices.getCyclePlans(username);
  console.log(result);
  if (result.success) {
    console.log('Cycle Plans geladen:', result.data);
    const formattedCyclePlans = result.data.map(plan => ({
      id: plan._id,
      content: plan.growCycleName,
      description: plan.description,
      totalGrowTime: plan.growData.totalGrowTime
    }));
    setGrowPlans(formattedCyclePlans);
	console.log("formattedCyclePlans: ", formattedCyclePlans);
	
  } else {
    console.error(result.message);
  }
};


  // Kontext-Werte bereitstellen
  return (
    <GrowPlanContext.Provider value={{ loadGrowPlans, growPlans, setGrowPlans, loadedGrowPlan, setLoadedGrowPlan }}>
      {children}
    </GrowPlanContext.Provider>
  );
};
