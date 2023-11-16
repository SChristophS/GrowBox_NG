import React, { useState } from 'react';
import NavigationBar from './components/navbar';
import Home from './components/home';
import AktuellerGrow from './components/currentGrow';
import Beleuchtung from './components/lightning';
import Bewaesserung from './components/watering';
import Temperatur from './components/temperature';
import General_zycle from './components/General_zycle'
import CreateGrowPlan from './components/CreateGrowPlan'
import Analyse from './components/analysis';
import Settings from './components/settings';
import SettingsContextProvider from './contexts/SettingsContext';
import { AuthProvider, useAuth } from './contexts/AuthContext';
import Load from './components/Load';
import Save from './components/Save';
import New from './components/new';
import Login from './components/Login';
import LoadCyclePlan from './components/LoadCyclePlan';
import LoadGrowPlan from './components/LoadGrowPlan';
import SendToGrowbox from './components/SendToGrowbox';
import GrowplanAnalyse from './components/GrowplanAnalyse';
import { BrowserRouter as Router, Route, Routes, Navigate } from 'react-router-dom';
import './App.css';

const App = () => {
  const [isGrowPlanLoaded, setIsGrowPlanLoaded] = useState(false);

  return (
    <SettingsContextProvider>
      <AuthProvider>
        <Router>
          <NavigationBar 
            isGrowPlanLoaded={isGrowPlanLoaded} 
            setIsGrowPlanLoaded={setIsGrowPlanLoaded} 
          />
          <Routes>
            <Route path="/login" element={<Login />} />
            <Route path="/*" element={<ProtectedRoutes 
              isGrowPlanLoaded={isGrowPlanLoaded} 
              setIsGrowPlanLoaded={setIsGrowPlanLoaded} 
            />} />
          </Routes>
        </Router>
      </AuthProvider>
    </SettingsContextProvider>
  );
};

const ProtectedRoutes = ({ isGrowPlanLoaded, setIsGrowPlanLoaded }) => {
  const { isLoggedIn } = useAuth();

  if (!isLoggedIn) {
    return <Navigate to="/login" replace />;
  }

  return (
    <Routes>
      <Route path="/home" element={<Home />} />
      <Route path="/aktueller-grow" element={<AktuellerGrow />} />
      <Route path="/growplaner/General_zycle" element={<General_zycle />} />
	  <Route path="/growplaner/CreateGrowPlan" element={<CreateGrowPlan />} />
      <Route path="/growplaner/beleuchtung" element={<Beleuchtung />} />
      <Route path="/growplaner/bewaesserung" element={<Bewaesserung />} />
      <Route path="/growplaner/temperatur" element={<Temperatur />} />
      
	  
	  
	  <Route path="/growplaner/LoadCyclePlan" element={<LoadCyclePlan 
        isGrowPlanLoaded={isGrowPlanLoaded} 
        setIsGrowPlanLoaded={setIsGrowPlanLoaded} 
      />} />
	  <Route path="/growplaner/LoadGrowPlan" element={<LoadGrowPlan />} />
	  <Route path="/growplaner/Load" element={<Load />} />
	  
	  <Route path="/growplaner/new" element={<New />} />
      <Route path="/growplaner/save" element={<Save />} />
      <Route path="/growplaner/GrowplanAnalyse" element={<GrowplanAnalyse />} />
      <Route path="/SendToGrowbox" element={<SendToGrowbox />} />
      <Route path="/analyse" element={<Analyse />} />
      <Route path="/settings" element={<Settings />} />
    </Routes>
  );
};

export default App;