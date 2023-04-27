import React from 'react';
import RadialBarChart from './RadialBarChart';
import prepareLedData from './prepareLedData';

const GrowplanAnalyse = () => {
  const data = prepareLedData();

  return (
    <div>
      <h2>Grow Plan Analyse3</h2>
      <RadialBarChart data={data} />
    </div>
  );
};

export default GrowplanAnalyse;
