import React, { useState } from 'react';
import { Card, Button, Col, Form, Row, InputGroup } from 'react-bootstrap';

const CycleInput = ({ cycleType, cycleIndex, cycle, onChange, onDelete, showDuration }) => {
  const handleChange = (name, value) => {
    onChange(cycleIndex, cycleType, name, value);
  };
  const [isCollapsed, setIsCollapsed] = useState(false);

  const calculateDuration = (cycle) => {
    if (cycleType === 'led') {
      return (cycle.durationOn + cycle.durationOff) * cycle.ledRepetitions;
    }
    else if (cycleType === 'water') {
      return (cycle.duration1 + cycle.duration2) * cycle.waterRepetitions;
    }
    else if (cycleType === 'temperature') {
      return cycle.duration1;
    }
  }

  const toggleCollapse = () => {
    setIsCollapsed(!isCollapsed);
  };






  return (
    <Card className="my-3">
      <Card.Header>
        <strong>{cycleType.toUpperCase()} Cycle {cycleIndex + 1}</strong>

        {showDuration && (
          <span>
            Dauer dieses Zyklus: {calculateDuration(cycle)} Minuten
          </span>
        )}

      <Button onClick={toggleCollapse} className="float-end">
        {isCollapsed ? "Expand" : "Collapse"}
      </Button>

      </Card.Header>
      <Card.Body style={{ display: isCollapsed ? "none" : "block" }}>
        {showDuration && (
          <p>
            Dauer dieses Zyklus: {calculateDuration(cycle)}{' '}
            {cycleType === 'led' ? 'Minuten' : ''}
          </p>
        )}
        {Object.entries(cycle).map(([name, value], index) => {
          // Skip rendering temperature2 and duration2 if cycleType is temperature
          if (cycleType === 'temperature' && (name === 'temperature2' || name === 'duration2')) {
            return null;
          }

          return (
            <Form.Group as={Row} key={name}>
              <Form.Label column sm={4}>
                {name}
              </Form.Label>
              <Col sm={6}>
                {cycleType === 'water' && (name === 'status1' || name === 'status2') ? (
                  <Form.Select
                    name={name}
                    value={value}
                    onChange={(e) => handleChange(name, e.target.value)}
                  >
                    <option value="full">Voll</option>
                    <option value="empty">Leer</option>
                  </Form.Select>
                ) : (
                  <InputGroup>
                    <Form.Control
                      type="number"
                      value={value}
                      onChange={(e) => handleChange(name, parseInt(e.target.value))}
                    />
                    {cycleType === 'led' && (name === 'durationOn' || name === 'durationOff') && (
                      <InputGroup.Text>Minuten</InputGroup.Text>
                    )}
                  </InputGroup>
                )}
              </Col>
            </Form.Group>
          );
        })}
        <Button variant="danger" onClick={() => onDelete(cycleIndex)}>
          Delete {cycleType} Cycle
        </Button>
      </Card.Body>
    </Card>
  );
};

export default CycleInput;
