# E-GRU Model Development

This model is designed to predict future environmental conditions in the poultry farm, such as temperature, humidity, CO₂, and NH₃, with a `30-minute forecasting window`. By training the model on the synthetic dataset generated in the previous step, that aim to create an accurate and efficient model for decision-making in environmental control.

## Step 1: Install and Import Packages

This block installs the necessary Python packages for the E-GRU model development, including TensorFlow, scikit-learn, and matplotlib. It then imports the required libraries for data processing, model building, and evaluation. Reproducibility is ensured by setting random seeds for NumPy, random, and TensorFlow.

The installed libraries include:

- **NumPy** and **Pandas** for data manipulation
- **scikit-learn** for preprocessing and evaluation
- **TensorFlow** for building and training the E-GRU model

```py
!pip -q install numpy pandas scikit-learn matplotlib joblib tensorflow

import os
import random
import joblib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import tensorflow as tf

from sklearn.compose import ColumnTransformer
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score

from tensorflow.keras.models import Model
from tensorflow.keras.layers import (
    Input, GRU, Dense, Dropout, LayerNormalization,
    MultiHeadAttention, Add, GlobalAveragePooling1D
)
from tensorflow.keras.callbacks import EarlyStopping, ReduceLROnPlateau, ModelCheckpoint

# Reproducibility
SEED = 42
np.random.seed(SEED)
random.seed(SEED)
tf.keras.utils.set_random_seed(SEED)

print("TensorFlow version:", tf.__version__)
```

## Step 2: Upload and Read Dataset

This block allows for uploading the **forecast_target_table_v1.csv** file from Google Colab. After uploading, it reads the CSV file into a Pandas DataFrame, parses the `timestamp` column as dates, and displays the shape and first few rows of the dataset to check its structure.

The dataset is loaded and ready for further processing in the model.

```py
from google.colab import files
uploaded = files.upload()
```

```py
file_path = "forecast_target_table_v1.csv"

df = pd.read_csv(file_path, parse_dates=["timestamp"])
print("Dataset shape:", df.shape)
print(df.head())
```

## Step 3: Basic Inspection

This block performs a basic inspection of the dataset. It checks the column names, identifies any missing values, reviews the data types of each column, and examines the range of the `age_day` column. This step helps ensure the dataset is ready for further processing.

```py
print("\nColumns:")
print(df.columns.tolist())

print("\nMissing values:")
print(df.isnull().sum())

print("\nData types:")
print(df.dtypes)

print("\nAge day range:")
print(df["age_day"].min(), "to", df["age_day"].max())
```

## Step 4: Sort Data and Define Columns

This block sorts the dataset by `coop_id`, `flock_id`, and `timestamp`, and resets the index. It defines columns for the target variables (30-minute future values) and separates features into numeric and categorical columns. It then creates a `group_id` column and keeps only the necessary columns for model development.

```py
df = df.sort_values(["coop_id", "flock_id", "timestamp"]).reset_index(drop=True)

# Group ID for sequence building
df["group_id"] = df["coop_id"].astype(str) + "_" + df["flock_id"].astype(str)

# Target columns (30-minute future)
target_cols = [
    "temp_t_plus_30",
    "humidity_t_plus_30",
    "co2_t_plus_30",
    "nh3_t_plus_30"
]

# Numeric features
numeric_cols = [
    "age_day",
    "outside_temp",
    "outside_humidity",
    "solar_factor",
    "temp",
    "humidity",
    "co2",
    "nh3",
    "T_min",
    "T_max",
    "RH_min",
    "RH_max",
    "CO2_warning",
    "CO2_strong",
    "CO2_emergency",
    "NH3_warning",
    "NH3_strong",
    "NH3_emergency",
    "heating_light",
    "chilling_fan",
    "exhaust_fan",
    "emergency_flag"
]

# Categorical features
categorical_cols = [
    "flock_style",
    "mode",
    "reason_code",
    "event_code"
]

# Keep only needed columns
keep_cols = ["timestamp", "group_id"] + numeric_cols + categorical_cols + target_cols
data = df[keep_cols].copy()

print("Final working data shape:", data.shape)
data.head()
```

