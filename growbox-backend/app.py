from flask_cors import CORS, cross_origin
from pymongo import MongoClient
from flask_bcrypt import Bcrypt
from flask import Flask, request, jsonify, render_template, redirect, url_for, flash, session, send_from_directory
from urllib.parse import parse_qs
from werkzeug.security import generate_password_hash, check_password_hash
import paho.mqtt.client as mqtt
import time
import threading
from bson import ObjectId
from bson import json_util
import json


app = Flask(__name__, template_folder="templates")

CORS(app, resources={r"*": {"origins": "*", "supports_credentials": True}})

app.config["SECRET_KEY"] = "your_secret_key"
app.config["CORS_HEADERS"] = "Content-Type"

# Set the database name
app.config["DATABASE_NAME"] = "your_database_name"
app.config["DATABASE_NAME_USER"] = "users"
app.config["DATABASE_NAME_GROW_PLANS"] = "grow_plans"
app.config["DATABASE_NAME_CYCLE_PLANS"] = "cycle_plans"
app.config["DATABASE_NAME_DEVICES"] = "devices"
app.config["DEVICES"] = "devices"

bcrypt = Bcrypt(app)

# Verbindung zur MongoDB
#client = MongoClient("mongodb://192.168.178.25:49186/")
client = MongoClient("mongodb://192.168.178.25:49155/")
db = client[app.config["DATABASE_NAME"]]
users = db["users"]

mqtt_username = "christoph"
mqtt_password = "Aprikose99"

           

# MQTT client
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(mqtt_username, mqtt_password)


connected_devices = {}
device_timers = {}

def mark_device_as_disconnected(device_id):
    if device_id in connected_devices:
        del connected_devices[device_id]
        print(f"Device {device_id} is now disconnected.")

def send_alive_messages():
    while True:
        mqtt_client.publish("growbox/alive", "I am alive")
        time.sleep(5)
        
def on_disconnect(client, userdata, rc):
    print("Disconnected with result code "+str(rc))
    device_id = userdata
    if device_id in connected_devices:
        del connected_devices[device_id]
        print(f"Device {device_id} is now disconnected.")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully.")
    else:
        print(f"Connection failed with error code {rc}")
        
    client.subscribe("growbox/connected")
    client.subscribe("growbox/disconnected")
    client.subscribe("growbox/status")

def on_message(client, userdata, msg):
    print(f"{msg.topic} {str(msg.payload)}")
    device_id = msg.payload.decode()
    print(f"device_id:{device_id}")

    if msg.topic == "growbox/status":
        print(f"message.topic is status")
        # Query the database to get the owner of the device
        #device = db["devices"].find_one({"device_id": device_id})

        device = db[app.config["DEVICES"]].find_one({"device_id": device_id})
        print(device)
        if device:
            username = device["username"]
            if device_id in connected_devices:
                # Device is already in the dictionary, just print a status message
                print(f"Device {device_id} owned by {username} is already connected.")
            else:
                # Device is not in the dictionary, add it
                connected_devices[device_id] = username
                print(f"Device {device_id} owned by {username} is now connected.")
            
            print(connected_devices)
        else:
            print(f"Device {device_id} is now connected, but no owner found in the database.")

        # Start or restart the timer for this device
        if device_id in device_timers:
            device_timers[device_id].cancel()
        device_timers[device_id] = threading.Timer(10.0, mark_device_as_disconnected, args=[device_id])
        device_timers[device_id].start()

def send_growplan_to_growbox(device_id, grow_plan_data):
  """
  Sendet den Wachstumsplan an die Growbox mit der angegebenen ID.

  Args:
      device_id (str): Die ID der Growbox.
      grow_plan_data (dict): Das Wörterbuch mit den Daten des Wachstumsplans.

  Returns:
      None
  """

  if not device_id or not grow_plan_data:
    return

  # Erstellen der Topic-Zeichenkette
  topic = f"growbox/{device_id}/newGrowplan"

  # Serialisierung des Wachstumsplans in JSON
  payload = json.dumps(grow_plan_data)

  # Veröffentlichen der Nachricht
  mqtt_client.publish(topic, payload)

  print(f"Wachstumsplan an Growbox {device_id} gesendet.")


mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.on_disconnect = on_disconnect

connection_result = mqtt_client.connect("192.168.178.25", 49154, 60)
if connection_result == 0:
    print("Connected to MQTT broker successfully.")
else:
    print(f"Failed to connect to MQTT broker. Error code: {connection_result}")
    
from flask import jsonify, request
from pymongo import MongoClient


