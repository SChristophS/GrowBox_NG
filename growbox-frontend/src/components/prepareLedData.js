const prepareLedData = () => {
  const growDuration = 100;
  const lightCycles = [
    { id: 'Hell 1', value: 20 },
    { id: 'Dunkel 1', value: 20 },
    { id: 'Hell 2', value: 20 },
    { id: 'Dunkel 2', value: 20 },
    { id: 'Hell 3', value: 20 },
  ];

  // Berechne den durchschnittlichen Wert für "Hell" und "Dunkel"
  const lightCycleValues = lightCycles.map((cycle) => cycle.value);
  const avgLightCycleValue = lightCycleValues.reduce((acc, value) => acc + value, 0) / lightCycleValues.length;

  return [
    {
      id: 'Gesamtdauer',
      data: [
        {
          id: 'Gesamtdauer',
          value: growDuration,
        },
      ],
    },
    ...lightCycles.map((cycle, index) => ({
      id: cycle.id,
      data: [
        {
          id: cycle.id,
          value: (cycle.value / growDuration) * 100,
        },
      ],
    })),
    // Füge eine zusätzliche Datenreihe für "Hell und Dunkel" hinzu
    {
      id: 'Hell und Dunkel',
      data: [
        {
          id: 'Hell und Dunkel',
          value: (avgLightCycleValue / growDuration) * 100,
        },
      ],
    },
  ];
};

export default prepareLedData;
