import React, { useState } from 'react';
import NavigationBar from './components/navbar';
import Home from './components/home';
import AktuellerGrow from './components/currentGrow';
import CreateGrowPlan from './components/CreateGrowPlan'
import Settings from './components/settings';
import SettingsContextProvider from './contexts/SettingsContext';
import { GrowPlanProvider } from './contexts/GrowPlanContext';
import { GrowCycleProvider } from './contexts/GrowCycleContext';

import { AuthProvider, useAuth } from './contexts/AuthContext';
import Load from './components/Load';
import New from './components/new';
import Login from './components/Login';
import LoadCyclePlan from './components/LoadCyclePlan';
import LoadGrowPlan from './components/LoadGrowPlan';
import SendToGrowbox from './components/SendToGrowbox';
import GrowplanAnalyse from './components/GrowplanAnalyse';
import { BrowserRouter as Router, Route, Routes, Navigate } from 'react-router-dom';
import './App.css';

// 
import GrowCycleOverview from './components/GrowCycleOverview';
import Cycle_General from './components/Cycle_General';
import Cycle_Light from './components/Cycle_Light';
import Cycle_Temperature from './components/Cycle_Temperature';
import Cycle_Water from './components/Cycle_Water';
import Cycle_Analyse from './components/Cycle_Analyse';
import Cycle_Save from './components/Cycle_Save';




const App = () => {
  const [isGrowPlanLoaded, setIsGrowPlanLoaded] = useState(false);

  return (
    <SettingsContextProvider>
      <AuthProvider>
	   <GrowPlanProvider>
	   <GrowCycleProvider>
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
		</GrowCycleProvider>
	  </GrowPlanProvider>
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
      <Route path="/growplaner/Cycle_General" element={<Cycle_General />} />
	  <Route path="/growplaner/Cycle_Light" element={<Cycle_Light />} />
      <Route path="/growplaner/Cycle_Water" element={<Cycle_Water />} />
      <Route path="/growplaner/Cycle_Temperature" element={<Cycle_Temperature />} />	
      <Route path="/growplaner/Cycle_Analyse" element={<Cycle_Analyse />} />	  
	  <Route path="/growplaner/Cycle_Save" element={<Cycle_Save />} />	  
	  <Route path="/growplaner/CreateGrowPlan" element={<CreateGrowPlan />} />
	  <Route path="/growplaner/GrowCycleOverview" element={<GrowCycleOverview />} />
	  <Route path="/growplaner/LoadCyclePlan" element={<LoadCyclePlan 
        isGrowPlanLoaded={isGrowPlanLoaded} 
        setIsGrowPlanLoaded={setIsGrowPlanLoaded} 
      />} />
	  <Route path="/growplaner/LoadGrowPlan" element={<LoadGrowPlan />} />
	  <Route path="/growplaner/Load" element={<Load />} />
	  <Route path="/growplaner/new" element={<New />} />
      <Route path="/SendToGrowbox" element={<SendToGrowbox />} />
      <Route path="/settings" element={<Settings />} />
	  
	  


    </Routes>
  );
};

export default App;