## Step 5: Time-Based Split Inside Each Group

This block splits the dataset into training, validation, and test sets based on time. For each group (identified by `group_id`), it divides the data into 70% for training, 15% for validation, and 15% for testing. The resulting datasets are stored in `train_df`, `val_df`, and `test_df`.

```py
train_parts = []
val_parts = []
test_parts = []

for gid, gdf in data.groupby("group_id", sort=False):
    n = len(gdf)
    train_end = int(n * 0.70)
    val_end = int(n * 0.85)

    train_parts.append(gdf.iloc[:train_end].copy())
    val_parts.append(gdf.iloc[train_end:val_end].copy())
    test_parts.append(gdf.iloc[val_end:].copy())

train_df = pd.concat(train_parts, axis=0).reset_index(drop=True)
val_df   = pd.concat(val_parts, axis=0).reset_index(drop=True)
test_df  = pd.concat(test_parts, axis=0).reset_index(drop=True)

print("Train shape:", train_df.shape)
print("Val shape:", val_df.shape)
print("Test shape:", test_df.shape)
```

## Step 6: Preprocessing

This block preprocesses the data by applying **StandardScaler** to numeric features and **OneHotEncoder** to categorical features. It transforms the training, validation, and test sets into 2D arrays. The target variables are also scaled using **StandardScaler**.

```py
X_train_raw = train_df[numeric_cols + categorical_cols]
X_val_raw   = val_df[numeric_cols + categorical_cols]
X_test_raw  = test_df[numeric_cols + categorical_cols]

y_train_raw = train_df[target_cols].copy()
y_val_raw   = val_df[target_cols].copy()
y_test_raw  = test_df[target_cols].copy()

# Handle sklearn version compatibility
try:
    ohe = OneHotEncoder(handle_unknown="ignore", sparse_output=False)
except TypeError:
    ohe = OneHotEncoder(handle_unknown="ignore", sparse=False)

preprocessor = ColumnTransformer(
    transformers=[
        ("num", StandardScaler(), numeric_cols),
        ("cat", ohe, categorical_cols)
    ]
)

X_train_2d = preprocessor.fit_transform(X_train_raw).astype(np.float32)
X_val_2d   = preprocessor.transform(X_val_raw).astype(np.float32)
X_test_2d  = preprocessor.transform(X_test_raw).astype(np.float32)

# Scale targets too
y_scaler = StandardScaler()
y_train_2d = y_scaler.fit_transform(y_train_raw).astype(np.float32)
y_val_2d   = y_scaler.transform(y_val_raw).astype(np.float32)
y_test_2d  = y_scaler.transform(y_test_raw).astype(np.float32)

print("Processed X_train shape:", X_train_2d.shape)
print("Processed y_train shape:", y_train_2d.shape)
```

### Step 7: Build Sequences

This block creates sequences from the preprocessed data to build the temporal structure required for the E-GRU model. It generates sequences of length 24 (representing 2 hours of data) for the training, validation, and test sets.

```py
SEQ_LEN = 24   # 24 x 5 min = 120 min history

def make_sequences(split_df, X_2d, y_2d, seq_len=24):
    X_seq, y_seq = [], []

    groups = split_df["group_id"].values
    unique_groups = split_df["group_id"].unique()

    for gid in unique_groups:
        idx = np.where(groups == gid)[0]
        Xg = X_2d[idx]
        yg = y_2d[idx]

        for i in range(seq_len, len(Xg)):
            X_seq.append(Xg[i-seq_len:i])
            y_seq.append(yg[i])

    return np.array(X_seq, dtype=np.float32), np.array(y_seq, dtype=np.float32)

X_train_seq, y_train_seq = make_sequences(train_df, X_train_2d, y_train_2d, SEQ_LEN)
X_val_seq, y_val_seq     = make_sequences(val_df, X_val_2d, y_val_2d, SEQ_LEN)
X_test_seq, y_test_seq   = make_sequences(test_df, X_test_2d, y_test_2d, SEQ_LEN)

print("X_train_seq:", X_train_seq.shape)
print("y_train_seq:", y_train_seq.shape)
print("X_val_seq:", X_val_seq.shape)
print("y_val_seq:", y_val_seq.shape)
print("X_test_seq:", X_test_seq.shape)
print("y_test_seq:", y_test_seq.shape)
```

