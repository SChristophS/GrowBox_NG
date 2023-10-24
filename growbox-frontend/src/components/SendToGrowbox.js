import React, { useContext, useState, useEffect } from "react";
import { Container, Table, Row, Col, Button } from "react-bootstrap";
import { AuthContext } from "../contexts/AuthContext";
import { SettingsContext } from "../contexts/SettingsContext";

const SendToGrowbox = () => {
    const { username } = useContext(AuthContext);
    const [growPlans, setGrowPlans] = useState([]);
    const [statusMessage, setStatusMessage] = useState("");
    const [loadStatus, setLoadStatus] = useState("");

    const {
        updateLedSettings,
        setTemperatureSettings,
        setWateringSettings,
        setGrowCycleName,
        setDescription,
        setSharingStatus,
        setTotalGrowTime
    } = useContext(SettingsContext);

    const sendToGrowbox = (plan) => {
        // Hier können Sie den Code zum Senden des Growplans an die Growbox hinzufügen
    };

    const getGrowPlans = () => {
        fetch(`http://localhost:5000/get-grow-plans/${username}`, {
            method: "GET",
            credentials: "include",
        })
        .then((response) => response.json())
        .then((data) => {
            if (data.status === "success") {
                setGrowPlans(data.data);
                setStatusMessage("Grow-Pläne erfolgreich geladen.");
            } else {
                setStatusMessage(`Fehler beim Laden der Grow-Pläne: ${data.message}`);
            }
        })
        .catch((error) => {
            console.error("Error:", error);
            setStatusMessage("Fehler beim Laden der Grow-Pläne.");
        });
    };

    useEffect(() => {
        getGrowPlans();
    }, []);

    return (
        <Container>
            <h2>An Growbox senden</h2>
            <Row>
                <Col>
                    <p>{statusMessage}</p>
                    <p>{loadStatus}</p>
                </Col>
            </Row>
            <Table striped bordered hover size="sm">
                <thead>
                    <tr>
                        <th>Benutzername</th>
                        <th>Growname</th>
                        <th>Beschreibung</th>
                        <th>Sharing Status</th>
                        <th></th>
                    </tr>
                </thead>
                <tbody>
                    {(growPlans || []).map((plan) => (
                        <tr key={`${plan.username}-${plan.growCycleName}`}>
                            <td>{plan.username}</td>
                            <td>{plan.growCycleName}</td>
                            <td>{plan.description}</td>
                            <td>{plan.sharingStatus ? "Geteilt" : "Privat"}</td>
                            <td>
                                <Button variant="primary" size="sm" onClick={() => sendToGrowbox(plan)}>An Growbox Senden</Button>
                            </td>
                        </tr>
                    ))}
                </tbody>
            </Table>
        </Container>
    );
};

export default SendToGrowbox;
