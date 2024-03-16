import React, { useState } from 'react';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';

// Import components
import Cycle_General from './Cycle_General';
import Cycle_Light from './Cycle_Light';
import Cycle_Watering from './Cycle_Water';
import Cycle_Temperature from './Cycle_Temperature';
import Cycle_Analyse from './Cycle_Analyse';
import Cycle_Save from './Cycle_Save';

function GrowCycleOverview() {
  const [selectedTab, setSelectedTab] = useState(0);

  const handleChange = (event, newValue) => {
    setSelectedTab(newValue);
  };

  return (
    <>
      {selectedTab === 0 && <Cycle_General />}
      {selectedTab === 1 && <Cycle_Light />}
      {selectedTab === 2 && <Cycle_Watering />}
      {selectedTab === 3 && <Cycle_Temperature />}
      {selectedTab === 4 && <Cycle_Analyse />}
      {selectedTab === 5 && <Cycle_Save />}
    </>
  );
}

export default GrowCycleOverview;
