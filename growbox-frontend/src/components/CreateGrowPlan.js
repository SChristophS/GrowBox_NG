import React, { useContext, useState} from "react";
import { Modal, Button } from 'react-bootstrap';
import { DragDropContext, Droppable, Draggable } from 'react-beautiful-dnd';
import { AuthContext } from "../contexts/AuthContext";
import GrowPlanServices from '../utility/GrowPlanServices';

// Eine einfache Funktion, um die Liste nach dem Drag & Drop zu aktualisieren
const reorder = (list, startIndex, endIndex) => {
  const result = Array.from(list);
  const [removed] = result.splice(startIndex, 1);
  result.splice(endIndex, 0, removed);
  return result;
};

// Anfangszustand der Cycle Plans
const initialCyclePlans = [];

// Anfangszustand für den zweiten Bereich, anfänglich leer
const initialDroppedItems = [];



class CreateGrowPlan extends React.Component {
	   static contextType = AuthContext;


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
			
		const username = this.context.username; 
	  
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
			console.log("planExists");
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




	calculateTotalTime = () => {
	  const totalTime = this.state.droppedItems.reduce((sum, item) => {
		return sum + item.totalGrowTime;
	  }, 0);
	  return totalTime  / 1440// Umrechnen von Minuten in Tage
	};
	
	getUniqueCycleNames = () => {
	  const uniqueNames = {};
	  this.state.droppedItems.forEach(item => {
		uniqueNames[item.content] = true; // Verwenden des Namens als Schlüssel im Objekt
	  });
	  return Object.keys(uniqueNames);
	};

  
	loadCyclePlans = async (username) => {
	  // Benutze das Argument `username`, das von componentDidMount übergeben wurde
	  const result = await GrowPlanServices.getCyclePlans(username);
	  if (result.success) {
		console.log('Cycle Plans geladen:', result.data);
		const formattedCyclePlans = result.data.map(plan => ({
		  id: plan._id,
		  content: plan.growCycleName,
		  description: plan.description,
			totalGrowTime: plan.growData.totalGrowTime
		}));
		this.setState({ cyclePlans: formattedCyclePlans });
	  } else {
		console.error(result.message);
	  }
	};
  
	componentDidMount() {
	  const username = this.context.username;
	  if (username) {
		console.log("Username = " + username);
		this.loadCyclePlans(username);
	  } else {
		console.log("Username ist nicht im Kontext definiert.");
	  }
	}

	onDragEnd = (result) => {
	  const { source, destination } = result;

	  // Nichts tun, wenn das Element außerhalb eines droppable Bereichs abgelegt wird
	  if (!destination) {
		return;
	  }

	  // Überprüfen, ob das Element innerhalb desselben Bereichs verschoben wird
	  if (source.droppableId === destination.droppableId) {
		// Wenn es sich um den gleichen Bereich handelt und es der Bereich 'droppedItems' ist
		if (source.droppableId === 'droppedItems') {
		  // Aktualisiere die Reihenfolge der Elemente innerhalb des 'droppedItems' Bereichs
		  const items = reorder(
			this.state.droppedItems,
			source.index,
			destination.index
		  );

		  // Setze den neuen Zustand
		  this.setState({
			droppedItems: items
		  });
		}
		return;
	  }

	  // Klonen der Arrays aus dem State
	  const sourceClone = Array.from(this.state[source.droppableId]);
	  const destClone = Array.from(this.state[destination.droppableId]);
	  const item = sourceClone[source.index];

	  // Verschieben eines Elements zurück in den ursprünglichen "Stack"
	  if (destination.droppableId === "cyclePlans") {
		// Entfernen des Elements aus dem aktuellen Container
		this.setState(prevState => ({
		  droppedItems: prevState.droppedItems.filter(i => i.id !== item.id)
		}));
	  } else {
		// Hinzufügen des Elements zum Zielcontainer und Vergabe einer neuen ID
		destClone.splice(destination.index, 0, { ...item, databaseID: `${item.id}`,  id: `${item.id}-${destClone.length}` });
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
			{this.state.cyclePlans.map((item, index) => (
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

export default CreateGrowPlan;