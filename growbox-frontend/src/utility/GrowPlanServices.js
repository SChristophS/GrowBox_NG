const GrowPlanService = {
	
	// Funktion zum Abrufen der totalGrowTime für einen Zyklus
async getCycleTotalTime(id) {
	console.log("Aufruf getCycleTotalTime mit ID:", id);
  try {
    const response = await fetch(`http://localhost:5000/get-cycle-totaltime/${id}`, {
      method: "GET",
      credentials: "include",
    });

    if (response.ok) {
		console.log("response:", response);
      const data = await response.json();
      return { success: true, totalGrowTime: data.totalGrowTime };
    } else {
      return { success: false, totalGrowTime: 0 };
    }
  } catch (error) {
    return { success: false, totalGrowTime: 0 };
  }
},
	
async getGrowPlans(username) {
	  console.log("getGrowPlans called with username:" + username);
    try {
      const response = await fetch(`http://localhost:5000/get-grow-plans/${username}`, {
        method: "GET",
        credentials: "include",
      });
	  
      if (response.ok) {
        const data = await response.json();
		console.log("data:", data);
        return { success: true, data: data.data, message: "Grow-Pläne erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden der Grow-Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden der Grow-Pläne: ${error.toString()}` };
    }
  },
  
 async checkGrowPlanExists (username, growPlanName) {
	  console.log("getGrowPlans called with username:" + username);
	   console.log("getGrowPlans called with growPlanName:" + growPlanName);
  try {
    const response = await fetch("http://localhost:5000/check-grow-plan-exists", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ username, growPlanName }),
    });
    const data = await response.json();
	console.log("Hier Return Data von Backend");
	console.log(data)
	
    return data.exists;
  } catch (error) {
    console.error("Fehler beim Überprüfen des Growplans:", error);
    return false; // Bei Fehler, annehmen dass der Plan nicht existiert
  }
},

async transmitGrowPlanToTarget(growplan_with_targetID) {
	console.log("GrowPlanServices: transmitGrowPlanToTarget called");
	console.log("growplan_with_targetID: ", growplan_with_targetID);
  

  try {
    const response = await fetch("http://localhost:5000/transmit-grow-plan-to-target", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      credentials: "include",
      body: JSON.stringify(growplan_with_targetID),
    });

    const data = await response.json();
    console.log("Response from server:", data);
    
    return {
      success: response.ok,
      data: data,
      message: response.ok ? "Grow plan transmitted successfully." : data.message,
    };
  } catch (error) {
    console.error("Error:", error);
    return { success: false, message: error.toString() };
  }
},

async saveGrowPlan(growPlanData) {
  console.log("GrowPlanServices: saveGrowPlan called with username:", growPlanData.username);
  console.log("and growPlanData:", growPlanData);

  try {
    const response = await fetch("http://localhost:5000/save-grow-plan", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      credentials: "include",
      body: JSON.stringify(growPlanData),
    });

    const data = await response.json();
    console.log("Response from server:", data);
    
    return {
      success: response.ok,
      data: data,
      message: response.ok ? "Grow plan saved successfully." : data.message,
    };
  } catch (error) {
    console.error("Error:", error);
    return { success: false, message: error.toString() };
  }
},
  
  async getCyclePlans(username) {
	console.log("GrowPlanServices: getCyclePlans is called with username: " + username);
    try {
      const response = await fetch(`http://localhost:5000/get-cycle-plans/${username}`, {
        method: "GET",
        credentials: "include",
      });
      if (response.ok) {
        const data = await response.json();
        return { success: true, data: data.data, message: "Cycle-Pläne erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${error.toString()}` };
    }
  },
  
  async getGrowboxIDsFromUsername(username) {
	console.log("GrowPlanServices: getGrowboxIDsFromUsername is called with username: " + username);
    try {
      const response = await fetch(`http://localhost:5000/get-growboxIDs-from-username/${username}`, {
        method: "GET",
        credentials: "include",
      });
	  
	  //console.log(response);
      if (response.ok) {
		  console.log("GrowPlanServices.getGrowboxIDsFromUsername: Response ok");
        
			const data = await response.json();
			console.log(data);
        return { success: true, data: data.data, message: "Cycle-Plan erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${error.toString()}` };
    }
  },

  async getAllPlans(username) {
    try {
      const response = await fetch(`http://localhost:5000/get-all-plans/${username}`, {
        method: "GET",
        credentials: "include",
      });
      if (response.ok) {
        const data = await response.json();
        return { success: true, data: data.data, message: "Alle Pläne erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden aller Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden aller Pläne: ${error.toString()}` };
    }
  },

 
  async getCyclePlanFromID(id) {
	console.log("GrowPlanServices: getCyclePlans is called with id: " + id);
    try {
      const response = await fetch(`http://localhost:5000/get-cycle-plan-from-id/${id}`, {
        method: "GET",
        credentials: "include",
      });
	  //console.log(response);
      if (response.ok) {
        const data = await response.json();
		//console.log(data);
        return { success: true, data: data.data, message: "Cycle-Plan erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden der Cycle-Pläne: ${error.toString()}` };
    }
  },

  async getAllPlans(username) {
    try {
      const response = await fetch(`http://localhost:5000/get-all-plans/${username}`, {
        method: "GET",
        credentials: "include",
      });
      if (response.ok) {
        const data = await response.json();
        return { success: true, data: data.data, message: "Alle Pläne erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden aller Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden aller Pläne: ${error.toString()}` };
    }
  },
  
	async deleteGrowPlan(growPlanID) {
		console.log("deleteGrowPlan called with growPlanID: " + growPlanID);
   
	  try {
        const response = await fetch("http://localhost:5000/delete-grow-plan", {
          method: "DELETE",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({
            growPlanID
          }),
        });

        if (response.ok) {
          const data = await response.json();
          return { success: true, message: data.message };
        } else {
          const data = await response.json();
          return { success: false, message: data.message };
        }
      } catch (error) {
        return { success: false, message: error.toString() };
      }
    
    return { success: false, message: "Aktion abgebrochen." };
  },
  
   async deleteCyclePlan(growCycleID) {
	   		console.log("deleteCyclePlan called with growPlanID: " + growCycleID);

      try {
        const response = await fetch("http://localhost:5000/delete-cycle-plan", {
          method: "DELETE",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({
            growCycleID
          }),
        });

        if (response.ok) {
          const data = await response.json();
          return { success: true, message: data.message };
        } else {
          const data = await response.json();
          return { success: false, message: data.message };
        }
      } catch (error) {
        return { success: false, message: error.toString() };
      }
    return { success: false, message: "Aktion abgebrochen." };
  }
  

  
};

export default GrowPlanService;
