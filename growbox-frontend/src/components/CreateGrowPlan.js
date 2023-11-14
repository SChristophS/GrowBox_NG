import React, { useContext} from "react";
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
  };
  
	loadCyclePlans = async (username) => {
	  // Benutze das Argument `username`, das von componentDidMount übergeben wurde
	  const result = await GrowPlanServices.getCyclePlans(username);
	  if (result.success) {
		console.log('Cycle Plans geladen:', result.data);
		const formattedCyclePlans = result.data.map(plan => ({
		  id: plan._id,
		  content: plan.growCycleName,
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

	  // Verschieben innerhalb des gleichen Bereichs ignorieren
	  if (source.droppableId === destination.droppableId) {
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
		destClone.splice(destination.index, 0, { ...item, id: `new-${item.id}-${destClone.length}` });
		this.setState({
		  [destination.droppableId]: destClone
		});
	  }
	};




render() {
  return (
    <DragDropContext onDragEnd={this.onDragEnd}>
      <div className="flex-container">
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
                    </div>
                  )}
                </Draggable>
              ))}
              {provided.placeholder}
            </div>
          )}
        </Droppable>
      </div>
    </DragDropContext>
  );
}
};

export default CreateGrowPlan;
