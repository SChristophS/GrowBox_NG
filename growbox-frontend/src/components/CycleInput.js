import React from 'react';
import { Card, Button, Col, Form, Row, InputGroup } from 'react-bootstrap';

const CycleInput = ({ cycleType, cycleIndex, cycle, onChange, onDelete }) => {
  const handleChange = (name, value) => {
    onChange(cycleIndex, cycleType, name, value);
  };

  return (
    <Card className="my-3">
      <Card.Header>
        <strong>{cycleType.toUpperCase()} Cycle {cycleIndex + 1}</strong>
      </Card.Header>
      <Card.Body>
        {Object.entries(cycle).map(([name, value], index) => (
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
        ))}
        <Button variant="danger" onClick={() => onDelete(cycleIndex)}>
          Delete {cycleType} Cycle
        </Button>
      </Card.Body>
    </Card>
  );
};

export default CycleInput;
