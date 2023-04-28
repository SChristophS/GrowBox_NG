import React, { useContext } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';

import { SettingsContext } from '../contexts/SettingsContext';

const Temperature = () => {
  const { temperatureSettings, setTemperatureSettings } = useContext(SettingsContext);

  const { totalGrowTime } = useContext(SettingsContext);

  const handleTemperatureCycleChange = (index, cycleType, name, value) => {
    const newTemperatureCycles = [...temperatureSettings];
    newTemperatureCycles[index] = { ...newTemperatureCycles[index], [name]: value };
    setTemperatureSettings(newTemperatureCycles);
  };

  const handleAddTemperatureCycle = () => {
    // Remove temperature2 and duration2 from the new cycle
    setTemperatureSettings([...temperatureSettings, { temperature1: 0, duration1: 0 }]);
  };

  const handleDeleteTemperatureCycle = (index) => {
    const newTemperatureCycles = temperatureSettings.filter((_, i) => i !== index);
    setTemperatureSettings(newTemperatureCycles);
  };

  const totalTemperatureDuration = temperatureSettings.reduce((acc, cycle) => {
    return acc + cycle.duration1;
  }, 0);
  

  return (
    <Container>
      <h2>Temperature Cycles</h2>
      <p>
        Total Grow Time:{' '}
        {totalGrowTime !== undefined ? `${totalGrowTime} Minuten` : 'Noch keine Gesamtdauer festgelegt'}
      </p>
      <p>Aktuelle Gesamtdauer Temperature Zyklen: {totalTemperatureDuration} Minuten</p>
      {temperatureSettings.map((temperatureCycle, index) => (
       <CycleInput
        key={index}
        cycleType="temperature"
        cycleIndex={index}
        cycle={temperatureCycle}
        onChange={handleTemperatureCycleChange}
        onDelete={handleDeleteTemperatureCycle}
        showDuration
      />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddTemperatureCycle}>Add Temperature Cycle</Button>
      </Row>
    </Container>
  );
};

export default Temperature;
