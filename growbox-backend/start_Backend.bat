@echo off
cd %~dp0


start cmd.exe /k "venv\Scripts\activate & python app.py"
