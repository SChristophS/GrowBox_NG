import React, { useContext } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';
import { SettingsContext } from '../contexts/SettingsContext';
const Watering = () => {
  const { wateringSettings, setWateringSettings } = useContext(SettingsContext);

  const handleWaterCycleChange = (index, cycleType, name, value) => {
    const newWaterCycles = [...wateringSettings];
    newWaterCycles[index] = { ...newWaterCycles[index], [name]: value };
    setWateringSettings(newWaterCycles);
  };

  const handleAddWaterCycle = () => {
    setWateringSettings([...wateringSettings, { status1: 'full', duration1: 0, status2: 'empty', duration2: 0, waterRepetitions: 0 }]);
  };

 
  const { totalGrowTime } = useContext(SettingsContext);

  const handleDeleteWaterCycle = (index) => {
    const newWaterCycles = wateringSettings.filter((_, i) => i !== index);
    setWateringSettings(newWaterCycles);
  };

  const totalWaterDuration = wateringSettings.reduce((acc, cycle) => {
    return acc + (cycle.duration1 + cycle.duration2) * cycle.waterRepetitions;
  }, 0);
  


  return (
    <Container>
      <h2>Water Cycles</h2>
      <p>
        Total Grow Time:{' '}
        {totalGrowTime !== undefined ? `${totalGrowTime} Minuten` : 'Noch keine Gesamtdauer festgelegt'}
      </p>
      <p>Aktuelle Gesamtdauer Wasser Zyklen: {totalWaterDuration} Minuten</p>
      {wateringSettings.map((waterCycle, index) => (
        <CycleInput
          key={index}
          cycleType="water"
          cycleIndex={index}
          cycle={waterCycle}
          onChange={handleWaterCycleChange}
          onDelete={handleDeleteWaterCycle}
          showDuration
      />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddWaterCycle}>Add Water Cycle</Button>
      </Row>
    </Container>
  );
};

export default Watering;
