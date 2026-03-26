import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
import pickle

# Sample dataset (you can replace with real data)
data = {
    'current': [2, 3, 5, 10, 12, 20, 25, 30],
    'voltage': [220, 220, 220, 220, 220, 220, 220, 220],
    'label':   [0, 0, 0, 1, 1, 1, 1, 1]  # 0=Normal, 1=Theft
}

df = pd.DataFrame(data)

X = df[['current', 'voltage']]
y = df['label']

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

model = RandomForestClassifier()
model.fit(X_train, y_train)

# Save model
pickle.dump(model, open('model/model.pkl', 'wb'))

print("✅ Model trained and saved!")
