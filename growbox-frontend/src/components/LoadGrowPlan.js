import React, { useContext, useState, useEffect } from "react";
import { Container, Table, Form, Row, Col, Button } from "react-bootstrap";
import GrowPlanServices from '../utility/GrowPlanServices';
import { AuthContext } from "../contexts/AuthContext";
import { GrowPlanContext } from '../contexts/GrowPlanContext';
import { useNavigate } from 'react-router-dom';


const LoadGrowCycle = () => {
	  let navigate = useNavigate();

	
  const [growCycles, setGrowCycles] = useState([]);
  const [filteredGrowCycles, setFilteredGrowCycles] = useState([]);
  const [statusMessage, setStatusMessage] = useState("");
  const [loading, setLoading] = useState(false);
  
  const { username } = useContext(AuthContext);
  console.log("username = " + username);

  const { setLoadedGrowPlan } = useContext(GrowPlanContext); // Hier den Kontext verwenden

  useEffect(() => {
	loadGrowPlans();
  }, []);
  
    const loadGrowPlans = () => {
		setLoading(true);
		GrowPlanServices.getGrowPlans(username)
    .then(response => {
		console.log("response from loadGrowPlans:", response)
        setGrowCycles(response.data);
        setFilteredGrowCycles(response.data);
        setLoading(false);
      })
      .catch(error => {
        setStatusMessage("Fehler beim Laden der GrowCycles");
        setLoading(false);
      });
  };

  const handleFilterChange = (e) => {
    const filterValue = e.target.value;
    const filtered = growCycles.filter(cycle =>
      cycle.growPlanName.includes(filterValue) ||
      cycle.growPlanDescription.includes(filterValue)
    );
    setFilteredGrowCycles(filtered);
  };
  
   const handleInfoGrowCycle = (e) => {
	console.log(e);
  };
  
  
// Angepasste handleLoadGrowCycle Funktion
const handleLoadGrowCycle = async (cycle) => {
  // Setzen des geladenen Grow Plans im Kontext
  setLoadedGrowPlan(cycle);

  // Abrufen der totalGrowTime für jeden Zyklus
  const updatedDroppedItems = await Promise.all(cycle.droppedItems.map(async (item) => {
    const totalTimeResponse = await GrowPlanServices.getCycleTotalTime(item.id);

    return {
      ...item,
      totalGrowTime: totalTimeResponse.success ? totalTimeResponse.totalGrowTime : 0
    };
  }));

  // Aktualisieren des geladenen Grow Plans im Kontext mit der totalGrowTime
  setLoadedGrowPlan({ ...cycle, droppedItems: updatedDroppedItems });

  // Navigieren zur CreateGrowPlan-Komponente
  navigate('/growplaner/CreateGrowPlan');
};



  
  
     const handleDeleteGrowCycle = (e) => {
	console.log(e);
	const confirmDelete = window.confirm("Möchten Sie diesen Growplan wirklich löschen?");
	console.log(e._id);
	const deleteID = e._id;
	
	
	GrowPlanServices.deleteGrowPlan(deleteID)
      .then(response => {
		console.log(response);
		console.log("Jetzt im .then bereich");
        setLoading(false);
		console.log(response.success);
		
		if (response.success){
			loadGrowPlans();
		}
		
      })
      .catch(error => {
        setStatusMessage("Fehler beim Laden der GrowCycles");
        setLoading(false);
      });
	  
	  
	
  };
  
  
  
  
  

  return (
    <Container>
      <Row>
        <Col>
          <Form.Control
            type="text"
            placeholder="Suche GrowCycle"
            onChange={handleFilterChange}
          />
        </Col>
      </Row>
      <Row>
        <Col>
 <Table striped bordered hover>
        <thead>
          <tr>
            <th>Benutzername</th>
            <th>GrowPlan-Name</th>
            <th>Beschreibung</th>
            <th>Gesamtwachstumsdauer</th>
            <th>Aktionen</th>
          </tr>
        </thead>
        <tbody>
          {filteredGrowCycles.map((cycle, index) => (
            <tr key={index}>
              <td>{cycle.username}</td>
              <td>{cycle.growPlanName}</td>
              <td>{cycle.growPlanDescription}</td>
              <td>{cycle.totalGrowDuration}</td>
              <td>
                <Button variant="info" size="sm" onClick={() => handleInfoGrowCycle(cycle)}>Info</Button>{' '}
                <Button variant="primary" size="sm" onClick={() => handleLoadGrowCycle(cycle)}>Laden</Button>{' '}
                <Button variant="danger" size="sm" onClick={() => handleDeleteGrowCycle(cycle)}>Löschen</Button>
              </td>
            </tr>
          ))}
        </tbody>
      </Table>
        </Col>
      </Row>
      {statusMessage && <Row><Col>{statusMessage}</Col></Row>}
    </Container>
  );
};

export default LoadGrowCycle;
