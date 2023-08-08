import React, { useState, useEffect } from "react";

function CurrentGrow() {
  const [devices, setDevices] = useState([]);

  const fetchDevices = async () => {
	const username = "christoph";  // replace with the actual username
	const response = await fetch(`http://localhost:5000/devices?username=${username}`);
	
	if (response.ok) {  // check if HTTP-status is 200-299
	  // get the response body (the method explained below)
	  const data = await response.json();
	  console.log(data);
	  setDevices(data.devices);
	} else {
	  console.log("HTTP-Error: " + response.status);
	}
  };

  useEffect(() => {
	fetchDevices();
  }, []);

  return (
    <div>
      <h1>My Devices</h1>
      <button onClick={fetchDevices}>Refresh</button>
      <ul>
        {devices.map(device => (
          <li key={device.device_id}>
            {device.device_id} - 
            <span style={{ color: device.status === "connected" ? "green" : "red" }}>
              {device.status}
            </span>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default CurrentGrow;