@app.route("/save-cycle-plan", methods=["POST"])
def save_cycle_plan():
    data = request.get_json()
    print("Erhaltene Daten:", data)  # Zeigt die empfangenen Daten an

    username = data["username"]
    growCycleName = data["growCycleName"]
    overwrite = data["overwrite"]
    del data["overwrite"]

    print(f"Benutzername: {username}, growCycleName: {growCycleName}, Überschreiben: {overwrite}")

    existing_plan = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].find_one({"username": username, "growCycleName": growCycleName})

    if existing_plan:
        print("Existierender Plan gefunden:", existing_plan)
        if overwrite:
            result = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].update_one({"_id": existing_plan["_id"]}, {"$set": data})
            print("Update-Ergebnis:", result.modified_count)  # Zeigt die Anzahl der geänderten Dokumente
        else:
            print("Plan existiert bereits und Überschreiben ist nicht erlaubt.")
            return jsonify({"message": "Grow plan with this name already exists. Overide is not allowed"}), 400
    else:
        print("Kein existierender Plan gefunden, füge neuen hinzu.")
        result = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].insert_one(data)
        print("Insert-Ergebnis:", result.inserted_id)  # Zeigt die ID des eingefügten Dokuments

    return jsonify({"message": "Grow plan saved successfully."}), 200

@app.route("/check-grow-plan-exists", methods=["POST"])
def check_grow_plan_exists():
    print(f"check_grow_plan_exists")
    data = request.get_json()
    print(data)
    username = data["username"]
    growPlanName = data["growPlanName"]
    print(username)
    print(growPlanName)

    existing_plan = db[app.config["DATABASE_NAME_GROW_PLANS"]].find_one({"username": username, "growPlanName": growPlanName})

    if existing_plan:
        return jsonify({"exists": True}), 200
    else:
        return jsonify({"exists": False}), 200
        
@app.route("/save-grow-plan", methods=["POST"])
def save_grow_plan():
    data = request.get_json()
    print("Erhaltene Daten:", data)  # Zeigt die empfangenen Daten an

    username = data["username"]
    growPlanName = data["growPlanName"]

    print(f"Benutzername: {username}, growPlanName: {growPlanName}")

    # wir checken ob es bereits einen Plan gibt der aktualisiert werden soll
    existing_plan = db[app.config["DATABASE_NAME_GROW_PLANS"]].find_one({"username": username, "growPlanName": growPlanName})
    
    if existing_plan:
        # wir updaten
        result = db[app.config["DATABASE_NAME_GROW_PLANS"]].update_one({"_id": existing_plan["_id"]}, {"$set": data})
        print("Update-Ergebnis:", result.modified_count)  # Zeigt die Anzahl der geänderten Dokumente
        return jsonify({"message": "Grow plan update successfully."}), 200
    else:
        # wir fügen neuen Hinzu
        result = db[app.config["DATABASE_NAME_GROW_PLANS"]].insert_one(data)
        print("Insert-Ergebnis:", result.inserted_id)  # Zeigt die ID des eingefügten Dokuments
        return jsonify({"message": "Grow plan insert successfully."}), 200

@app.route("/transmit-grow-plan-to-target", methods=["POST"])
def transmit_grow_plan_to_target():
    print("transmit_grow_plan_to_target aufgerufen")
    grow_plan_data = request.get_json()
    device_id = grow_plan_data['device_id']
    print(f"Transmit Received Data to Device mit device_id: {device_id}")

    send_growplan_to_growbox(device_id, grow_plan_data)
    return jsonify({"message": "Transmit to target was successfully."}), 200
  
@app.route("/delete-grow-plan", methods=["DELETE"])
def delete_grow_plan():
    data = request.get_json()
    print(data);
    growPlanID = data["growPlanID"]
    print("Delete GrowPlan with ID" , growPlanID)

    result = db[app.config["DATABASE_NAME_GROW_PLANS"]].delete_one({"_id": ObjectId(growPlanID)})

    if result.deleted_count > 0:
        return jsonify({"message": "Grow plan deleted successfully."}), 200
    else:
        return jsonify({"message": "Unable to delete the grow plan. Grow plan not found."}), 400

@app.route("/delete-cycle-plan", methods=["DELETE"])
def delete_cycle_plan():
    data = request.get_json()
    growCycleID = data["growCycleID"]
    print("Delete GrowCycle with ID" , growCycleID)

    result = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].delete_one({"_id": ObjectId(growCycleID)})

    if result.deleted_count > 0:
        return jsonify({"message": "cycle plan deleted successfully."}), 200
    else:
        return jsonify({"message": "Unable to delete the cycle plan. Cycle plan not found."}), 400

@app.route("/get-grow-plans/<username>", methods=["GET"])
def get_grow_plans(username):
    try:
        user_grow_plans = list(db[app.config["DATABASE_NAME_GROW_PLANS"]].find({"username": username}))
        #public_grow_plans = list(
        #    db[app.config["DATABASE_NAME_GROW_PLANS"]].find({"sharingStatus": "public", "username": {"$ne": username}}))

        #grow_plans = user_grow_plans + public_grow_plans
        #print(user_grow_plans)

        for plan in user_grow_plans:
            plan["_id"] = str(plan["_id"])
        return jsonify({"status": "success", "data": user_grow_plans}), 200
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch grow plans."}), 500
        
