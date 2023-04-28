import React, { useContext } from 'react';
import { SettingsContext } from "../contexts/SettingsContext";
import RingComponent from "./RingComponent";

const GrowplanAnalyse = () => {
  const { ledSettings, wateringSettings, temperatureSettings } = useContext(SettingsContext);

  let totalGrowTime = 0;
  for (const ledCycle of ledSettings) {
    const { durationOn, durationOff, ledRepetitions } = ledCycle;
    totalGrowTime += (durationOn + durationOff) * ledRepetitions;
  }

  const growData = {
    totalGrowTime,
    ledCycles: ledSettings,
    wateringCycles: wateringSettings,
    tempCycles: temperatureSettings,
  };

  return (
    <div>
      <h2>Grow Plan Analyse</h2>
      <RingComponent growData={growData} />
    </div>
  );
};

export default GrowplanAnalyse;