const GrowPlanService = {
  async getGrowPlans(username) {
	  console.log("getGrowPlans called with username:" + username);
    try {
      const response = await fetch(`http://localhost:5000/get-grow-plans/${username}`, {
        method: "GET",
        credentials: "include",
      });
      if (response.ok) {
        const data = await response.json();
        return { success: true, data: data.data, message: "Grow-Pläne erfolgreich geladen." };
      } else {
        const data = await response.json();
        return { success: false, message: `Fehler beim Laden der Grow-Pläne: ${data.message}` };
      }
    } catch (error) {
      return { success: false, message: `Fehler beim Laden der Grow-Pläne: ${error.toString()}` };
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
  
  async deleteGrowPlan(username, growPlanName) {
    const confirmDelete = window.confirm("Möchten Sie diesen Growplan wirklich löschen?");
    if (confirmDelete) {
      try {
        const response = await fetch("http://localhost:5000/delete-grow-plan", {
          method: "DELETE",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({
            username,
            growPlanName,
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
    }
    return { success: false, message: "Aktion abgebrochen." };
  },
  
   async deleteCyclePlan(username, growCycleName) {
    const confirmDelete = window.confirm("Möchten Sie diesen Cycle wirklich löschen?");
    if (confirmDelete) {
      try {
        const response = await fetch("http://localhost:5000/delete-cycle-plan", {
          method: "DELETE",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({
            username,
            growCycleName,
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
    }
    return { success: false, message: "Aktion abgebrochen." };
  }
  
};

export default GrowPlanService;
