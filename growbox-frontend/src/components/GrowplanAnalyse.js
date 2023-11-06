import React, { useContext } from 'react';
import { SettingsContext } from "../contexts/SettingsContext";
import RingComponent from "./RingComponent";
import { Container, Row, Col } from 'react-bootstrap';
import GrowplanChecks, { calculateChecks } from './GrowplanChecks';

const GrowplanAnalyse = () => {
    const { ledSettings, wateringSettings, temperatureSettings, totalGrowTime } = useContext(SettingsContext);

    const growData = {
        totalGrowTime,
        ledCycles: ledSettings,
        wateringCycles: wateringSettings,
        tempCycles: temperatureSettings,
    };

    const { totalLightOnDuration, totalWateringFullDuration, minTemperature, maxTemperature } = calculateChecks(growData);

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
                    <GrowplanChecks growData={growData} />
                </Col>
            </Row>
        </Container>
    );
};

export default GrowplanAnalyse;
