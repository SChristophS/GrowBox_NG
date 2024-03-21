@echo off
cd %~dp0

cd growbox-backend
start cmd.exe /k "venv\Scripts\activate & python app.py"
start cmd.exe /k "venv\Scripts\activate & python socketServer.py"

cd ..
cd growbox-frontend
start cmd.exe /k "npm start"
