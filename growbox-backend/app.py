from flask import Flask, request, jsonify
from flask_cors import CORS
from pymongo import MongoClient

app = Flask(__name__)
CORS(app)

# Verwenden Sie die Verbindungsdaten Ihrer MongoDB-Instanz
client = MongoClient("mongodb://username:password@your.mongodb.host:27135/database")
db = client["your_database_name"]

@app.route("/save-grow-plan", methods=["POST"])
def save_grow_plan():
    data = request.get_json()
    db.grow_plans.insert_one(data)
    return jsonify({"message": "Grow plan saved successfully"}), 201

@app.route("/get-grow-plans", methods=["GET"])
def get_grow_plans():
    grow_plans = list(db.grow_plans.find())
    for plan in grow_plans:
        plan["_id"] = str(plan["_id"])
    return jsonify(grow_plans), 200

if __name__ == "__main__":
    app.run(debug=True)
