import React, { useContext } from 'react';
import { SettingsContext } from "../contexts/SettingsContext";
import RingComponent from "./RingComponent";
import { Container, Row, Col } from 'react-bootstrap';

const GrowplanAnalyse = () => {
  const { ledSettings, wateringSettings, temperatureSettings, totalGrowTime } = useContext(SettingsContext);

  const growData = {
    totalGrowTime,
    ledCycles: ledSettings,
    wateringCycles: wateringSettings,
    tempCycles: temperatureSettings,
  };

  const totalLightOnDuration = ledSettings.reduce((acc, cycle) => acc + cycle.durationOn * cycle.ledRepetitions, 0);
  
  const totalWateringFullDuration = wateringSettings.reduce((acc, cycle) => {
    const fullDuration = cycle.status1 === 'full' ? cycle.duration1 * cycle.waterRepetitions : cycle.duration2 * cycle.waterRepetitions;
    return acc + fullDuration;
  }, 0);


  const totalWateringDefinedDuration = wateringSettings.reduce((acc, cycle) => {
    const fullDuration = cycle.duration1 * cycle.waterRepetitions;
    const emptyDuration = cycle.duration2 * cycle.waterRepetitions;
    return acc + fullDuration + emptyDuration;
  }, 0);
  
  
const testWateringDuration = totalGrowTime === totalWateringDefinedDuration;
  
  

  console.log("totalWateringFullDuration = " + totalWateringFullDuration)
  

  const [minTemperature, maxTemperature] = temperatureSettings.reduce(([min, max], cycle) => [Math.min(min, cycle.temperature1), Math.max(max, cycle.temperature1)], [Infinity, -Infinity]);

  const testLightDuration = totalGrowTime === (totalLightOnDuration + ledSettings.reduce((acc, cycle) => acc + cycle.durationOff * cycle.ledRepetitions, 0));
  const testTemperatureDuration = totalGrowTime === temperatureSettings.reduce((acc, cycle) => acc + cycle.duration1, 0);

  console.log(growData)
  return (
    <Container>
  <h2 className="text-center section-spacing">Grow Plan Analyse</h2>
  <Row className="section-spacing">
    <Col xs={12} md={6} className="d-flex justify-content-center">
      <RingComponent growData={growData} />
    </Col>
    <Col xs={12} md={6} className="d-flex flex-column justify-content-center">
      <h3 className="section-spacing custom-h3">Details</h3>
      <p>Growdauer: {totalGrowTime} Minuten</p>
      <p>Beleuchtungsdauer: {totalLightOnDuration} Minuten</p>
      <p>Bewässerungsdauer (Becken voll): {totalWateringFullDuration} Minuten</p>
      <p>Temperatur: {minTemperature}°C - {maxTemperature}°C</p>
    </Col>
  </Row>

      <h3 className="text-center">Checks</h3>
      <Row className="justify-content-center">
        <Col xs={12} md={6}>
        <p className="text-center">
  Test LED-Dauer: <span style={{ color: testLightDuration ? 'green' : 'red' }}>{testLightDuration ? 'passed' : 'failed'}</span>
  <br />
  (Die Gesamtdauer der LED-Zyklen sollte der Wachstumszeit entsprechen.)
</p>
<p className="text-center">
  Test Bewässerungsdauer: <span style={{ color: testWateringDuration ? 'green' : 'red' }}>{testWateringDuration ? 'passed' : 'failed'}</span>
  <br />
  (Die Gesamtdauer der Bewässerungszyklen sollte der Wachstumszeit entsprechen.)
</p>
<p className="text-center">
  Test Temperaturdauer: <span style={{ color: testTemperatureDuration ? 'green' : 'red' }}>{testTemperatureDuration ? 'passed' : 'failed'}</span>
  <br />
  (Die Gesamtdauer der Temperaturzyklen sollte der Wachstumszeit entsprechen.)
</p>
        </Col>
      </Row>
    </Container>
  );
};

export default GrowplanAnalyse;