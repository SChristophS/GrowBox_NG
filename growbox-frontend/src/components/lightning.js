import React, { useContext } from 'react';
import { Container, Button, Row } from 'react-bootstrap';
import CycleInput from './CycleInput';

import { SettingsContext } from '../contexts/SettingsContext';

const Lightning = () => {
  const { ledSettings, setLedSettings } = useContext(SettingsContext);

  const handleLightCycleChange = (index, cycleType, name, value) => {
    const newLedCycles = [...ledSettings];
    newLedCycles[index] = { ...newLedCycles[index], [name]: value };
    setLedSettings(newLedCycles);
  };

  const handleAddLightCycle = () => {
    setLedSettings([...ledSettings, { durationOn: 0, durationOff: 0, ledRepetitions: 0 }]);
  };

  const handleDeleteLightCycle = (index) => {
    const newLedCycles = ledSettings.filter((_, i) => i !== index);
    setLedSettings(newLedCycles);
  };

  return (
    <Container>
      <h2>Light Cycles</h2>
      {ledSettings.map((lightCycle, index) => (
        <CycleInput
          key={index}
          cycleType="led"
          cycleIndex={index}
          cycle={lightCycle}
          onChange={handleLightCycleChange}
          onDelete={handleDeleteLightCycle}
        />
      ))}
      <Row className="justify-content-center">
        <Button onClick={handleAddLightCycle}>Add Light Cycle</Button>
      </Row>
    </Container>
  );
};

export default Lightning;
