import React from 'react';
import { Container, Row, Col, Button } from 'react-bootstrap';
import homeLogo from '../img/home_logo.jpg'; // Importieren Sie das Bild

const HomePage = () => {
  return (
    <Container>
      <Row className="justify-content-center">
        <Col md={8} className="text-center">
          <h1>Willkommen bei SmartGrow</h1>
          <p>Entdecken Sie die Zukunft des Pflanzenanbaus mit unserer hochmodernen, smarten Anbautechnologie.</p>
          <img
            src={homeLogo} // Verwenden Sie das importierte Bild
            alt="SmartGrow Anlage"
            className="img-fluid"
          />
          <div className="mt-4">
            <Button variant="primary">Erkunde mehr</Button>
          </div>
        </Col>
      </Row>
    </Container>
  );
};

export default HomePage;
