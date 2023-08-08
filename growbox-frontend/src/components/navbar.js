import React, { useState } from "react";
import { Navbar, Nav, NavDropdown } from "react-bootstrap";
import { Link, useNavigate } from "react-router-dom";
import { useAuth } from "../contexts/AuthContext";

const NavigationBar = () => {
  const { isLoggedIn, setIsLoggedIn, setAuthUsername } = useAuth();
  const [expanded, setExpanded] = useState(false);
  const navigate = useNavigate();

  const handleClose = () => {
    setExpanded(false);
  };

  const handleLogout = () => {
    setIsLoggedIn(false);
    setAuthUsername(null);
    localStorage.removeItem("user");
    navigate("/");
  };

  return (
    <Navbar bg="light" expand="lg" expanded={expanded}>
      <Link to="/" className="navbar-brand" onClick={handleClose}>
        GrowBox
      </Link>
      <Navbar.Toggle
        aria-controls="basic-navbar-nav"
        onClick={() => setExpanded(!expanded)}
      />
      <Navbar.Collapse id="basic-navbar-nav">
        <Nav className="mr-auto">
          {isLoggedIn && (
            <>
              <Link
                to="/aktueller-grow"
                className="nav-link"
                onClick={handleClose}
              >
                Aktueller Grow
              </Link>
              <NavDropdown title="Growplaner" id="growplaner-dropdown">
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/general"
                  onClick={handleClose}
                >
                  General
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/beleuchtung"
                  onClick={handleClose}
                >
                  Beleuchtung
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/bewaesserung"
                  onClick={handleClose}
                >
                  Bewässerung
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/temperatur"
                  onClick={handleClose}
                >
                  Temperatur
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/load"
                  onClick={handleClose}
                >
                  Laden
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/save"
                  onClick={handleClose}
                >
                  Speichern
                </NavDropdown.Item>
                <NavDropdown.Item
                  as={Link}
                  to="/growplaner/GrowplanAnalyse"
                  onClick={handleClose}
                >
                  Growplan Analyse
                </NavDropdown.Item>
				<NavDropdown.Item
                  as={Link}
                  to="/growplaner/SendToGrowbox"
                  onClick={handleClose}
                >
                  Send to Growbox
                </NavDropdown.Item>
              </NavDropdown>
              <Link to="/analyse" className="nav-link" onClick={handleClose}>
                Analyse
              </Link>
              <Link to="/settings" className="nav-link" onClick={handleClose}>
                Einstellungen
              </Link>
              <Nav.Link onClick={handleLogout}>Logout</Nav.Link>
            </>
          )}
        </Nav>
      </Navbar.Collapse>
    </Navbar>
  );
};

export default NavigationBar;
