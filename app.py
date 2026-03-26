from flask import Flask, render_template, request, jsonify
import pickle
import numpy as np

app = Flask(__name__)

# Load trained model
model = pickle.load(open('model/model.pkl', 'rb'))

@app.route('/')
def home():
    return render_template('index.html')

# Web form prediction
@app.route('/predict', methods=['POST'])
def predict():
    current = float(request.form['current'])
    voltage = float(request.form['voltage'])

    prediction = model.predict([[current, voltage]])

    if prediction[0] == 1:
        result = "⚠️ Theft Detected"
    else:
        result = "✅ Normal Usage"

    return render_template('index.html', prediction_text=result)

# ESP32 API (JSON)
@app.route('/api/data', methods=['POST'])
def api_data():
    data = request.get_json()

    current = data['current']
    voltage = data['voltage']

    prediction = model.predict([[current, voltage]])

    if prediction[0] == 1:
        return jsonify({"status": "theft", "relay": "OFF"})
    else:
        return jsonify({"status": "normal", "relay": "ON"})

if __name__ == "__main__":
    app.run(debug=True)
