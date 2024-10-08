import React, { useMemo } from 'react';

const RingComponent = ({ growData }) => {
  const { totalGrowTime, ledCycles, wateringCycles, tempCycles } = growData;

  const radius1 = 100;
  const radius2 = 120;
  const radius3 = 140;
  const radius4 = 160;

  const strokeWidth_time = 10;
  const strokeWidth_led = 10;
  const strokeWidth_water = 10;
  const strokeWidth_temperature = 10;

  const totalGrowTimeInHours = useMemo(() => totalGrowTime / 60, [totalGrowTime]);

  const GrowDuration = useMemo(() => {
    if (totalGrowTimeInHours === 1) {
      return <text x="50%" y="50%" textAnchor="middle" dy=".3em" fontSize="14">{totalGrowTimeInHours} Stunde</text>;
    } else {
      return <text x="50%" y="50%" textAnchor="middle" dy=".3em" fontSize="14">{totalGrowTimeInHours} Stunden</text>;
    }
  }, [totalGrowTimeInHours]);

  const waterSegments = useMemo(() => renderWaterSegments(wateringCycles, totalGrowTime, radius3, strokeWidth_water), [wateringCycles, totalGrowTime]);
  const ledSegments = useMemo(() => renderLedSegments(ledCycles, totalGrowTime, radius2, strokeWidth_led), [ledCycles, totalGrowTime]);
  const tempSegments = useMemo(() => renderTempSegments(tempCycles, totalGrowTime, radius4, strokeWidth_temperature), [tempCycles, totalGrowTime]);

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

function renderWaterSegments(wateringCycles, totalGrowTime, radius, strokeWidth) {
  const segments = [];
  let cumulativeWaterDuration = 0;
  for (let i = 0; i < wateringCycles.length; i++) {
    const { status1, duration1, status2, duration2, waterRepetitions } = wateringCycles[i];

    for (let j = 0; j < waterRepetitions; j++) {
      const startAngleFull = (360 / totalGrowTime) * cumulativeWaterDuration;
      const endAngleFull = startAngleFull + (360 / totalGrowTime) * duration1;
      const startAngleEmpty = endAngleFull;
      const endAngleEmpty = startAngleEmpty + (360 / totalGrowTime) * duration2;

      segments.push(
        <path
          key={`water-full-${i}-${j}`}
          d={describeArc(200, 200, radius, startAngleFull, endAngleFull)}
          fill="none"
          stroke="darkblue"
          strokeWidth={strokeWidth}
        />,
        <path
          key={`water-empty-${i}-${j}`}
          d={describeArc(200, 200, radius, startAngleEmpty, endAngleEmpty)}
          fill="none"
          stroke="lightblue"
          strokeWidth={strokeWidth}
        />
      );

      cumulativeWaterDuration += duration1 + duration2;
    }
  }
  return segments;
}

function renderLedSegments(ledCycles, totalGrowTime, radius, strokeWidth) {
  const segments = [];
  let cumulativeLedDuration = 0;
  for (let j = 0; j < ledCycles.length; j++) {
    const { durationOn, durationOff, ledRepetitions } = ledCycles[j];

    for (let i = 0; i < ledRepetitions; i++) {
      const startAngleOn = (360 / totalGrowTime) * (cumulativeLedDuration + i * (durationOn + durationOff));
      const endAngleOn = startAngleOn + (360 / totalGrowTime) * durationOn;
      const startAngleOff = endAngleOn;
      const endAngleOff = startAngleOff + (360 / totalGrowTime) * durationOff;

      segments.push(
        <path
          key={`led-on-${j}-${i}`}
          d={describeArc(200, 200, radius, startAngleOn, endAngleOn)}
          fill="none"
          stroke="purple"
          strokeWidth={strokeWidth}
        />,
        <path
          key={`led-off-${j}-${i}`}
          d={describeArc(200, 200, radius, startAngleOff, endAngleOff)}
          fill="none"
          stroke="gray"
          strokeWidth={strokeWidth}
        />
      );
    }
    cumulativeLedDuration += ledRepetitions * (durationOn + durationOff);
  }
  return segments;
}

function renderTempSegments(tempCycles, totalGrowTime, radius, strokeWidth) {
  const segments = [];
  let cumulativeDuration = 0;
  for (let i = 0; i < tempCycles.length; i++) {
    const { temperature, duration } = tempCycles[i];
    const startAngle = (360 / totalGrowTime) * cumulativeDuration;
    const endAngle = startAngle + (360 / totalGrowTime) * duration;
    cumulativeDuration += duration;

    segments.push(
      <path
        key={`temp-${i}`}
        d={describeArc(200, 200, radius, startAngle, endAngle)}
        fill="none"
        stroke={temperatureColor(temperature)}
        strokeWidth={strokeWidth}
      />
    );
  }
  return segments;
}

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
  const d = ['M', start.x, start.y, 'A', radius, radius, 0, arcSweep, 0, end.x, end.y].join(' ');
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
