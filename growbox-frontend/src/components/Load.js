import { Container, Row, Col, Button } from 'react-bootstrap';
import { useNavigate } from 'react-router-dom';


  
const Load = () => {
  
  let navigate = useNavigate();

  const handleLoadCycle = () => {
    navigate('/growplaner/LoadCyclePlan');
  };

  const handleLoadGrowPlan = () => {
    navigate('/growplaner/LoadGrowPlan');
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
          <Button onClick={handleLoadCycle} className="mx-2">
            GrowCycle laden
          </Button>
          <Button onClick={handleLoadGrowPlan} className="mx-2">
            Growplan laden
          </Button>
        </Col>
      </Row>
    </Container>
  );
};

export default Load;