## Step 8: Build E-GRU Model

This block defines and builds the **E-GRU model** using TensorFlow. It consists of multiple GRU layers, multi-head attention, and dense layers for regression tasks. The model is compiled with the Adam optimizer and Huber loss function for training.

```py
def build_egru_model(seq_len, n_features, n_outputs):
    inp = Input(shape=(seq_len, n_features))

    x = GRU(128, return_sequences=True)(inp)
    x = LayerNormalization()(x)
    x = Dropout(0.20)(x)

    x = GRU(64, return_sequences=True)(x)
    x = LayerNormalization()(x)

    attn = MultiHeadAttention(num_heads=4, key_dim=16, dropout=0.10)(x, x)
    x = Add()([x, attn])
    x = LayerNormalization()(x)

    x = GlobalAveragePooling1D()(x)

    x = Dense(128, activation="relu")(x)
    x = Dropout(0.20)(x)

    x = Dense(64, activation="relu")(x)

    out = Dense(n_outputs, activation="linear")(x)

    model = Model(inputs=inp, outputs=out)
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
        loss=tf.keras.losses.Huber(),
        metrics=["mae"]
    )
    return model

model = build_egru_model(
    seq_len=SEQ_LEN,
    n_features=X_train_seq.shape[2],
    n_outputs=y_train_seq.shape[1]
)

model.summary()
```

## Step 9: Train Model

This block trains the E-GRU model with the training and validation data. It includes callbacks for early stopping, learning rate reduction, and model checkpointing to save the best model based on validation loss.

The model is now being trained for 80 epochs.

```py
callbacks = [
    EarlyStopping(
        monitor="val_loss",
        patience=12,
        restore_best_weights=True
    ),
    ReduceLROnPlateau(
        monitor="val_loss",
        factor=0.5,
        patience=5,
        min_lr=1e-6,
        verbose=1
    ),
    ModelCheckpoint(
        "best_egru_model.keras",
        monitor="val_loss",
        save_best_only=True,
        verbose=1
    )
]

history = model.fit(
    X_train_seq, y_train_seq,
    validation_data=(X_val_seq, y_val_seq),
    epochs=80,
    batch_size=128,
    callbacks=callbacks,
    verbose=1
)
```

## Step 10: Plot Training History

This block plots the training and validation loss and mean absolute error (MAE) curves over epochs. It helps visualize the model's performance during training.

```py
plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.plot(history.history["loss"], label="Train Loss")
plt.plot(history.history["val_loss"], label="Val Loss")
plt.title("Loss Curve")
plt.xlabel("Epoch")
plt.ylabel("Loss")
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(history.history["mae"], label="Train MAE")
plt.plot(history.history["val_mae"], label="Val MAE")
plt.title("MAE Curve")
plt.xlabel("Epoch")
plt.ylabel("MAE")
plt.legend()

plt.tight_layout()
plt.show()
```

## Step 11: Predict and Inverse Transform

This block makes predictions using the trained E-GRU model and then applies the inverse transformation to the predicted and true target values. It recovers the original scale of the targets for training, validation, and test sets.

```py
y_train_pred_scaled = model.predict(X_train_seq, verbose=0)
y_val_pred_scaled   = model.predict(X_val_seq, verbose=0)
y_test_pred_scaled  = model.predict(X_test_seq, verbose=0)

y_train_true = y_scaler.inverse_transform(y_train_seq)
y_val_true   = y_scaler.inverse_transform(y_val_seq)
y_test_true  = y_scaler.inverse_transform(y_test_seq)

y_train_pred = y_scaler.inverse_transform(y_train_pred_scaled)
y_val_pred   = y_scaler.inverse_transform(y_val_pred_scaled)
y_test_pred  = y_scaler.inverse_transform(y_test_pred_scaled)
```

## Step 12: Evaluation Functions

This block defines two evaluation functions:

1. **`regression_report`**: Calculates and prints performance metrics (MAE, RMSE, R²) for each target variable and overall.
2. **`tolerance_accuracy`**: Evaluates the model's accuracy within specified tolerance levels for each target.


