import React, { useContext} from "react";
import { Container, Form } from "react-bootstrap";
import { SettingsContext } from "../contexts/SettingsContext";
import { AuthContext } from '../contexts/AuthContext';

const General = () => {
  const {
    growCycleName,
    setGrowCycleName,
    description,
    setDescription,
    sharingStatus,
    setSharingStatus,
    totalGrowTime,
    setTotalGrowTime,
  } = useContext(SettingsContext);

  const { username } = useContext(AuthContext);

  const handleTotalGrowTimeChange = (value) => {
    setTotalGrowTime(value * 60);
  };

  return (
    <Container>
      <h2>Allgemein</h2>
      <Form>
      <p>Benutzername: {username}</p>

        <Form.Group>
          <Form.Label>Growzyklus Name</Form.Label>
          <Form.Control
            type="text"
            value={growCycleName}
            onChange={(e) => setGrowCycleName(e.target.value)}
          />
        </Form.Group>
        <Form.Group>
          <Form.Label>Beschreibung</Form.Label>
          <Form.Control
            type="text"
            value={description}
            onChange={(e) => setDescription(e.target.value)}
          />
        </Form.Group>
        <Form.Group>
          <Form.Label>Freigabestatus</Form.Label>
          <Form.Control
            as="select"
            value={sharingStatus}
            onChange={(e) => setSharingStatus(e.target.value)}
          >
            <option value="public">public</option>
            <option value="private">private</option>
          </Form.Control>
        </Form.Group>
        <Form.Group>
        <Form.Label>TotalGrowTime (in Stunden)</Form.Label>
            <Form.Control
                type="number"
                value={totalGrowTime ? totalGrowTime / 60 : ''}
                onChange={(e) => handleTotalGrowTimeChange(parseFloat(e.target.value))}
            />
        </Form.Group>
      </Form>
    </Container>
  );
};

export default General;
