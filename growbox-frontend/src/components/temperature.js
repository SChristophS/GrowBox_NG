import React, { useContext } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';

import { SettingsContext } from '../contexts/SettingsContext';

const Temperature = () => {
  const { temperatureSettings, setTemperatureSettings } = useContext(SettingsContext);

  const handleTemperatureCycleChange = (index, cycleType, name, value) => {
    const newTemperatureCycles = [...temperatureSettings];
    newTemperatureCycles[index] = { ...newTemperatureCycles[index], [name]: value };
    setTemperatureSettings(newTemperatureCycles);
  };

  const handleAddTemperatureCycle = () => {
    setTemperatureSettings([...temperatureSettings, { temperature1: 0, duration1: 0, temperature2: 0, duration2: 0 }]);
  };

  const handleDeleteTemperatureCycle = (index) => {
    const newTemperatureCycles = temperatureSettings.filter((_, i) => i !== index);
    setTemperatureSettings(newTemperatureCycles);
  };

  return (
    <Container>
      <h2>Temperature Cycles</h2>
      {temperatureSettings.map((temperatureCycle, index) => (
        <CycleInput
          key={index}
          cycleType="temperature"
          cycleIndex={index}
          cycle={temperatureCycle}
          onChange={handleTemperatureCycleChange}
          onDelete={handleDeleteTemperatureCycle}
        />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddTemperatureCycle}>Add Temperature Cycle</Button>
      </Row>
    </Container>
  );
};

export default Temperature;
