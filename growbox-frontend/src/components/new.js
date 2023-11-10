import React, { useState, useContext } from "react";
import { SettingsContext } from '../contexts/SettingsContext';

import { Container, Row, Col, Button } from 'react-bootstrap';
import { useNavigate } from 'react-router-dom';


  
const GrowPlanManager = () => {
  
  const { 
    createSettings
  } = useContext(SettingsContext);
  
  let navigate = useNavigate();

  const handleCreateCycle = () => {
	createSettings();  
    navigate('/growplaner/General_zycle');
  };

  const handleCreateGrowPlan = () => {
    navigate('/create-growplan');
  };

  return (
    <Container>
      <Row className="my-4">
        <Col className="text-center">
          <h2>GrowPlan Manager</h2>
          <p>Verwalten Sie Ihre Zyklen und Growpläne.</p>
        </Col>
      </Row>
      <Row className="text-center">
        <Col md={{ span: 6, offset: 3 }}>
          <Button onClick={handleCreateCycle} className="mx-2">
            Neuen Zyklus erstellen
          </Button>
          <Button onClick={handleCreateGrowPlan} className="mx-2">
            Neuen Growplan erstellen
          </Button>
        </Col>
      </Row>
    </Container>
  );
};

export default GrowPlanManager;
