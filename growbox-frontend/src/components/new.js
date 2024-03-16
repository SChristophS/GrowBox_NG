import React, { useContext } from "react";
import { useNavigate } from 'react-router-dom';
import { Container, Row, Col, Button } from 'react-bootstrap';
import { SettingsContext } from '../contexts/SettingsContext';
import { GrowPlanContext } from '../contexts/GrowPlanContext'; // Import des GrowPlanContexts
import { GrowCycleContext } from '../contexts/GrowCycleContext'; // Import des GrowPlanContexts

const GrowPlanManager = () => {
	const { createEmptyCycleSettings } = useContext(GrowCycleContext);
	const { setLoadedGrowPlan } = useContext(GrowPlanContext); // Zugriff auf setLoadedGrowPlan
	let navigate = useNavigate();

	const handleCreateCycle = () => {
		createEmptyCycleSettings();  
		navigate('/growplaner/GrowCycleOverview');
	};

	const handleCreateGrowPlan = () => {
		console.log("new.js: handleCreateGrowPlan is called");
		setLoadedGrowPlan(null); // Zurücksetzen des geladenen GrowPlans
		navigate('/growplaner/CreateGrowPlan');
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
