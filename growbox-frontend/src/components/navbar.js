import React, { useState, useContext } from "react";
import { Navbar, Nav, NavDropdown } from "react-bootstrap";
import { Link, useNavigate } from "react-router-dom";
import { useAuth } from "../contexts/AuthContext";
import { SettingsContext } from '../contexts/SettingsContext';
import { FaLeaf } from 'react-icons/fa'; // Beispiel für ein Blatt-Icon
import { GrowCycleContext } from '../contexts/GrowCycleContext';


const NavigationBar = () => {
  const { isLoggedIn, setIsLoggedIn, setAuthUsername } = useAuth();
  const [expanded, setExpanded] = useState(false);
  
  // Zugriff auf die benötigten Werte aus dem SettingsContext
  const { 
    growCycleName, 
    isGrowPlanLoaded, 
    unloadSettings,
  } = useContext(SettingsContext);
  
   // Zugriff auf die benötigten Werte aus der GrowCycleContext
	const { 
		isCyclePlanLoaded, 
		setIsCyclePlanLoaded,
		unloadCycleSettings
	} = useContext(GrowCycleContext);  
  
   console.log("navbar.js: Folgende Werte aus der SettingsContext geladen:");
   console.log("growCycleName: ", growCycleName);
   console.log("isGrowPlanLoaded: ", isGrowPlanLoaded);
   console.log("unloadSettings: ", unloadSettings);


	const handleUnload = () => {
		console.log("handleUnload gestartet");
		  if(window.confirm("Aktuell geladene Werte entladen?")) {
			unloadSettings();
			console.log("unloadSettings wurde aufgerufen");
			navigate('/home'); // Navigieren Sie zur Start
		  }
	};
  
	const handleCycleUnload = () => {
		console.log("handleCycleUnload gestartet");
		
		if(window.confirm("Aktuell geladenen Cycle-Plan entladen?")) {
			unloadCycleSettings();
			console.log("unloadCycleSettings wurde aufgerufen");
			navigate('/home'); // Navigieren Sie zur Start
		}
	};

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
  <>
    
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
						{
								<>
							<NavDropdown.Item
								as={Link}
								to="/growplaner/new"
								onClick={handleClose}
							>
								Neu
							</NavDropdown.Item>
							
							<NavDropdown.Item
								as={Link}
								to="/growplaner/load"
								onClick={handleClose}
							>
								Laden
							</NavDropdown.Item>
						  </>
						}
					</NavDropdown>
		  
					<Link to="/SendToGrowbox" className="nav-link" onClick={handleClose}>
						Send to Growbox
					</Link>
		
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

	{isCyclePlanLoaded && (
		<Navbar bg="secondary" variant="dark" expand="lg">
			<Nav className="mr-auto">
				<Nav.Link as={Link} to="/growplaner/Cycle_General" onClick={handleClose}>
					Allgemein
				</Nav.Link>
		  
				<Nav.Link as={Link} to="/growplaner/Cycle_Light" onClick={handleClose}>
					Beleuchtung
				</Nav.Link>
				
				<Nav.Link as={Link} to="/growplaner/Cycle_Water" onClick={handleClose}>
					Bewässerung
				</Nav.Link>
				
				<Nav.Link as={Link} to="/growplaner/Cycle_Temperature" onClick={handleClose}>
					Temperatur
				</Nav.Link>
				
				<Nav.Link as={Link} to="/growplaner/Cycle_Save" onClick={handleClose}>
					Speichern
				</Nav.Link>

				<Nav.Link as={Link} to="/growplaner/Cycle_Analyse" onClick={handleClose}>
					Growplan Analyse
				</Nav.Link>

				<Nav.Link onClick={handleCycleUnload}>
					Unload
				</Nav.Link>					
			</Nav>
		</Navbar>
	)}



</>
);

  
  
  
  
};

export default NavigationBar;
