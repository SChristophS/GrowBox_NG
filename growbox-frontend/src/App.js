import React, { useState } from 'react';
import NavigationBar from './components/navbar';
import Home from './components/home';
import AktuellerGrow from './components/currentGrow';
import Beleuchtung from './components/lightning';
import Bewaesserung from './components/watering';
import Temperatur from './components/temperature';
import General from './components/General'
import Analyse from './components/analysis';
import Settings from './components/settings';
import SettingsContextProvider from './contexts/SettingsContext';
import { AuthProvider, useAuth } from './contexts/AuthContext';
import Load from './components/Load';
import Save from './components/Save';
import Login from './components/Login';
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
      <Route path="/growplaner/general" element={<General />} />
      <Route path="/growplaner/beleuchtung" element={<Beleuchtung />} />
      <Route path="/growplaner/bewaesserung" element={<Bewaesserung />} />
      <Route path="/growplaner/temperatur" element={<Temperatur />} />
      <Route path="/growplaner/load" element={<Load 
        isGrowPlanLoaded={isGrowPlanLoaded} 
        setIsGrowPlanLoaded={setIsGrowPlanLoaded} 
      />} />
      <Route path="/growplaner/save" element={<Save />} />
      <Route path="/growplaner/GrowplanAnalyse" element={<GrowplanAnalyse />} />
      <Route path="/SendToGrowbox" element={<SendToGrowbox />} />
      <Route path="/analyse" element={<Analyse />} />
      <Route path="/settings" element={<Settings />} />
    </Routes>
  );
};

export default App;
