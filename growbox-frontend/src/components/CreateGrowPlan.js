import React, { useContext, useState, } from "react";
import { Modal, Button } from 'react-bootstrap';
import { DragDropContext, Droppable, Draggable } from 'react-beautiful-dnd';
import { GrowPlanContext } from '../contexts/GrowPlanContext';
import withContexts from '../contexts/withContexts'; // Pfad zur HOC-Datei anpassen
import { reorder } from '../utility/utils'; // Passen Sie den Pfad an Ihre Verzeichnisstruktur an


import GrowPlanServices from '../utility/GrowPlanServices';


// Anfangszustand der Cycle Plans
const initialCyclePlans = [];

// Anfangszustand für den zweiten Bereich, anfänglich leer
const initialDroppedItems = [];



class CreateGrowPlan extends React.Component {


	
  
	state = {
		cyclePlans: initialCyclePlans,
		droppedItems: initialDroppedItems,
		growPlanName: '',
		growPlanDescription: '',
		totalGrowDuration: 0,
		showNameRequiredModal: false,
		showConfirmModal: false,
		overwrite: false,
		saveSuccess: false,

	};
	
	
	saveGrowPlan = async () => {
		console.log("Aufruf saveGrowPlan");
			
		const username = this.props.authContext.username;
	  
		if (!this.state.growPlanName) {
			console.log("Kein Username");
			this.setState({ showNameRequiredModal: true });
			return;
		}	
		
		const growPlanData = {
			username: username,
			growPlanName: this.state.growPlanName,
			growPlanDescription: this.state.growPlanDescription,
			totalGrowDuration: this.state.totalGrowDuration,
			droppedItems: this.state.droppedItems.map(item => ({
				id: item.id,
				name: item.content,
			}))
		};
	   
	   // check if growplan exist
		const planExists = await GrowPlanServices.checkGrowPlanExists(username, growPlanData.growPlanName);
		console.log("Result planExists : " + planExists);

		  if (planExists) {
			this.setState({ showConfirmModal: true, growPlanData });
		  } else {
			this.submitGrowPlan(growPlanData);
		  }
	};
	
	submitGrowPlan = async (growPlanData) => {	
		console.log("aufruf submitGrowPlan");	
		console.log(growPlanData.username);
		
		try {
			console.log("Rufe GrowPlanServices.saveGrowPlan auf");
			const response = await GrowPlanServices.saveGrowPlan(growPlanData);
	
			if (response.success) {
				this.setState({ saveSuccess: true });
				console.log("Grow-Plan erfolgreich gespeichert:", response.data);
			} else {
				console.error("Fehler beim Speichern des Grow-Plans:", response.message);
			}
		} catch (error) {
			console.error("Fehler beim Aufruf von submitGrowPlan:", error);
		}
	}



componentDidUpdate(prevProps, prevState) {
  // Überprüfen, ob sich die droppedItems geändert haben
  if (prevState.droppedItems !== this.state.droppedItems) {
    const totalTime = this.calculateTotalTime();
    this.setState({ totalGrowDuration: totalTime });
  }
}

calculateTotalTime = () => {

  const totalTime = this.state.droppedItems.reduce((sum, item) => {
	  	    //console.log("item:", item);

    // Stellen Sie sicher, dass `item.totalGrowTime` existiert und ein gültiger Wert ist
    return sum + (item.totalGrowTime || 0);
  }, 0);
  return totalTime / 1440; // Umrechnen von Minuten in Tage
};
	