```py
def regression_report(y_true, y_pred, target_names, title="Report"):
    print(f"\n{'='*60}")
    print(title)
    print(f"{'='*60}")

    r2_list = []

    for i, col in enumerate(target_names):
        mae = mean_absolute_error(y_true[:, i], y_pred[:, i])
        rmse = np.sqrt(mean_squared_error(y_true[:, i], y_pred[:, i]))
        r2 = r2_score(y_true[:, i], y_pred[:, i])
        r2_list.append(r2)

        print(f"\nTarget: {col}")
        print(f"MAE  : {mae:.4f}")
        print(f"RMSE : {rmse:.4f}")
        print(f"R²   : {r2:.4f}")

    overall_r2 = r2_score(y_true, y_pred, multioutput="uniform_average")
    print(f"\nOverall Average R²: {overall_r2:.4f}")
    print(f"Mean Target-wise R²: {np.mean(r2_list):.4f}")

def tolerance_accuracy(y_true, y_pred, target_names, tolerances):
    print(f"\n{'='*60}")
    print("Tolerance Accuracy")
    print(f"{'='*60}")

    scores = []

    for i, col in enumerate(target_names):
        tol = tolerances[col]
        acc = np.mean(np.abs(y_true[:, i] - y_pred[:, i]) <= tol)
        scores.append(acc)
        print(f"{col} within ±{tol}: {acc:.4f}")

    print(f"\nAverage Tolerance Accuracy: {np.mean(scores):.4f}")
```

```py
# Train metrics
regression_report(y_train_true, y_train_pred, target_cols, title="TRAIN REPORT")

# Validation metrics
regression_report(y_val_true, y_val_pred, target_cols, title="VALIDATION REPORT")

# Test metrics
regression_report(y_test_true, y_test_pred, target_cols, title="TEST REPORT")

# Tolerance-based accuracy
tolerances = {
    "temp_t_plus_30": 1.0,
    "humidity_t_plus_30": 5.0,
    "co2_t_plus_30": 150.0,
    "nh3_t_plus_30": 1.0
}

print("\nTRAIN TOLERANCE ACCURACY")
tolerance_accuracy(y_train_true, y_train_pred, target_cols, tolerances)

print("\nVALIDATION TOLERANCE ACCURACY")
tolerance_accuracy(y_val_true, y_val_pred, target_cols, tolerances)

print("\nTEST TOLERANCE ACCURACY")
tolerance_accuracy(y_test_true, y_test_pred, target_cols, tolerances)
```

## Step 13: Acceptance Rule

This block evaluates the model's performance using the R² score on the validation and test sets. If both R² scores are greater than or equal to 0.90, the model is considered acceptable. Otherwise, it indicates the need for tuning.

```py
val_r2 = r2_score(y_val_true, y_val_pred, multioutput="uniform_average")
test_r2 = r2_score(y_test_true, y_test_pred, multioutput="uniform_average")

print("\nValidation Average R²:", round(val_r2, 4))
print("Test Average R²:", round(test_r2, 4))

if val_r2 >= 0.90 and test_r2 >= 0.90:
    print("\nModel Status: ACCEPTABLE (>= 0.90 average R² on validation and test)")
else:
    print("\nModel Status: NEEDS TUNING")
```

## Step 14: Plot Actual vs Predicted

This block plots the actual vs predicted values for each target variable (temperature, humidity, CO₂, NH₃) for the first 300 points in the test set. The plots help visualize the model's performance over time.

```py
plot_targets = target_cols
n_points = 300   # show first 300 test points

for i, col in enumerate(plot_targets):
    plt.figure(figsize=(14, 4))
    plt.plot(y_test_true[:n_points, i], label=f"Actual {col}")
    plt.plot(y_test_pred[:n_points, i], label=f"Predicted {col}")
    plt.title(f"Actual vs Predicted - {col}")
    plt.xlabel("Time index")
    plt.ylabel(col)
    plt.legend()
    plt.show()
```

## Step 15: Save Model and Preprocessors

