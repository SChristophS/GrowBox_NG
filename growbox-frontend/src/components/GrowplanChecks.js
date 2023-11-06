import React from 'react';

export const calculateChecks = (growData) => {
    const { totalGrowTime, ledCycles, wateringCycles, tempCycles } = growData;
	
	console.log("TotalGrowTime = " + totalGrowTime);
	console.log("ledCycles = " + ledCycles);
	console.log("wateringCycles = " + wateringCycles);

    // Überprüfung, ob die benötigten Daten vorhanden sind
    if (!ledCycles || !wateringCycles || !tempCycles) {
        console.error("Einige der benötigten Daten sind nicht verfügbar!");
        return {};
    }

    const totalLightOnDuration = ledCycles.reduce((acc, cycle) => acc + cycle.durationOn * cycle.ledRepetitions, 0);
    const totalWateringFullDuration = wateringCycles.reduce((acc, cycle) => {
        const fullDuration = cycle.status1 === 'full' ? cycle.duration1 * cycle.waterRepetitions : cycle.duration2 * cycle.waterRepetitions;
        return acc + fullDuration;
    }, 0);
    const totalWateringDefinedDuration = wateringCycles.reduce((acc, cycle) => {
        const fullDuration = cycle.duration1 * cycle.waterRepetitions;
        const emptyDuration = cycle.duration2 * cycle.waterRepetitions;
        return acc + fullDuration + emptyDuration;
    }, 0);
    const testWateringDuration = totalGrowTime === totalWateringDefinedDuration;
    const testLightDuration = totalGrowTime === (totalLightOnDuration + ledCycles.reduce((acc, cycle) => acc + cycle.durationOff * cycle.ledRepetitions, 0));
    const testTemperatureDuration = totalGrowTime === tempCycles.reduce((acc, cycle) => acc + cycle.duration, 0);

    const minTemperature = Math.min(...tempCycles.map(cycle => cycle.temperature));
    const maxTemperature = Math.max(...tempCycles.map(cycle => cycle.temperature));

console.log("totalGrowTime:", totalGrowTime);
console.log("totalLightOnDuration:", totalLightOnDuration);
console.log("LED off duration:", ledCycles.reduce((acc, cycle) => acc + cycle.durationOff * cycle.ledRepetitions, 0));
console.log("totalWateringDefinedDuration:", totalWateringDefinedDuration);
console.log("Temperature duration:", tempCycles.reduce((acc, cycle) => acc + cycle.duration, 0));


    return {
        totalLightOnDuration,
        totalWateringFullDuration,
        testLightDuration,
        testWateringDuration,
        testTemperatureDuration,
        minTemperature,
        maxTemperature
    };
};

const GrowplanChecks = ({ growData }) => {
    const checks = calculateChecks(growData);

    return (
        <>
            <p>
                Test LED-Dauer: <span style={{ color: checks.testLightDuration ? 'green' : 'red' }}>{checks.testLightDuration ? 'passed' : 'failed'}</span>
            </p>
            <p>
                Test Bewässerungsdauer: <span style={{ color: checks.testWateringDuration ? 'green' : 'red' }}>{checks.testWateringDuration ? 'passed' : 'failed'}</span>
            </p>
            <p>
                Test Temperaturdauer: <span style={{ color: checks.testTemperatureDuration ? 'green' : 'red' }}>{checks.testTemperatureDuration ? 'passed' : 'failed'}</span>
            </p>
        </>
    );
};

export default GrowplanChecks;
