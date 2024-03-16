import React, { useContext, useState, useEffect } from "react";
import { Container, Table, Form, Row, Col, Button } from "react-bootstrap";
import { AuthContext } from "../contexts/AuthContext";
import { SettingsContext } from "../contexts/SettingsContext";
import { GrowCycleContext } from "../contexts/GrowCycleContext";

import RingComponent from "./RingComponent";
import GrowPlanServices from '../utility/GrowPlanServices';

const Load = () => {
  const { username } = useContext(AuthContext);
  const [growPlans, setGrowPlans] = useState([]);
  const [filteredGrowPlans, setFilteredGrowPlans] = useState([]);
  const [cyclePlans, setCyclePlans] = useState([]);
  const [filteredCyclePlans, setFilteredCyclePlans] = useState([]);
  const [showOnlyOwn, setShowOnlyOwn] = useState(true);
  const [statusMessage, setStatusMessage] = useState("");
  const [showTooltip, setShowTooltip] = useState(false);
  const [tooltipData, setTooltipData] = useState(null);
  const [tooltipPosition, setTooltipPosition] = useState({ x: 0, y: 0 });
  const [loadStatus, setLoadStatus] = useState("");
  const { isCyclePlanLoaded, setIsCyclePlanLoaded } = useContext(GrowCycleContext);


  const {
    setLedSettings,
    setTemperatureSettings,
    setWateringSettings,
    setGrowCycleName,
    setDescription,
    setSharingStatus,
    setTotalGrowTime,
  } = useContext(GrowCycleContext);
  


  const loadCyclePlan = (plan) => {
		console.log("loadCyclePlan aufgerufen");
		
		const loadedLedSettings = plan.growData.ledCycles;
		const loadedTemperatureSettings = plan.growData.tempCycles;
		const loadedWateringSettings = plan.growData.wateringCycles;
		setLoadStatus(`Grow-Plan '${plan.growCycleName}' erfolgreich geladen.`);
		
		// Setzen von isGrowPlanLoaded auf true, um zu signalisieren, dass ein Plan geladen wurde
		setIsCyclePlanLoaded(true);


		setLedSettings(loadedLedSettings);
		setTemperatureSettings(loadedTemperatureSettings);
		setWateringSettings(loadedWateringSettings);
		setGrowCycleName(plan.growCycleName);
		setDescription(plan.description);
		setSharingStatus(plan.sharingStatus);
		setTotalGrowTime(plan.growData.totalGrowTime);
  };
  
  
    
     const deleteCyclePlan = (plan) => {
	console.log(plan);
	console.log(plan._id);
	const deleteID = plan._id;
	const confirmDelete = window.confirm("Möchten Sie diesen Growplan wirklich löschen?");

	if (confirmDelete){
		GrowPlanServices.deleteCyclePlan(deleteID)
		  .then(response => {
			console.log(response);
			console.log("Jetzt im .then bereich");
			console.log(response.success);
			
			if (response.success){
				getCyclePlans();
			}
			
		  })
		  .catch(error => {
			setStatusMessage("Fehler beim Laden der GrowCycles");
		  });
	};
	  
	  
	
  };
  
  

  const [checkPassed, setCheckPassed] = useState(false);

  const checkGrowData = (data) => {
    // read the totalGrowTime
    let totalGrowTime = data.growData.totalGrowTime;

    // read the total LED Cycle times
    let totalLedTime = 0;
    let combinedDurationTimeLED = 0;
    let repetitions_LED = 0;

    for (let i = 0; i < data.growData.ledCycles.length; i++) {
      combinedDurationTimeLED = data.growData.ledCycles[i].durationOff + data.growData.ledCycles[i].durationOn;
      repetitions_LED = data.growData.ledCycles[i].ledRepetitions;

      totalLedTime = totalLedTime + (combinedDurationTimeLED * repetitions_LED);
    }

    // read the total Water Cycle times
    let totalWaterTime = 0;
    let combinedDurationTime_water = 0;
    let repetitionsWater = 0;
    for (let i = 0; i < data.growData.wateringCycles.length; i++) {
      combinedDurationTime_water = data.growData.wateringCycles[i].duration1 + data.growData.wateringCycles[i].duration2;
      repetitionsWater = data.growData.wateringCycles[i].waterRepetitions;

      totalWaterTime = totalWaterTime + (combinedDurationTime_water * repetitionsWater);
    }

    // read the total Temperature Cycle times
    let totalTemperatureTime = 0;
    let repetitionsTemperature = 0;
    for (let i = 0; i < data.growData.tempCycles.length; i++) {
      totalTemperatureTime = totalTemperatureTime + data.growData.tempCycles[i].duration;
    }

    if (totalGrowTime === totalLedTime && totalGrowTime === totalWaterTime && totalGrowTime === totalTemperatureTime) {
      return true;
    } else {
      return false;
    }
  };

  const handleMouseEnter = (event, growPlan) => {
    setTooltipData(growPlan);
    setTooltipPosition({ x: event.clientX + 10, y: event.clientY + 10 });
    setTimeout(() => setShowTooltip(true), 500); // Verzögerung hinzufügen
    setCheckPassed(checkGrowData(growPlan));
  };

  const handleMouseLeave = () => {
    setShowTooltip(false);
  };

  const getCyclePlans = async () => {
	  console.log("Load.js: getCyclePlans is called");
	  const result = await GrowPlanServices.getCyclePlans(username);
	  console.log("result:");
	  console.log(result);	
	  
	  if (result.success) {
		setCyclePlans(result.data);
		setFilteredCyclePlans(result.data.filter((plan) => plan.username === username));
		setStatusMessage(result.message);
	  } else {
		setStatusMessage(result.message);
	  }
	};
	/* 
  const getGrowPlans = async () => {
	  console.log("Load.js: getGrowPlans is called");
	  const result = await GrowPlanServices.getGrowPlans(username);
	  console.log(result);	
	  
	  if (result.success) {
		setGrowPlans(result.data);
		setFilteredGrowPlans(result.data.filter((plan) => plan.username === username));
		setStatusMessage(result.message);
	  } else {
		setStatusMessage(result.message);
	  }
	};	 */


  useEffect(() => {
    getCyclePlans();
  }, []);

  useEffect(() => {
    if (showOnlyOwn) {
      setFilteredGrowPlans(growPlans.filter((plan) => plan.username === username));
    } else {
      setFilteredGrowPlans(growPlans);
    }
  }, [showOnlyOwn, growPlans, username]);

  return (
    <Container>
      <h2>Laden</h2>
      <Row>
        <Col>
          <Form.Group controlId="showOwnPlans">
            <Form.Check
              type="checkbox"
              label="Nur eigene Pläne anzeigen"
              checked={showOnlyOwn}
              onChange={() => setShowOnlyOwn(!showOnlyOwn)}
            />
          </Form.Group>
        </Col>
        <Col>
          <p>{statusMessage}</p>
          <p>{loadStatus}</p>
        </Col>
      </Row>
      <Table striped bordered hover size="sm">
        <thead>
          <tr>
            <th>Benutzername</th>
            <th>Growname</th>
            <th>Beschreibung</th>
            <th>Sharing Status</th>
            <th>Aktionen</th>
          </tr>
        </thead>
        <tbody>
          {(filteredCyclePlans || []).map((plan) => (
            <tr
              key={`${plan.username}-${plan.growCycleName}`}
              onMouseEnter={(e) => handleMouseEnter(e, plan)}
              onMouseLeave={handleMouseLeave}
            >
              <td>{plan.username}</td>
              <td>{plan.growCycleName}</td>
              <td>{plan.description}</td>
              <td>{plan.sharingStatus ? "Geteilt" : "Privat"}</td>
              <td>
                <Button variant="primary" size="sm" onClick={() => loadCyclePlan(plan)}>
                  Laden
                </Button>
                {' '}
                <Button variant="danger" size="sm" onClick={() => deleteCyclePlan(plan)}>
                  Löschen
                </Button>
              </td>
            </tr>
          ))}
        </tbody>
      </Table>
      {showTooltip && (
        <div
          style={{
            position: "fixed",
            top: tooltipPosition.y,
            left: tooltipPosition.x,
            background: "white",
            border: "1px solid black",
            boxShadow: "0 4px 6px rgba(0, 0, 0, 0.1)",
            padding: "10px",
            borderRadius: "4px",
            zIndex: 10,
          }}
        >
          {checkPassed && <RingComponent growData={tooltipData.growData} />}
          <p><strong>Growname:</strong> {tooltipData.growCycleName}</p>
          <p><strong>Beschreibung:</strong> {tooltipData.description}</p>
          <p>
            <strong>Check:</strong>{" "}
            {checkPassed ? (
              <span style={{ color: "green" }}>Der Growplan hat den Test bestanden. Die Gesamtzeiten für LED-, Wasser- und Temperaturzyklen entsprechen der totalen Wachstumszeit.</span>
            ) : (
              <span style={{ color: "red" }}>Der Growplan hat den Test nicht bestanden. Die Gesamtzeiten für LED-, Wasser- und Temperaturzyklen stimmen nicht mit der totalen Wachstumszeit überein.</span>
            )}
          </p>
        </div>
      )}
    </Container>
  );
};

export default Load;