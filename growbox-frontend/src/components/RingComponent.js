import React from 'react';

const RingComponent = ({ growData }) => {
  const { totalGrowTime, ledCycles, wateringCycles, tempCycles } = growData;

  


  const radius1 = 100;
  const radius2 = 120;
  const radius3 = 140;
  const radius4 = 160;

  const strokeWidth_time = 10; // Dicke des ersten Kreises
  const strokeWidth_led = 10; // Dicke des zweiten Kreises
  const strokeWidth_water = 10; // Dicke des dritten Kreises
  const strokeWidth_temperature = 10; // Dicke des vierten Kreises

  const ledSegments = [];
  const waterSegments = [];
  const tempSegments = [];

  const GrowDuration = [];
  const totalGrowTimeInHours = totalGrowTime/60;

  if (totalGrowTimeInHours == 1){
    GrowDuration.push(
      <text x="50%" y="50%" textAnchor="middle" dy=".3em" fontSize="14">{totalGrowTimeInHours} Stunde</text>
    );
  } else {
    GrowDuration.push(
      <text x="50%" y="50%" textAnchor="middle" dy=".3em" fontSize="14">{totalGrowTimeInHours} Stunden</text>
    );
  }

  



  


// Berechnung der Wasserzyklen
let cumulativeWaterDuration = 0;
for (let i = 0; i < wateringCycles.length; i++) {
  const { status1, duration1, status2, duration2, waterRepetitions } = wateringCycles[i];

  for (let j = 0; j < waterRepetitions; j++) {
    const startAngleFull = (360 / totalGrowTime) * cumulativeWaterDuration;
    const endAngleFull = startAngleFull + (360 / totalGrowTime) * duration1;
    const startAngleEmpty = endAngleFull;
    const endAngleEmpty = startAngleEmpty + (360 / totalGrowTime) * duration2;

    waterSegments.push(
      <path
        key={`water-full-${i}-${j}`}
        d={describeArc(200, 200, radius3, startAngleFull, endAngleFull)}
        fill="none"
        stroke="darkblue"
        strokeWidth={strokeWidth_water}
      />,
      <path
        key={`water-empty-${i}-${j}`}
        d={describeArc(200, 200, radius3, startAngleEmpty, endAngleEmpty)}
        fill="none"
        stroke="lightblue"
        strokeWidth={strokeWidth_water}
      />,
    );

    cumulativeWaterDuration += duration1 + duration2;
  }
}


// Fügen Sie diese Zeile vor der for-Schleife für ledCycles ein
let cumulativeLedDuration = 0;

for (let j = 0; j < ledCycles.length; j++) {
  const { durationOn, durationOff, ledRepetitions } = ledCycles[j];

  for (let i = 0; i < ledRepetitions; i++) {
    const startAngleOn = (360 / totalGrowTime) * (cumulativeLedDuration + i * (durationOn + durationOff));
    const endAngleOn = startAngleOn + (360 / totalGrowTime) * durationOn;
    const startAngleOff = endAngleOn;
    const endAngleOff = startAngleOff + (360 / totalGrowTime) * durationOff;

    ledSegments.push(
      <path
        key={`led-on-${j}-${i}`}
        d={describeArc(200, 200, radius2, startAngleOn, endAngleOn)}
        fill="none"
        stroke="purple"
        strokeWidth={strokeWidth_led}
      />,
      <path
        key={`led-off-${j}-${i}`}
        d={describeArc(200, 200, radius2, startAngleOff, endAngleOff)}
        fill="none"
        stroke="gray"
        strokeWidth={strokeWidth_led}
      />,
    );
  }

  // Fügen Sie diese Zeile am Ende der for-Schleife für ledCycles hinzu
  cumulativeLedDuration += ledRepetitions * (durationOn + durationOff);
}


 // Berechnung der Temperaturzyklen
let cumulativeDuration = 0;
for (let i = 0; i < tempCycles.length; i++) {
  const { temperature1, duration1 } = tempCycles[i];
  const startAngle = (360 / totalGrowTime) * cumulativeDuration;
  const endAngle = startAngle + (360 / totalGrowTime) * duration1;
  cumulativeDuration += duration1;

  tempSegments.push(
    <path
      key={`temp-${i}`}
      d={describeArc(200, 200, radius4, startAngle, endAngle)}
      fill="none"
      stroke={temperatureColor(temperature1)}
      strokeWidth={strokeWidth_temperature}
    />
  );
}

  return (
    <svg width="400" height="400" viewBox="0 0 400 400">
      <circle
        cx="200"
        cy="200"
        r={radius1}
        fill="none"
        stroke="black"
        strokeWidth={strokeWidth_time}
        strokeLinecap="round"
      />
      {ledSegments}
      {waterSegments}
      {tempSegments}
      {GrowDuration} 
    </svg>
  );
};

// Helper function to create arc path
function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
  const angleInRadians = ((angleInDegrees - 90) * Math.PI) / 180.0;

  return {
    x: centerX + radius * Math.cos(angleInRadians),
    y: centerY + radius * Math.sin(angleInRadians),
  };
}

function describeArc(x, y, radius, startAngle, endAngle) {
  const start = polarToCartesian(x, y, radius, endAngle);
  const end = polarToCartesian(x, y, radius, startAngle);

  const arcSweep = endAngle - startAngle <= 180 ? '0' : '1';

  const d = [
    'M',
    start.x,
    start.y,
    'A',
    radius,
    radius,
    0,
    arcSweep,
    0,
    end.x,
    end.y,
  ].join(' ');

  return d;
}

function temperatureColor(temperature) {
  const minTemp = 0;
  const maxTemp = 50;
  const ratio = (temperature - minTemp) / (maxTemp - minTemp);
  const r = Math.round(255 * ratio);
  return `rgb(${r}, 0, 0)`;
}

export default RingComponent;