This block saves the trained E-GRU model, the data preprocessor, and the target scaler to disk. The model and preprocessors are saved in `.keras` and `.pkl` formats, respectively, for future use.

```py
model.save("final_egru_forecast_model.keras")
joblib.dump(preprocessor, "egru_preprocessor.pkl")
joblib.dump(y_scaler, "egru_target_scaler.pkl")

print("Saved:")
print("- final_egru_forecast_model.keras")
print("- egru_preprocessor.pkl")
print("- egru_target_scaler.pkl")
```

## Step 16: Export Predicted Future Values

This block generates a DataFrame with the predicted and true values for future targets (temperature, humidity, CO₂, NH₃) based on the model's predictions. It saves the results as a CSV file for further analysis.

The predictions are now exported to **`egru_test_forecast_output.csv`**.

```py
def build_sequence_prediction_dataframe(split_df, y_true, y_pred, seq_len, target_cols):
    out_rows = []
    unique_groups = split_df["group_id"].unique()

    current_pos = 0
    for gid in unique_groups:
        gdf = split_df[split_df["group_id"] == gid].reset_index(drop=True)

        for i in range(seq_len, len(gdf)):
            row = {
                "timestamp": gdf.loc[i, "timestamp"],
                "group_id": gid,
                "age_day": gdf.loc[i, "age_day"],
                "temp_current": gdf.loc[i, "temp"],
                "humidity_current": gdf.loc[i, "humidity"],
                "co2_current": gdf.loc[i, "co2"],
                "nh3_current": gdf.loc[i, "nh3"]
            }

            for j, col in enumerate(target_cols):
                row[f"true_{col}"] = y_true[current_pos, j]
                row[f"pred_{col}"] = y_pred[current_pos, j]

            out_rows.append(row)
            current_pos += 1

    return pd.DataFrame(out_rows)

test_forecast_df = build_sequence_prediction_dataframe(
    split_df=test_df,
    y_true=y_test_true,
    y_pred=y_test_pred,
    seq_len=SEQ_LEN,
    target_cols=target_cols
)

test_forecast_df.to_csv("egru_test_forecast_output.csv", index=False)
print("Saved: egru_test_forecast_output.csv")
test_forecast_df.head()
```

## Step 17: Load Full Forecast Dataset

This block loads the full forecast dataset from **`forecast_target_table.csv`**, sorts it by `coop_id`, `flock_id`, and `timestamp`, and creates stable keys (`group_id` and `row_in_group`) for safe time-series processing.

```py
full_df = pd.read_csv("forecast_target_table.csv", parse_dates=["timestamp"])

# Sort for safe time-series processing
full_df = full_df.sort_values(["coop_id", "flock_id", "timestamp"]).reset_index(drop=True)

# Stable keys
full_df["group_id"] = full_df["coop_id"].astype(str) + "_" + full_df["flock_id"].astype(str)
full_df["row_in_group"] = full_df.groupby("group_id").cumcount()

print("Full dataset shape:", full_df.shape)
print("Age range:", full_df["age_day"].min(), "to", full_df["age_day"].max())
print("Number of groups:", full_df["group_id"].nunique())
full_df.head()
```

## Step 18: Prepare E-GRU Full Input Matrix

This block prepares the full input matrix for the E-GRU model by applying the same preprocessor used during training to the full dataset. The transformed data is now ready for forecasting.

```py
X_full_raw = full_df[numeric_cols + categorical_cols].copy()

# IMPORTANT: use the SAME preprocessor fitted during E-GRU training
X_full_2d = preprocessor.transform(X_full_raw).astype(np.float32)

print("Full transformed X shape:", X_full_2d.shape)
```

## Step 19: Build Full E-GRU Sequences

This block creates sequences from the full dataset, similar to the training data, using the preprocessed input matrix. It generates sequences of the specified length (`SEQ_LEN`) for each group and returns the sequences along with the original row references.

