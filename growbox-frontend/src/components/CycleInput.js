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
      return cycle.duration;
    }
  }

  const toggleCollapse = () => {
    setIsCollapsed(!isCollapsed);
  };






  return (
    <Card className="my-3">
    <Card.Header>
  <Row className="align-items-center">
    <Col xs={5}>
      <strong>{cycleType.toUpperCase()} Cycle {cycleIndex + 1}</strong>
    </Col>
    <Col xs={4} className="text-end">
      {showDuration && (
        <p className="duration-display mb-0">
          Dauer dieses Zyklus: {calculateDuration(cycle)} Minuten
        </p>
      )}
    </Col>
    <Col xs={3} className="text-end">
      <Button 
        onClick={toggleCollapse} 
        className="me-2">
        {isCollapsed ? "Expand" : "Collapse"}
      </Button>
      <Button
        variant="danger"
        size="sm"
        onClick={() => onDelete(cycleIndex)}
      >
        Delete {cycleType} Cycle
      </Button>
    </Col>
  </Row>
</Card.Header>

      <Card.Body style={{ display: isCollapsed ? "none" : "block" }}>

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
      </Card.Body>
    </Card>
  );
};

export default CycleInput;
