from flask_cors import CORS, cross_origin
from pymongo import MongoClient
from flask_bcrypt import Bcrypt
from flask import Flask, request, jsonify, render_template, redirect, url_for, flash, session, send_from_directory
from urllib.parse import parse_qs

# Importiere den Passwort-Hasher und den Passwort-Verifier
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__, template_folder="templates")

CORS(app, resources={r"*": {"origins": "*", "supports_credentials": True}})

app.config["SECRET_KEY"] = "your_secret_key"
app.config["CORS_HEADERS"] = "Content-Type"

# Set the database name
app.config["DATABASE_NAME"] = "your_database_name"
app.config["DATABASE_NAME_USER"] = "users"
app.config["DATABASE_NAME_GROW_PLANS"] = "grow_plans"

bcrypt = Bcrypt(app)

# Verbindung zur MongoDB
client = MongoClient("mongodb://192.168.178.25:49155/")
db = client[app.config["DATABASE_NAME"]]
users = db["users"]

@app.route("/save-grow-plan", methods=["POST"])
def save_grow_plan():
    data = request.get_json()
    print("data:")
    print(data)
    username = data["username"]
    growCycleName = data["growCycleName"]
    overwrite = data["overwrite"]
    #del data["username"]
    del data["overwrite"]

    existing_plan = db[app.config["DATABASE_NAME_GROW_PLANS"]].find_one({"username": username, "growCycleName": growCycleName})
    print(existing_plan)
    print(username)
    print(data["growCycleName"])

    if existing_plan:
        if overwrite:
            db[app.config["DATABASE_NAME_GROW_PLANS"]].update_one({"_id": existing_plan["_id"]}, {"$set": data})
        else:
            return jsonify({"message": "Grow plan with this name already exists."}), 400
    else:
        db[app.config["DATABASE_NAME_GROW_PLANS"]].insert_one(data)

    return jsonify({"message": "Grow plan saved successfully."}), 200





@app.route("/get-grow-plans/<username>", methods=["GET"])
def get_grow_plans(username):
    try:
        grow_plans = list(db[app.config["DATABASE_NAME_GROW_PLANS"]].find({"username": username}))
        for plan in grow_plans:
            plan["_id"] = str(plan["_id"])
        return jsonify({"status": "success", "data": grow_plans}), 200
    except Exception as e:
        print(e)
        return jsonify({"status": "error", "message": "Unable to fetch grow plans."}), 500


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




@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        data = request.get_json()
        username = data["username"]
        password = data["password"]
        user = db["users"].find_one({"username": username})

        if user and check_password_hash(user["password"], password):
            # Set the user ID in the session
            session["user_id"] = str(user["_id"])
            flash("Login successful!")
            return jsonify({"message": "Login successful!"})
        else:
            return jsonify({"message": "Invalid username or password."})

    return render_template("login.html")



if __name__ == "__main__":
    # Beispiel-Datensätze für Benutzer "user1"
    #db["user1"].insert_one({"user_id": "user1", "temperature": 25.0, "humidity": 60.0})
    #db["user1"].insert_one({"user_id": "user1", "temperature": 24.5, "humidity": 62.0})
    #db["user1"].insert_one({"user_id": "user1", "temperature": 23.0, "humidity": 65.0})

    # Beispiel-Datensätze für Benutzer "user2"
    #db["user2"].insert_one({"user_id": "user2", "light_intensity": 5000, "water_level": 50})
    #db["user2"].insert_one({"user_id": "user2", "light_intensity": 6000, "water_level": 40})
    #db["user2"].insert_one({"user_id": "user2", "light_intensity": 5500, "water_level": 45})

    app.run(debug=True)