```py
def make_full_sequences(df, X_2d, seq_len):
    X_seq = []
    row_refs = []

    for gid, gdf in df.groupby("group_id", sort=False):
        idx = gdf.index.to_numpy()
        Xg = X_2d[idx]

        for i in range(seq_len, len(gdf)):
            X_seq.append(Xg[i-seq_len:i])
            row_refs.append(idx[i])   # original row reference in full_df

    return np.array(X_seq, dtype=np.float32), row_refs

X_full_seq, full_row_refs = make_full_sequences(full_df, X_full_2d, SEQ_LEN)

print("X_full_seq shape:", X_full_seq.shape)
print("Aligned row refs:", len(full_row_refs))
```

## Step 20: Predict Full E-GRU Outputs

This block uses the trained E-GRU model to make predictions on the full dataset sequences. The predictions are then inverse-transformed to their original scale.

```py
y_full_pred_scaled = model.predict(X_full_seq, verbose=0)
y_full_pred = y_scaler.inverse_transform(y_full_pred_scaled)

print("Full prediction array shape:", y_full_pred.shape)
```

## Step 21: Build Full E-GRU Output Dataset

This block constructs the full output dataset by aligning the original dataset with the model's predictions. It includes current values, true future values, and predicted future values for each target variable. Additionally, error columns are calculated for auditing purposes.

```py
aligned_df = full_df.loc[full_row_refs].copy().reset_index(drop=True)

egru_full_output = aligned_df[[
    "timestamp",
    "coop_id",
    "flock_id",
    "scenario_id",
    "group_id",
    "row_in_group",
    "house_type",
    "season",
    "flock_style",
    "age_day",
    "outside_temp",
    "outside_humidity",
    "solar_factor",
    "temp",
    "humidity",
    "co2",
    "nh3",
    "T_min",
    "T_max",
    "RH_min",
    "RH_max",
    "CO2_warning",
    "CO2_strong",
    "CO2_emergency",
    "NH3_warning",
    "NH3_strong",
    "NH3_emergency",
    "mode",
    "heating_light",
    "chilling_fan",
    "exhaust_fan",
    "emergency_flag",
    "reason_code",
    "event_code",
    "temp_t_plus_30",
    "humidity_t_plus_30",
    "co2_t_plus_30",
    "nh3_t_plus_30"
]].copy()

# Rename current and true future columns clearly
egru_full_output = egru_full_output.rename(columns={
    "temp": "temp_current",
    "humidity": "humidity_current",
    "co2": "co2_current",
    "nh3": "nh3_current",
    "temp_t_plus_30": "true_temp_t_plus_30",
    "humidity_t_plus_30": "true_humidity_t_plus_30",
    "co2_t_plus_30": "true_co2_t_plus_30",
    "nh3_t_plus_30": "true_nh3_t_plus_30"
})

# Add E-GRU predictions
egru_full_output["pred_temp_t_plus_30"] = y_full_pred[:, 0]
egru_full_output["pred_humidity_t_plus_30"] = y_full_pred[:, 1]
egru_full_output["pred_co2_t_plus_30"] = y_full_pred[:, 2]
egru_full_output["pred_nh3_t_plus_30"] = y_full_pred[:, 3]

# Simple error columns for audit only
egru_full_output["err_temp"] = egru_full_output["true_temp_t_plus_30"] - egru_full_output["pred_temp_t_plus_30"]
egru_full_output["err_humidity"] = egru_full_output["true_humidity_t_plus_30"] - egru_full_output["pred_humidity_t_plus_30"]
egru_full_output["err_co2"] = egru_full_output["true_co2_t_plus_30"] - egru_full_output["pred_co2_t_plus_30"]
egru_full_output["err_nh3"] = egru_full_output["true_nh3_t_plus_30"] - egru_full_output["pred_nh3_t_plus_30"]

print("egru_full_output shape:", egru_full_output.shape)
egru_full_output.head()
```

## Step 22: Save Full E-GRU Prediction File

This block saves the full E-GRU forecast output, including both the true and predicted values, to a CSV file named **`egru_full_forecast_output.csv`**.

```py
egru_full_output.to_csv("egru_full_forecast_output.csv", index=False)
print("Saved: egru_full_forecast_output.csv")
```

## Next Step: Create Master Model Dataset

The next step focuses on preparing a **master dataset** that combines all relevant data sources for model training. This dataset will incorporate the features, target variables, and additional engineered variables necessary for the final model, providing a comprehensive input for developing a robust predictive model.
