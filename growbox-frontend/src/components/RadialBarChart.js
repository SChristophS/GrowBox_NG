import React from 'react';
import { ResponsiveRadialBar } from '@nivo/radial-bar';

const RadialBarChart = ({ data }) => (
  <div style={{ height: '400px', width: '400px' }}>
    <ResponsiveRadialBar
      data={data}
      keys={['value']}
      indexBy="id"
      maxValue={100}
      margin={{ top: 40, right: 120, bottom: 40, left: 40 }}
      borderWidth={1}
      borderColor={{ from: 'color', modifiers: [['darker', 1.6]] }}
      enableLabel
      label={(d) => `${d.id}: ${d.value}`}
      labelTextColor={{ from: 'color', modifiers: [['darker', 1.6]] }}
      colors={(bar) => {
        if (bar.id === 'Gesamtdauer') return 'rgba(0, 150, 255, 0.7)';
        if (bar.id.startsWith('Hell')) return 'rgba(255, 215, 0, 0.7)';
        if (bar.id.startsWith('Dunkel')) return 'rgba(80, 80, 80, 0.7)';
        if (bar.id === 'Hell und Dunkel') return 'rgba(0, 255, 0, 0.7)';
        return 'rgba(0, 0, 0, 0.7)';
      }}
    />
  </div>
);

export default RadialBarChart;
