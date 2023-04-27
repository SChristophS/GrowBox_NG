import React, { useContext, useState, useEffect } from "react";
import { Container, Table, Form, Row, Col } from "react-bootstrap";
import { AuthContext } from "../contexts/AuthContext";


const Load = () => {
  const { username } = useContext(AuthContext);
  const [growPlans, setGrowPlans] = useState([]);
  const [filteredGrowPlans, setFilteredGrowPlans] = useState([]);
  const [showOnlyOwn, setShowOnlyOwn] = useState(true);
  const [statusMessage, setStatusMessage] = useState("");

  useEffect(() => {
    fetch(`http://localhost:5000/get-grow-plans/${username}`, {
      method: "GET",
      credentials: "include",
    })
      .then((response) => response.json())
      .then((data) => {
        if (data.status === "success") {
          setGrowPlans(data.data);
          setFilteredGrowPlans(data.data.filter((plan) => plan.username === username));
          setStatusMessage("Grow-Pläne erfolgreich geladen.");
        } else {
          setStatusMessage(`Fehler beim Laden der Grow-Pläne: ${data.message}`);
        }
      })
      .catch((error) => {
        console.error("Error:", error);
        setStatusMessage("Fehler beim Laden der Grow-Pläne.");
      });
  }, []);

  useEffect(() => {
    if (showOnlyOwn) {
      setFilteredGrowPlans(growPlans.filter((plan) => plan.username === username));
    } else {
      setFilteredGrowPlans(growPlans);
    }
  }, [showOnlyOwn]);

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
      </Row>
      <Table striped bordered hover size="sm">
        <thead>
          <tr>
            <th>Benutzername</th>
            <th>Growname</th>
            <th>Beschreibung</th>
          </tr>
        </thead>
        <tbody>
          {filteredGrowPlans.map((plan) => (
            <tr key={`${plan.username}-${plan.growCycleName}`}>
              <td>{plan.username}</td>
              <td>{plan.growCycleName}</td>
              <td>{plan.description}</td>
            </tr>
          ))}
        </tbody>
      </Table>
      <p>{statusMessage}</p>
    </Container>
  );
};


export default Load;