	getUniqueCycleNames = () => {
	  const uniqueNames = {};
	  this.state.droppedItems.forEach(item => {
		uniqueNames[item.content] = true; // Verwenden des Namens als Schlüssel im Objekt
	  });
	  return Object.keys(uniqueNames);
	};

  

  
componentDidMount() {
	console.log("Aufruf componentDidMount");
  const { loadedGrowPlan } = this.props.growPlanContext;

	console.log("loadedGrowPlan: ",loadedGrowPlan);

  if (loadedGrowPlan) {
	  console.log("loadedGrowPlan: ",loadedGrowPlan);
    // Aktualisieren Sie hier die Zustände mit den Daten des geladenen Grow Plans
    
	this.setState({
      growPlanName: loadedGrowPlan.growPlanName,
      growPlanDescription: loadedGrowPlan.growPlanDescription,
      totalGrowDuration: loadedGrowPlan.totalGrowDuration,
      // Für 'droppedItems' müssen Sie sicherstellen, dass die Daten korrekt formatiert sind
	  droppedItems: loadedGrowPlan.droppedItems.map(item => ({
        id: item.id, // oder eine andere ID, die Sie verwenden möchten
        content: item.name, // oder eine andere Eigenschaft, die Sie anzeigen möchten
		totalGrowTime: item.totalGrowTime
        // Fügen Sie hier weitere notwendige Eigenschaften hinzu
      }))
    });
  }

  // Laden der ursprünglichen Cycle Plans, falls kein Grow Plan geladen wurde
  else {
    const username = this.props.authContext.username;
    if (username) {
      this.props.growPlanContext.loadGrowPlans(username);
    }
  }
}


onDragEnd = (result) => {
  const { source, destination } = result;

  // Nichts tun, wenn das Element außerhalb eines droppable Bereichs abgelegt wird
  if (!destination) {
    return;
  }

  let sourceClone, destClone;

  // Zuweisen der Arrays basierend auf der droppableId
  if (source.droppableId === 'cyclePlans') {
    sourceClone = Array.from(this.props.growPlanContext.growPlans);
  } else if (source.droppableId === 'droppedItems') {
    sourceClone = Array.from(this.state.droppedItems);
  }

  if (destination.droppableId === 'cyclePlans') {
    destClone = Array.from(this.props.growPlanContext.growPlans);
  } else if (destination.droppableId === 'droppedItems') {
    destClone = Array.from(this.state.droppedItems);
  }

  const item = sourceClone[source.index];

  // Überprüfen, ob das Element existiert
  if (!item) {
    console.error('Zu verschiebendes Element nicht gefunden');
    return;
  }

  // Verschieben eines Elements zurück in den ursprünglichen "Stack"
  if (destination.droppableId === "cyclePlans") {
    // Entfernen des Elements aus dem aktuellen Container
    this.setState(prevState => ({
      droppedItems: prevState.droppedItems.filter(i => i.id !== item.id)
    }));
  } else {
    // Hinzufügen des Elements zum Zielcontainer und Vergabe einer neuen ID
    destClone.splice(destination.index, 0, { ...item, databaseID: `${item.id}`, id: `${item.id}-${destClone.length}` });
    this.setState({
      [destination.droppableId]: destClone
    });
  }
};




render() {
	const totalCycleTime = this.calculateTotalTime();
	const uniqueCycleNames = this.getUniqueCycleNames();
	const totalGrowDuration = this.state.totalGrowDuration;
	const progressPercentage = totalGrowDuration > 0 ? Math.min((totalCycleTime / totalGrowDuration) * 100, 100) : 0;
	const isOverLimit = totalCycleTime > totalGrowDuration;
    const cyclePlans = this.props.growPlanContext.growPlans;

	console.log("Render cyclePlans:", cyclePlans);
  
    const progressBarStyle = isOverLimit 
    ? { width: `${progressPercentage}%`, backgroundColor: 'red' } 
    : { width: `${progressPercentage}%` };
	

  return (
  
    <div> {/* Äußerer Container */}

  
    <DragDropContext onDragEnd={this.onDragEnd}>
      <div className="flex-container">
	  
        {/* Eingabebereich für Growplan-Details */}
        <div className="input-container">
          <div className="input-group">
            <label htmlFor="growPlanName">Growplan Name</label>
            <input 
              id="growPlanName"
              type="text" 
              value={this.state.growPlanName}
              onChange={(e) => this.setState({ growPlanName: e.target.value })}
            />
          </div>
          <div className="input-group">
            <label htmlFor="growPlanDescription">Beschreibung</label>
            <textarea 
              id="growPlanDescription"
              value={this.state.growPlanDescription}
              onChange={(e) => this.setState({ growPlanDescription: e.target.value })}
            />
          </div>
          <div className="input-group">
            <label htmlFor="totalGrowDuration">Gesamtdauer (Tage)</label>
            <input 
              id="totalGrowDuration"
              type="number" 
              value={this.state.totalGrowDuration}
              onChange={(e) => this.setState({ totalGrowDuration: e.target.value })}
            />
          </div>
          <button onClick={this.saveGrowPlan}>Speichern</button>
        </div>

		
		
<Droppable droppableId="cyclePlans">
  {(provided, snapshot) => (
    <div
      ref={provided.innerRef}
      className={`droppable-container initial-plans ${snapshot.isDraggingOver ? 'initial-plans-dragging-over' : ''}`}
      {...provided.droppableProps}
    >
      {cyclePlans.map((item, index) => (
        <Draggable key={item.id} draggableId={item.id} index={index}>
          {(provided, snapshot) => (
            <div
              ref={provided.innerRef}
              {...provided.draggableProps}
              {...provided.dragHandleProps}
              className={`draggable-item ${snapshot.isDragging ? 'draggable-item-dragging' : ''}`}
              style={provided.draggableProps.style}
            >
              <div>{item.content}</div>
              <div>{item.description}</div>
              <div>Total Grow Time: {item.totalGrowTime / 1440} Tage</div>
            </div>
          )}
        </Draggable>
      ))}
      {provided.placeholder}
    </div>
  )}
</Droppable>



        {/* Zweiter Bereich zum Ablegen der Cycle Plans */}
        <Droppable droppableId="droppedItems">
          {(provided, snapshot) => (
            <div
              ref={provided.innerRef}
              className={`droppable-container dropped-items ${snapshot.isDraggingOver ? 'dropped-items-dragging-over' : ''}`}
              {...provided.droppableProps}
            >
			{this.state.droppedItems.map((item, index) => (
			  <Draggable key={item.id} draggableId={item.id} index={index}>
				{(provided, snapshot) => (
				  <div
					ref={provided.innerRef}
					{...provided.draggableProps}
					{...provided.dragHandleProps}
					className={`draggable-item ${snapshot.isDragging ? 'draggable-item-dragging' : ''}`}
					style={provided.draggableProps.style}
				  >
					<div>{item.content}</div>
					<div>{item.description}</div>
					<div>Total Grow Time: {item.totalGrowTime / 1440} Tage</div>
				  </div>
				)}
			  </Draggable>
			))}	
              {provided.placeholder}
            </div>
          )}
        </Droppable>
		
      {/* Rechter Bereich */}
		<div className="information-about-growplan-container">
	          
        {/* Fortschrittsbalken */}
        <div className="progress-container">
          <div className="progress-bar" style={progressBarStyle}></div>
        </div>
		
        <div>Soll Tage: {totalGrowDuration}</div>
        <div>Ist Tage: {totalCycleTime}</div>
		
		{/* Warnmeldung, wenn Ist-Wert größer als Soll-Wert */}
        {isOverLimit && (
          <div className="warning-message">
            Warnung: Die Ist-Tage überschreiten die Soll-Tage!
          </div>
        )}

        {/* Anzeige der einzigartigen GrowCycles */}
        <div>Einzigartige GrowCycles:</div>
        <ul>
          {uniqueCycleNames.map(name => (
            <li key={name}>{name}</li>
          ))}
        </ul>
      </div>
		
      </div>
    </DragDropContext>


        {/* Modal-Dialog für "Plan überschreiben" Bestätigung */}
	<Modal show={this.state.showConfirmModal} onHide={() => this.setState({ showConfirmModal: false })}>
	  <Modal.Header closeButton>
		<Modal.Title>Grow Plan überschreiben</Modal.Title>
	  </Modal.Header>
	  <Modal.Body>
		Ein Grow Plan mit diesem Namen existiert bereits. Möchten Sie ihn überschreiben?
	  </Modal.Body>
	  <Modal.Footer>
		<Button variant="secondary" onClick={() => this.setState({ showConfirmModal: false })}>
		  Abbrechen
		</Button>
		<Button
		  type="button"
		  onClick={() => {
			this.submitGrowPlan(this.state.growPlanData);
			this.setState({ showConfirmModal: false });
		  }}
		>
		  Überschreiben
		</Button>
	  </Modal.Footer>
	</Modal>


     {/* Modal-Dialog für "Name erforderlich" Warnung */}
      <Modal 
        show={this.state.showNameRequiredModal} 
        onHide={() => this.setState({ showNameRequiredModal: false })}
      >
        <Modal.Header closeButton>
          <Modal.Title>Name erforderlich</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          Bitte geben Sie einen Namen für den Growplan ein.
        </Modal.Body>
        <Modal.Footer>
          <Button variant="secondary" onClick={() => this.setState({ showNameRequiredModal: false })}>
            Schließen
          </Button>
        </Modal.Footer>
      </Modal>
	  
</div>

	
  );
}
};

export default withContexts(CreateGrowPlan);
