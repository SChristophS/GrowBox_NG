import React from "react";
import { Modal, Form, Button } from "react-bootstrap";
import PropTypes from "prop-types";

function GrowPlanModal({
    show,
    onHide,
    growPlans,
    selectedGrowPlan,
    setSelectedGrowPlan,
    fetchGrowPlans,
    loadingGrowPlans,
    error,
    onSendGrowPlan
}) {
    return (
        <Modal show={show} onHide={onHide} size="lg">
            <Modal.Header closeButton>
                <Modal.Title>Growpläne auswählen</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                <Form>
                    {growPlans.map((plan, index) => (
                        <Form.Check
                            type="radio"
                            label={plan.growPlanName}
                            name="growPlanGroup"
                            id={`growPlan-${index}`}
                            key={index}
                            onChange={() => setSelectedGrowPlan(plan)}
                        />
                    ))}
                </Form>
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={onHide}>Abbrechen</Button>
                <Button
                    variant="primary"
                    onClick={() => onSendGrowPlan(selectedGrowPlan)}
                    disabled={!selectedGrowPlan}
                >
                    An Growbox senden
                </Button>
            </Modal.Footer>
        </Modal>
    );
}

GrowPlanModal.propTypes = {
    show: PropTypes.bool.isRequired,
    onHide: PropTypes.func.isRequired,
    growPlans: PropTypes.array.isRequired,
    selectedGrowPlan: PropTypes.object,
    setSelectedGrowPlan: PropTypes.func.isRequired,
    fetchGrowPlans: PropTypes.func.isRequired,
    loadingGrowPlans: PropTypes.bool.isRequired,
    error: PropTypes.string,
    onSendGrowPlan: PropTypes.func.isRequired,
};

export default GrowPlanModal;
