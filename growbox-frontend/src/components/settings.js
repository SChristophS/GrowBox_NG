import React, { useContext } from 'react';
import { SettingsContext } from '../contexts/SettingsContext';
import Button from 'react-bootstrap/Button';

const Settings = () => {
  const { saveSettings } = useContext(SettingsContext);

  const handleSaveClick = () => {
    saveSettings();
  };

  return (
    <div>
      {/* Ihre bisherigen Einstellungskomponenten */}
      <Button onClick={handleSaveClick}>Einstellungen speichern</Button>
    </div>
  );
};

export default Settings;