@app.route("/get-cycle-plans/<username>", methods=["GET"])
def get_cycle_plans(username):
    try:
        user_cycle_plans = list(db[app.config["DATABASE_NAME_CYCLE_PLANS"]].find({"username": username}))
        public_cycle_plans = list(
            db[app.config["DATABASE_NAME_CYCLE_PLANS"]].find({"sharingStatus": "public", "username": {"$ne": username}}))

        cycle_plans = user_cycle_plans + public_cycle_plans
        print(cycle_plans)

        for plan in cycle_plans:
            plan["_id"] = str(plan["_id"])
        return jsonify({"status": "success", "data": cycle_plans}), 200
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch cycle plans."}), 500
        
@app.route("/get-cycle-plan-from-id/<id>", methods=["GET"])
def get_cycle_plan(id):
    try:
        cycle_plan = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].find_one({"_id": ObjectId(id)})
        print(cycle_plan)
        
        cycle_plan_json = json_util.dumps(cycle_plan)

        return jsonify({"status": "success", "data": cycle_plan_json}), 200
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch cycle plan with id."}), 500        

@app.route("/get-growboxIDs-from-username/<username>", methods=["GET"])
def get_growboxIDs_from_username(username):
    print(f"/get_growboxIDs_from_username is called with username {username}")
    
    try:
        mapped_device_ids = list(
            db[app.config["DATABASE_NAME_DEVICES"]].find({"device_id": {"$ne": username}}))

        #print(f"/get_growboxIDs_from_username, result mapped_device_ids:")
        #print(mapped_device_ids)
        
        deviceIDs = []
        
        for device in mapped_device_ids:
            device_id = device['device_id']
            status = 'connected' if device_id in connected_devices else 'disconnected'
            print(f"/get_growboxIDs_from_username, found status for device with ID {device_id}, Status: {status}")
            
            deviceIDs.append({
                'device_id': device_id,
                'status': status
            })

        return jsonify({"status": "success", "data": deviceIDs}), 200
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch Device IDs."}), 500

@app.route("/get-cycle-totaltime/<cycle_id>", methods=["GET"])
def get_cycle_totaltime(cycle_id):
    try:
        # Entfernen Sie alles nach dem Bindestrich in der ID
        sanitized_cycle_id = cycle_id.split('-')[0]

        # Führen Sie die Abfrage mit der bereinigten ID durch
        cycle_plan = db[app.config["DATABASE_NAME_CYCLE_PLANS"]].find_one({"_id": ObjectId(sanitized_cycle_id)})
        if cycle_plan:
            total_grow_time = cycle_plan.get("growData", {}).get("totalGrowTime", 0)
            return jsonify({"status": "success", "totalGrowTime": total_grow_time}), 200
        else:
            return jsonify({"status": "error", "message": "Cycle Plan not found."}), 404
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch cycle total time."}), 500

@app.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        data = request.get_json()
        username = data["username"]
        password = data["password"]
        dummy1 = data["dummy1"]
        dummy2 = data["dummy2"]
        hashed_password = generate_password_hash(password)
        existing_user = db["users"].find_one({"username": username})

        if existing_user is None:
            db[app.config["DATABASE_NAME_USER"]].insert_one({"username": username, "password": hashed_password, "dummy1": dummy1, "dummy2": dummy2})
            return jsonify({"message": "Registration successful! Please log in."})
        else:
            return jsonify({"message": "Username is already taken."})

    else:
        return render_template("register.html")

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        data = request.get_json()
        username = data["username"]
        password = data["password"]
        user = db["users"].find_one({"username": username})

        if user and check_password_hash(user["password"], password):
            session["user_id"] = str(user["_id"])
            flash("Login successful!")
            return jsonify({"message": "Login successful!"})
        else:
            return jsonify({"message": "Invalid username or password."})

    return render_template("login.html")

@app.route('/devices', methods=['GET'])
def get_devices():
    print(f"/devices is called with {request}")
    username = request.args.get('username')
    print(f"/devices is called with username {username}")
    if not username:
        return {"error": "Username is required"}, 400

    #devices = db.devices.find({"username": username})
    devices = list(db[app.config["DEVICES"]].find({"username": username}))
    print(f"return from Database for devices: {devices}")
    result = []
    for device in devices:
        device_id = device['device_id']
        status = 'connected' if device_id in connected_devices else 'disconnected'
        result.append({
            'device_id': device_id,
            'status': status
        })

    return {"devices": result}, 200


if __name__ == "__main__":

    alive_thread = threading.Thread(target=send_alive_messages)
    alive_thread.start()
    
    mqtt_client.loop_start()  # start the MQTT background thread
    app.run(host='0.0.0.0', port=5000, debug=True)
