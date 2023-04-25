import React from 'react';
import { Navbar, Nav, NavDropdown } from 'react-bootstrap';
import { NavLink } from 'react-router-dom';

const NavigationBar = () => {
  return (
    <Navbar bg="light" expand="lg">
      <NavLink to="/" className="navbar-brand">
        GrowBox
      </NavLink>
      <Navbar.Toggle aria-controls="basic-navbar-nav" />
      <Navbar.Collapse id="basic-navbar-nav">
        <Nav className="mr-auto">
          <NavLink to="/aktueller-grow" className="nav-link">
            Aktueller Grow
          </NavLink>
          <NavDropdown title="Growplaner" id="growplaner-dropdown">
            <NavLink to="/growplaner/beleuchtung" className="dropdown-item">
              Beleuchtung
            </NavLink>
            <NavLink to="/growplaner/bewaesserung" className="dropdown-item">
              Bewässerung
            </NavLink>
            <NavLink to="/growplaner/temperatur" className="dropdown-item">
              Temperatur
            </NavLink>
            <NavLink to="/growplaner/load" className="dropdown-item">
              Laden
            </NavLink>
            <NavLink to="/growplaner/save" className="dropdown-item">
              Speichern
            </NavLink>
          </NavDropdown>
          <NavLink to="/analyse" className="nav-link">
            Analyse
          </NavLink>
          <NavLink to="/settings" className="nav-link">
            Einstellungen
          </NavLink>
        </Nav>
      </Navbar.Collapse>
    </Navbar>
  );
};

export default NavigationBar;
