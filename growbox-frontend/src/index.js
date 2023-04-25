import React from "react";
import ReactDOM from "react-dom";
import { createRoot } from "react-dom/client"; // Importieren Sie createRoot von "react-dom/client"
import "bootstrap/dist/css/bootstrap.min.css";
import App from "./App";

// Erstellen Sie eine Root-Instanz und übergeben Sie das DOM-Element
const root = createRoot(document.getElementById("root"));

// Rendern Sie die App-Komponente innerhalb der Root-Instanz
root.render(<App />);
