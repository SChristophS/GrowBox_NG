import React from 'react';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import NavigationBar from './components/navbar';
import Home from './components/home';
import AktuellerGrow from './components/currentGrow';
import Beleuchtung from './components/lightning';
import Bewaesserung from './components/watering';
import Temperatur from './components/temperature';
import Analyse from './components/analysis';
import Settings from './components/settings';
import SettingsContextProvider from './contexts/SettingsContext';
import Load from './components/Load';
import Save from './components/Save';

const App = () => {
  return (
    <SettingsContextProvider>
      <Router>
        <NavigationBar />
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/aktueller-grow" element={<AktuellerGrow />} />
          <Route path="/growplaner/beleuchtung" element={<Beleuchtung />} />
          <Route path="/growplaner/bewaesserung" element={<Bewaesserung />} />
          <Route path="/growplaner/temperatur" element={<Temperatur />} />
          <Route path="/growplaner/load" component={Load} />
          <Route path="/growplaner/save" component={Save} />
          <Route path="/analyse" element={<Analyse />} />
          <Route path="/settings" element={<Settings />} />
        </Routes>
      </Router>
    </SettingsContextProvider>
  );
};

export default App;
