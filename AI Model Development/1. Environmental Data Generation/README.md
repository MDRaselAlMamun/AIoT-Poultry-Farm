# AI Model Development (Data Generation)

This project focuses on developing an AI-based decision support and control system for a poultry farm environment. The full model development process is divided into four main stages:

1. **Synthetic data generation** for a standard 42-day broiler production cycle  
2. **Environmental forecasting** using an **E-GRU model** for 30-minute future prediction  
3. **Master dataset preparation** by combining current conditions, engineered features, and forecast outputs  
4. **CatBoost model development** for intelligent control decision-making in the poultry house  

## Running the Code on Google Colab

This code is designed to run on **Google Colab**, a cloud-based platform that allows you to write and execute Python code in a browser. Follow these steps to access and run the code:

1. **Open Google Colab**: Go to [Google Colab](https://colab.research.google.com/) and sign in with your Google account.

2. **Create a New Notebook**: Click on **"File" > "New Notebook"** to start a new project.

3. **Upload the Python Script**: Paste the code directly into a cell in the notebook.

4. **Run the Cells**: Execute each code block step by step by clicking the **play button** next to each cell or pressing `Shift + Enter` on your keyboard.

5. **View the Output**: After running the code, view the outputs (like datasets and visualizations) directly in the notebook.

## Step 1: Imports and Setup

This block sets up the project environment by importing required libraries, creating the output folder, and fixing the random seed for reproducible synthetic data generation.

```py
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from dataclasses import dataclass
from pathlib import Path
import zipfile
import json

from IPython.display import display

OUTPUT_DIR = Path("poultry_farm_dataset")
OUTPUT_DIR.mkdir(exist_ok=True)

np.random.seed(42)

print("Setup complete.")
```

## Step 2: Configuration Setup

This block defines the simulation configuration using the `SimConfig` dataclass. It sets up the parameters for the poultry farm simulation, including:

- **Flock settings**: Number of days, step interval, and flock style (e.g., normal, cool_brooding, heat_stress).
- **Environmental variables**: Initial values for temperature, humidity, CO₂, and NH₃.
- **Simulation identifiers**: Flock ID, coop ID, and scenario ID.

```py
@dataclass
class SimConfig:
    flock_days: int = 42
    step_minutes: int = 5
    season: str = "summer"
    house_type: str = "semi_closed"
    flock_style: str = "normal" 

    flock_id: str = "FLOCK_001"
    coop_id: str = "COOP_B1"
    scenario_id: str = "SUMMER_SEMI_1"
    seed: int = 42

    temp_init: float = 30.0
    rh_init: float = 63.0
    co2_init: float = 800.0
    nh3_init: float = 0.8

    co2_outdoor: float = 420.0
```

## Step 3: Age-Wise Targets

This function defines age-wise environmental target values for poultry based on the flock's age (in days). It returns minimum and maximum values for temperature, humidity, and thresholds for CO₂ and NH₃ based on the bird's age group.

```py
def age_profile(age_day: int) -> dict:
    if 1 <= age_day <= 7:
        return {
            "T_min": 30.0, "T_max": 34.0,
            "RH_min": 60.0, "RH_max": 70.0,
            "CO2_warning": 1400.0, "CO2_strong": 2200.0, "CO2_emergency": 3000.0,
            "NH3_warning": 3.0, "NH3_strong": 6.0, "NH3_emergency": 10.0
        }
    elif 8 <= age_day <= 14:
        return {
            "T_min": 28.0, "T_max": 32.0,
            "RH_min": 58.0, "RH_max": 68.0,
            "CO2_warning": 2000.0, "CO2_strong": 2600.0, "CO2_emergency": 3200.0,
            "NH3_warning": 5.0, "NH3_strong": 8.0, "NH3_emergency": 12.0
        }
    elif 15 <= age_day <= 21:
        return {
            "T_min": 25.0, "T_max": 29.0,
            "RH_min": 55.0, "RH_max": 67.0,
            "CO2_warning": 2000.0, "CO2_strong": 2600.0, "CO2_emergency": 3200.0,
            "NH3_warning": 8.0, "NH3_strong": 10.0, "NH3_emergency": 15.0
        }
    elif 22 <= age_day <= 28:
        return {
            "T_min": 22.0, "T_max": 26.0,
            "RH_min": 50.0, "RH_max": 65.0,
            "CO2_warning": 3000.0, "CO2_strong": 3800.0, "CO2_emergency": 4500.0,
            "NH3_warning": 10.0, "NH3_strong": 15.0, "NH3_emergency": 20.0
        }
    elif 29 <= age_day <= 35:
        return {
            "T_min": 22.0, "T_max": 25.0,
            "RH_min": 50.0, "RH_max": 65.0,
            "CO2_warning": 3000.0, "CO2_strong": 3800.0, "CO2_emergency": 4500.0,
            "NH3_warning": 10.0, "NH3_strong": 15.0, "NH3_emergency": 20.0
        }
    else:
        return {
            "T_min": 22.0, "T_max": 25.0,
            "RH_min": 50.0, "RH_max": 65.0,
            "CO2_warning": 3000.0, "CO2_strong": 3800.0, "CO2_emergency": 4500.0,
            "NH3_warning": 10.0, "NH3_strong": 15.0, "NH3_emergency": 20.0
        }
```

## Step 4: Summer Weather Simulation

This function simulates outdoor weather based on the timestamp (`ts`) and flock style. It calculates temperature, humidity, and solar factor with variations based on the flock's style, using sinusoidal patterns and random noise.

```py
def outside_weather(ts: pd.Timestamp, flock_style: str, rng: np.random.Generator) -> dict:
    hour = ts.hour + ts.minute / 60.0
    doy = ts.dayofyear

    # base summer profile
    temp_mean = 31.2
    temp_amp = 6.0
    rh_mean = 68.5
    rh_amp = 9.5

    # style offsets
    if flock_style == "normal":
        temp_offset = 0.0
        rh_offset = 0.0
    elif flock_style == "cool_brooding":
        temp_offset = -0.8
        rh_offset = 0.3
    elif flock_style == "heat_stress":
        temp_offset = 1.2
        rh_offset = -0.5
    elif flock_style == "gas_stress":
        temp_offset = 0.2
        rh_offset = 1.0
    else:
        temp_offset = 0.0
        rh_offset = 0.0

    slow_drift = 0.5 * np.sin(2 * np.pi * doy / 8.0)

    outside_temp = (
        temp_mean
        + temp_offset
        + slow_drift
        + temp_amp * np.sin(2 * np.pi * (hour - 9) / 24.0)
        + rng.normal(0, 0.30)
    )

    outside_humidity = (
        rh_mean
        + rh_offset
        - rh_amp * np.sin(2 * np.pi * (hour - 9) / 24.0)
        + rng.normal(0, 0.9)
    )

    # extra cool night tendency for brooding-friendly flocks
    if flock_style == "cool_brooding" and 0 <= hour <= 5:
        outside_temp -= 0.8

    # extra hot afternoon tendency
    if flock_style == "heat_stress" and 12 <= hour <= 16:
        outside_temp += 0.8

    solar_factor = max(0.0, np.sin(2 * np.pi * (hour - 6) / 24.0))

    return {
        "outside_temp": float(outside_temp),
        "outside_humidity": float(np.clip(outside_humidity, 40, 95)),
        "solar_factor": float(solar_factor),
    }
```

## Step 5: Physical Coefficients

This block defines coefficients for environmental factors, bird metabolism, and actuator efficiency in the simulation. They include constants for temperature, humidity, CO₂, and NH₃ generation and removal, as well as actuator performance and noise factors for each parameter.

```py
COEFFS = {
    "k_out_T": 0.11,
    "k_out_RH": 0.09,
    "k_out_CO2": 0.025,

    "bird_heat_base": 0.010,
    "bird_heat_slope": 0.060,

    "bird_moist_base": 0.040,
    "bird_moist_slope": 0.120,
    "litter_moist_gain": 0.080,

    "co2_gen_base": 54.0,
    "co2_gen_slope": 130.0,

    "nh3_gen_base": 0.07,
    "nh3_gen_slope": 0.26,
    "nh3_gen_quad": 0.42,

    "heat_gain": 1.20,
    "chill_cool": 0.42,
    "exhaust_cool": 0.66,

    "chill_rh_drop": 0.40,
    "exhaust_rh_drop": 0.75,

    "chill_co2_remove": 34.0,
    "exhaust_co2_remove": 80.0,
    "both_co2_bonus": 18.0,

    "chill_nh3_remove": 0.34,
    "exhaust_nh3_remove": 0.72,
    "both_nh3_bonus": 0.16,

    "solar_gain": 0.60,

    "nh3_rh_amp": 0.06,
    "nh3_temp_amp": 0.025,

    "temp_noise_sd": 0.07,
    "rh_noise_sd": 0.40,
    "co2_noise_sd": 12.0,
    "nh3_noise_sd": 0.06,
}
```

## Step 6: Mode-First Controller

This block defines the controller logic for determining the poultry house mode based on environmental conditions. It uses the current state (temperature, humidity, CO₂, NH₃) and a profile to decide the appropriate mode (e.g., "HEAT", "VENTILATE_LIGHT", "GAS_EMERGENCY"). The controller then adjusts actuators (heating light, chilling fan, exhaust fan) accordingly.

```py
def mode_to_actuators(mode: str):
    if mode == "IDLE":
        return 0, 0, 0
    elif mode == "HEAT":
        return 1, 0, 0
    elif mode == "VENTILATE_LIGHT":
        return 0, 1, 0
    elif mode in ("VENTILATE_STRONG", "GAS_EMERGENCY"):
        return 0, 1, 1
    else:
        raise ValueError(f"Unknown mode: {mode}")


def desired_mode_from_state(state: dict, profile: dict):
    T = state["temp"]
    RH = state["humidity"]
    CO2 = state["co2"]
    NH3 = state["nh3"]

    T_min = profile["T_min"]
    T_max = profile["T_max"]
    RH_max = profile["RH_max"]

    co2_warning = profile["CO2_warning"]
    co2_strong = profile["CO2_strong"]
    co2_emergency = profile["CO2_emergency"]

    nh3_warning = profile["NH3_warning"]
    nh3_strong = profile["NH3_strong"]
    nh3_emergency = profile["NH3_emergency"]

    # margins
    hT_strong = 1.0
    hT_light = 0.50
    hRH_extreme = 8.0

    # 1. emergency
    if CO2 >= co2_emergency or NH3 >= nh3_emergency:
        return "GAS_EMERGENCY", "GAS_EMERGENCY"

    # 2. strong ventilation
    if (
        T > T_max + hT_strong
        or CO2 >= co2_strong
        or NH3 >= nh3_strong
        or (RH > RH_max + hRH_extreme and (T > T_max or NH3 >= nh3_warning))
    ):
        if T > T_max + hT_strong:
            return "VENTILATE_STRONG", "HIGH_TEMP"
        elif CO2 >= co2_strong:
            return "VENTILATE_STRONG", "HIGH_CO2"
        elif NH3 >= nh3_strong:
            return "VENTILATE_STRONG", "HIGH_NH3"
        else:
            return "VENTILATE_STRONG", "EXTREME_RH_WITH_TEMP_OR_GAS"

    # 3. light ventilation
    if (
        T > T_max + hT_light
        or CO2 >= co2_warning
        or NH3 >= nh3_warning
    ):
        if T > T_max + hT_light:
            return "VENTILATE_LIGHT", "TEMP_ABOVE_TARGET"
        elif CO2 >= co2_warning:
            return "VENTILATE_LIGHT", "CO2_WARNING"
        else:
            return "VENTILATE_LIGHT", "NH3_WARNING"

    # 4. heating
    if T < T_min - 0.25:
        return "HEAT", "LOW_TEMP"

    # 5. idle
    return "IDLE", "NORMAL_RANGE"


def teacher_controller(state: dict, profile: dict, prev_mode: str, mode_steps: int, min_mode_steps: int = 5):
    desired_mode, reason_code = desired_mode_from_state(state, profile)

    if desired_mode == "GAS_EMERGENCY":
        final_mode = desired_mode
        new_mode_steps = 1 if prev_mode != final_mode else mode_steps + 1
    else:
        if prev_mode != desired_mode and mode_steps < min_mode_steps:
            final_mode = prev_mode
            new_mode_steps = mode_steps + 1
        else:
            final_mode = desired_mode
            new_mode_steps = 1 if prev_mode != final_mode else mode_steps + 1

    heating_light, chilling_fan, exhaust_fan = mode_to_actuators(final_mode)

    return {
        "mode": final_mode,
        "heating_light": int(heating_light),
        "chilling_fan": int(chilling_fan),
        "exhaust_fan": int(exhaust_fan),
        "emergency_flag": int(final_mode == "GAS_EMERGENCY"),
        "reason_code": reason_code,
        "mode_steps": new_mode_steps
    }
```

## Step 7: Style-Dependent Events

This function models environmental disturbances based on the flock's style (e.g., normal, cool_brooding, heat_stress) and age. It generates random events such as heat spikes, poor ventilation, gas buildup, and cleaning, which affect temperature, humidity, CO₂, and NH₃ levels. The active event type and its duration are tracked and updated based on probabilities.

The function returns:

- **Disturbance values** (`dT`, `dRH`, `dCO2`, `dNH3`)
- **Event type** (`event_code`)

```py
def disturbance_terms(ts: pd.Timestamp, age_day: int, flock_style: str,
                      rng: np.random.Generator, active_event: dict):
    hour = ts.hour
    age_factor = min(1.0, age_day / 42.0)
    early_age = 1.0 if age_day <= 14 else 0.0

    dT = rng.normal(0, 0.035)
    dRH = rng.normal(0, 0.18)
    dCO2 = rng.normal(0, 6.0)
    dNH3 = rng.normal(0, 0.03)
    cleaning_event = 0.0
    event_code = "NONE"

    if flock_style == "normal":
        heat_prob = 0.010 if 12 <= hour <= 16 else 0.0
        poor_vent_prob = 0.004 + 0.004 * age_factor
        litter_prob = 0.004 + 0.006 * age_factor
        emergency_prob = 0.0005 + 0.001 * age_factor
        cleaning_prob = 0.003
        cool_night_prob = 0.015 if (0 <= hour <= 5 and early_age == 1.0) else 0.0
        recovery_prob = 0.025

    elif flock_style == "cool_brooding":
        heat_prob = 0.008 if 12 <= hour <= 16 else 0.0
        poor_vent_prob = 0.003 + 0.003 * age_factor
        litter_prob = 0.003 + 0.005 * age_factor
        emergency_prob = 0.0003 + 0.0007 * age_factor
        cleaning_prob = 0.003
        cool_night_prob = 0.050 if (0 <= hour <= 5 and early_age == 1.0) else 0.0
        recovery_prob = 0.020

    elif flock_style == "heat_stress":
        heat_prob = 0.040 if 11 <= hour <= 17 else 0.0
        poor_vent_prob = 0.006 + 0.006 * age_factor
        litter_prob = 0.004 + 0.006 * age_factor
        emergency_prob = 0.0008 + 0.001 * age_factor
        cleaning_prob = 0.002
        cool_night_prob = 0.010 if (0 <= hour <= 5 and early_age == 1.0) else 0.0
        recovery_prob = 0.010

    elif flock_style == "gas_stress":
        heat_prob = 0.010 if 12 <= hour <= 16 else 0.0
        poor_vent_prob = 0.014 + 0.012 * age_factor
        litter_prob = 0.012 + 0.015 * age_factor
        emergency_prob = 0.0015 + 0.0020 * age_factor
        cleaning_prob = 0.004
        cool_night_prob = 0.012 if (0 <= hour <= 5 and early_age == 1.0) else 0.0
        recovery_prob = 0.010

    else:
        raise ValueError(f"Unknown flock_style: {flock_style}")

    if active_event["remaining"] <= 0:
        active_event = {"type": "NONE", "remaining": 0}
        p = rng.random()

        c1 = heat_prob
        c2 = c1 + poor_vent_prob
        c3 = c2 + litter_prob
        c4 = c3 + emergency_prob
        c5 = c4 + cleaning_prob
        c6 = c5 + cool_night_prob
        c7 = c6 + recovery_prob

        if p < c1:
            active_event = {"type": "HEAT_SPIKE", "remaining": int(rng.integers(6, 16))}
        elif p < c2:
            active_event = {"type": "POOR_VENTILATION", "remaining": int(rng.integers(8, 18))}
        elif p < c3:
            active_event = {"type": "LITTER_GAS_BUILDUP", "remaining": int(rng.integers(10, 24))}
        elif p < c4:
            active_event = {"type": "GAS_EMERGENCY_SCENARIO", "remaining": int(rng.integers(4, 8))}
        elif p < c5:
            active_event = {"type": "CLEANING_EVENT", "remaining": int(rng.integers(2, 4))}
        elif p < c6:
            active_event = {"type": "COOL_NIGHT_DRAFT", "remaining": int(rng.integers(6, 12))}
        elif p < c7:
            active_event = {"type": "NORMAL_RECOVERY", "remaining": int(rng.integers(8, 20))}

    if active_event["type"] == "HEAT_SPIKE":
        dT += rng.uniform(0.5, 1.0)
        event_code = "HEAT_SPIKE"

    elif active_event["type"] == "POOR_VENTILATION":
        dCO2 += rng.uniform(55, 120)
        dRH += rng.uniform(0.3, 1.2)
        event_code = "POOR_VENTILATION"

    elif active_event["type"] == "LITTER_GAS_BUILDUP":
        dNH3 += rng.uniform(0.5, 1.4)
        dCO2 += rng.uniform(20, 60)
        dRH += rng.uniform(0.6, 1.6)
        event_code = "LITTER_GAS_BUILDUP"

    elif active_event["type"] == "GAS_EMERGENCY_SCENARIO":
        dCO2 += rng.uniform(220, 480)
        dNH3 += rng.uniform(1.5, 3.5)
        dRH += rng.uniform(0.5, 1.5)
        event_code = "GAS_EMERGENCY_SCENARIO"

    elif active_event["type"] == "CLEANING_EVENT":
        cleaning_event = -rng.uniform(1.0, 2.5)
        dRH -= rng.uniform(0.4, 1.0)
        event_code = "CLEANING_EVENT"

    elif active_event["type"] == "COOL_NIGHT_DRAFT":
        dT -= rng.uniform(1.1, 2.2)
        dRH += rng.uniform(0.2, 0.9)
        event_code = "COOL_NIGHT_DRAFT"

    elif active_event["type"] == "NORMAL_RECOVERY":
        dT += rng.uniform(-0.15, 0.15)
        dRH += rng.uniform(-0.3, 0.3)
        dCO2 += rng.uniform(-10, 5)
        dNH3 += rng.uniform(-0.2, 0.1)
        event_code = "NORMAL_RECOVERY"

    active_event["remaining"] -= 1

    return {
        "dT": float(dT),
        "dRH": float(dRH),
        "dCO2": float(dCO2),
        "dNH3": float(dNH3),
        "cleaning_event": float(cleaning_event),
        "event_code": event_code
    }, active_event
```

## Step 8: State Update

This function updates the environmental state (temperature, humidity, CO₂, NH₃) based on the current weather, control actions, and disturbances. It uses coefficients to calculate the effects of bird metabolism, weather conditions, and actuator operations, while incorporating noise and disturbance factors. The function returns the updated state values for the next time step.

```py
def update_state(state: dict, weather: dict, ctrl: dict, age_day: int, flock_days: int,
                 flock_style: str, rng: np.random.Generator, disturb: dict, co2_outdoor: float = 420.0):

    z = (age_day - 1) / max(1, flock_days - 1)

    T = state["temp"]
    RH = state["humidity"]
    CO2 = state["co2"]
    NH3 = state["nh3"]

    u_heat = ctrl["heating_light"]
    u_chill = ctrl["chilling_fan"]
    u_exh = ctrl["exhaust_fan"]

    Tout = weather["outside_temp"]
    RHout = weather["outside_humidity"]
    solar = weather["solar_factor"]

    if flock_style == "normal":
        gas_scale = 0.62
        heat_scale = 0.95
    elif flock_style == "cool_brooding":
        gas_scale = 0.58
        heat_scale = 0.90
    elif flock_style == "heat_stress":
        gas_scale = 0.72
        heat_scale = 1.05
    elif flock_style == "gas_stress":
        gas_scale = 0.90
        heat_scale = 1.00
    else:
        gas_scale = 1.0
        heat_scale = 1.0

    bird_heat = (COEFFS["bird_heat_base"] + COEFFS["bird_heat_slope"] * z) * heat_scale
    T_next = (
        T
        + COEFFS["k_out_T"] * (Tout - T)
        + bird_heat
        + COEFFS["solar_gain"] * solar
        + COEFFS["heat_gain"] * u_heat
        - COEFFS["chill_cool"] * u_chill
        - COEFFS["exhaust_cool"] * u_exh
        + rng.normal(0, COEFFS["temp_noise_sd"])
        + disturb["dT"]
    )

    bird_moist = COEFFS["bird_moist_base"] + COEFFS["bird_moist_slope"] * z
    litter_moist = COEFFS["litter_moist_gain"] * (z ** 1.10)
    RH_next = (
        RH
        + COEFFS["k_out_RH"] * (RHout - RH)
        + bird_moist
        + litter_moist
        - COEFFS["chill_rh_drop"] * u_chill
        - COEFFS["exhaust_rh_drop"] * u_exh
        + rng.normal(0, COEFFS["rh_noise_sd"])
        + disturb["dRH"]
    )

    co2_gen = (COEFFS["co2_gen_base"] + COEFFS["co2_gen_slope"] * z) * gas_scale
    co2_remove = (
        COEFFS["chill_co2_remove"] * u_chill
        + COEFFS["exhaust_co2_remove"] * u_exh
        + COEFFS["both_co2_bonus"] * (u_chill * u_exh)
    )
    CO2_next = (
        CO2
        + co2_gen
        - co2_remove
        - COEFFS["k_out_CO2"] * (CO2 - co2_outdoor)
        + rng.normal(0, COEFFS["co2_noise_sd"])
        + disturb["dCO2"]
    )

    nh3_gen = (
        COEFFS["nh3_gen_base"]
        + COEFFS["nh3_gen_slope"] * z
        + COEFFS["nh3_gen_quad"] * (z ** 2)
    ) * gas_scale
    nh3_amp = (
        COEFFS["nh3_rh_amp"] * max(0.0, RH - 65.0)
        + COEFFS["nh3_temp_amp"] * max(0.0, T - 28.0)
    ) * gas_scale
    nh3_remove = (
        COEFFS["chill_nh3_remove"] * u_chill
        + COEFFS["exhaust_nh3_remove"] * u_exh
        + COEFFS["both_nh3_bonus"] * (u_chill * u_exh)
    )
    NH3_next = (
        NH3
        + nh3_gen
        + nh3_amp
        - nh3_remove
        + disturb["cleaning_event"]
        + rng.normal(0, COEFFS["nh3_noise_sd"])
        + disturb["dNH3"]
    )

    return {
        "temp": float(np.clip(T_next, 18, 42)),
        "humidity": float(np.clip(RH_next, 40, 95)),
        "co2": float(np.clip(CO2_next, 350, 6000)),
        "nh3": float(np.clip(NH3_next, 0, 35)),
    }
```

## Step 9: Simulation

This function simulates the flock's environmental conditions over the specified number of days. It iterates through each time step, updating the state (temperature, humidity, CO₂, NH₃), applying weather and disturbance effects, and determining the appropriate control actions. The results are stored in a DataFrame with detailed information for each timestamp.

```py
def simulate_flock(config: SimConfig) -> pd.DataFrame:
    rng = np.random.default_rng(config.seed)
    n_steps = config.flock_days * 24 * (60 // config.step_minutes)
    timestamps = pd.date_range("2026-04-01", periods=n_steps, freq=f"{config.step_minutes}min")

    state = {
        "temp": config.temp_init,
        "humidity": config.rh_init,
        "co2": config.co2_init,
        "nh3": config.nh3_init
    }

    prev_mode = "IDLE"
    mode_steps = 999
    active_event = {"type": "NONE", "remaining": 0}

    rows = []
    steps_per_day = 24 * (60 // config.step_minutes)

    for i, ts in enumerate(timestamps):
        age_day = int(i // steps_per_day) + 1
        profile = age_profile(age_day)
        weather = outside_weather(ts, config.flock_style, rng)
        disturb, active_event = disturbance_terms(ts, age_day, config.flock_style, rng, active_event)

        ctrl = teacher_controller(state, profile, prev_mode, mode_steps, min_mode_steps=5)

        rows.append({
            "timestamp": ts,
            "coop_id": config.coop_id,
            "flock_id": config.flock_id,
            "scenario_id": config.scenario_id,
            "house_type": config.house_type,
            "season": config.season,
            "flock_style": config.flock_style,
            "age_day": age_day,

            "outside_temp": weather["outside_temp"],
            "outside_humidity": weather["outside_humidity"],
            "solar_factor": weather["solar_factor"],

            "temp": state["temp"],
            "humidity": state["humidity"],
            "co2": state["co2"],
            "nh3": state["nh3"],

            "T_min": profile["T_min"],
            "T_max": profile["T_max"],
            "RH_min": profile["RH_min"],
            "RH_max": profile["RH_max"],

            "CO2_warning": profile["CO2_warning"],
            "CO2_strong": profile["CO2_strong"],
            "CO2_emergency": profile["CO2_emergency"],

            "NH3_warning": profile["NH3_warning"],
            "NH3_strong": profile["NH3_strong"],
            "NH3_emergency": profile["NH3_emergency"],

            "mode": ctrl["mode"],
            "heating_light": ctrl["heating_light"],
            "chilling_fan": ctrl["chilling_fan"],
            "exhaust_fan": ctrl["exhaust_fan"],
            "emergency_flag": ctrl["emergency_flag"],
            "reason_code": ctrl["reason_code"],

            "event_code": disturb["event_code"]
        })

        state = update_state(
            state=state,
            weather=weather,
            ctrl=ctrl,
            age_day=age_day,
            flock_days=config.flock_days,
            flock_style=config.flock_style,
            rng=rng,
            disturb=disturb,
            co2_outdoor=config.co2_outdoor
        )

        prev_mode = ctrl["mode"]
        mode_steps = ctrl["mode_steps"]

    return pd.DataFrame(rows)
```

## Step 10: Feature Engineering + Export

This block performs feature engineering on the simulated poultry data, adding lag features, rolling statistics, and custom calculations for environmental parameters (e.g., temperature, humidity, CO₂, NH₃). It also prepares the data for machine learning by creating separate dataframes for raw data, engineered features, control labels, and forecast targets.

The function includes:

- **Feature engineering**: Adding lag, rolling mean, and standard deviation for key parameters.
- **Label preparation**: Extracting control modes and actuator data for supervised learning.
- **Export**: Saving the processed data into CSV files and zipping them for easy storage and sharing.

```py
def generate_multiple_flocks():
    configs = [
        SimConfig(flock_id="FLOCK_001", coop_id="COOP_B1", scenario_id="SUMMER_SEMI_NORMAL_1", seed=42, flock_style="normal",        temp_init=29.8, rh_init=62.0, co2_init=780.0, nh3_init=0.8),
        SimConfig(flock_id="FLOCK_002", coop_id="COOP_B2", scenario_id="SUMMER_SEMI_NORMAL_2", seed=43, flock_style="normal",        temp_init=30.2, rh_init=63.0, co2_init=820.0, nh3_init=1.0),

        SimConfig(flock_id="FLOCK_003", coop_id="COOP_B3", scenario_id="SUMMER_SEMI_COOL_1",   seed=44, flock_style="cool_brooding", temp_init=28.6, rh_init=62.5, co2_init=760.0, nh3_init=0.8),
        SimConfig(flock_id="FLOCK_004", coop_id="COOP_B4", scenario_id="SUMMER_SEMI_COOL_2",   seed=45, flock_style="cool_brooding", temp_init=28.9, rh_init=61.8, co2_init=780.0, nh3_init=0.9),

        SimConfig(flock_id="FLOCK_005", coop_id="COOP_B5", scenario_id="SUMMER_SEMI_HEAT_1",   seed=46, flock_style="heat_stress",   temp_init=31.2, rh_init=64.5, co2_init=900.0, nh3_init=1.2),
        SimConfig(flock_id="FLOCK_006", coop_id="COOP_B6", scenario_id="SUMMER_SEMI_GAS_1",    seed=47, flock_style="gas_stress",    temp_init=30.6, rh_init=64.0, co2_init=950.0, nh3_init=1.3),
    ]

    raw_df = pd.concat([simulate_flock(cfg) for cfg in configs], ignore_index=True)
    raw_df["timestamp"] = pd.to_datetime(raw_df["timestamp"])

    feat = raw_df.copy().sort_values(["flock_id", "timestamp"]).reset_index(drop=True)

    for col in ["temp", "humidity", "co2", "nh3"]:
        for lag in [1, 2, 3, 6, 12]:
            feat[f"{col}_lag_{lag}"] = feat.groupby("flock_id")[col].shift(lag)

        feat[f"{col}_roll_mean_6"] = feat.groupby("flock_id")[col].transform(lambda s: s.rolling(6).mean())
        feat[f"{col}_roll_std_6"] = feat.groupby("flock_id")[col].transform(lambda s: s.rolling(6).std())
        feat[f"{col}_slope_6"] = feat.groupby("flock_id")[col].transform(lambda s: s.diff(6) / 6.0)

    feat["temp_above_tmax"] = feat["temp"] - feat["T_max"]
    feat["temp_below_tmin"] = feat["T_min"] - feat["temp"]
    feat["rh_above_rhmax"] = feat["humidity"] - feat["RH_max"]
    feat["co2_above_warning"] = feat["co2"] - feat["CO2_warning"]
    feat["nh3_above_warning"] = feat["nh3"] - feat["NH3_warning"]

    feat["hour"] = feat["timestamp"].dt.hour + feat["timestamp"].dt.minute / 60.0
    feat["hour_sin"] = np.sin(2 * np.pi * feat["hour"] / 24.0)
    feat["hour_cos"] = np.cos(2 * np.pi * feat["hour"] / 24.0)

    feat["prev_mode"] = feat.groupby("flock_id")["mode"].shift(1)
    feat["prev_heat"] = feat.groupby("flock_id")["heating_light"].shift(1)
    feat["prev_chill"] = feat.groupby("flock_id")["chilling_fan"].shift(1)
    feat["prev_exhaust"] = feat.groupby("flock_id")["exhaust_fan"].shift(1)

    feat["sensor_quality_flags"] = 0

    feature_df = feat.dropna().reset_index(drop=True)

    label_df = raw_df[
        ["timestamp", "coop_id", "flock_id", "scenario_id", "age_day",
         "mode", "heating_light", "chilling_fan", "exhaust_fan",
         "emergency_flag", "reason_code"]
    ].copy()

    forecast_df = raw_df.copy().sort_values(["flock_id", "timestamp"]).reset_index(drop=True)
    for col in ["temp", "humidity", "co2", "nh3"]:
        forecast_df[f"{col}_t_plus_30"] = forecast_df.groupby("flock_id")[col].shift(-6)
    forecast_df = forecast_df.dropna().reset_index(drop=True)

    return raw_df, feature_df, label_df, forecast_df


def export_outputs(raw_df, feature_df, label_df, forecast_df):
    raw_path = OUTPUT_DIR / "raw_sensor_table.csv"
    feat_path = OUTPUT_DIR / "derived_feature_table.csv"
    label_path = OUTPUT_DIR / "control_label_table.csv"
    forecast_path = OUTPUT_DIR / "forecast_target_table.csv"

    raw_df.to_csv(raw_path, index=False)
    feature_df.to_csv(feat_path, index=False)
    label_df.to_csv(label_path, index=False)
    forecast_df.to_csv(forecast_path, index=False)

    summary = {
        "description": "Summer + semi_closed only synthetic poultry dataset",
        "house_type": "semi_closed",
        "season": "summer",
        "main_variables": ["temp", "humidity", "co2", "nh3"],
        "actuators": ["heating_light", "chilling_fan", "exhaust_fan"],
        "decision_modes": ["HEAT", "IDLE", "VENTILATE_LIGHT", "VENTILATE_STRONG", "GAS_EMERGENCY"],
        "step_minutes": 5
    }

    with open(OUTPUT_DIR / "dataset_summary.json", "w") as f:
        json.dump(summary, f, indent=2)

    with zipfile.ZipFile(OUTPUT_DIR / "synthetic_poultry_dataset.zip", "w", zipfile.ZIP_DEFLATED) as zf:
        for fname in [
            "raw_sensor_table.csv",
            "derived_feature_table.csv",
            "control_label_table.csv",
            "forecast_target_table.csv",
            "dataset_summary.json"
        ]:
            zf.write(OUTPUT_DIR / fname, arcname=fname)


def quick_summary(raw_df, label_df):
    print("=== RAW DATA SUMMARY ===")
    print(raw_df[["temp", "humidity", "co2", "nh3"]].describe())

    print("\n=== MODE DISTRIBUTION ===")
    print(label_df["mode"].value_counts(normalize=True).round(4))

    print("\n=== FLOCK COUNTS ===")
    print(raw_df["flock_id"].value_counts().sort_index())


def plot_environment(raw_df, flock_id="FLOCK_001", days_to_plot=3):
    df = raw_df[raw_df["flock_id"] == flock_id].copy().sort_values("timestamp")
    df = df.iloc[:days_to_plot * 24 * 12]

    fig, axes = plt.subplots(4, 1, figsize=(18, 14), sharex=True)

    axes[0].plot(df["timestamp"], df["temp"], label="Inside Temp")
    axes[0].plot(df["timestamp"], df["outside_temp"], label="Outside Temp", alpha=0.7)
    axes[0].plot(df["timestamp"], df["T_min"], "--", label="T_min")
    axes[0].plot(df["timestamp"], df["T_max"], "--", label="T_max")
    axes[0].legend(); axes[0].grid(alpha=0.3); axes[0].set_ylabel("Temp")

    axes[1].plot(df["timestamp"], df["humidity"], label="Inside RH")
    axes[1].plot(df["timestamp"], df["outside_humidity"], label="Outside RH", alpha=0.7)
    axes[1].plot(df["timestamp"], df["RH_min"], "--", label="RH_min")
    axes[1].plot(df["timestamp"], df["RH_max"], "--", label="RH_max")
    axes[1].legend(); axes[1].grid(alpha=0.3); axes[1].set_ylabel("RH")

    axes[2].plot(df["timestamp"], df["co2"], label="CO2")
    axes[2].plot(df["timestamp"], df["CO2_warning"], "--", label="CO2 warn")
    axes[2].plot(df["timestamp"], df["CO2_strong"], "--", label="CO2 strong")
    axes[2].legend(); axes[2].grid(alpha=0.3); axes[2].set_ylabel("CO2")

    axes[3].plot(df["timestamp"], df["nh3"], label="NH3")
    axes[3].plot(df["timestamp"], df["NH3_warning"], "--", label="NH3 warn")
    axes[3].plot(df["timestamp"], df["NH3_strong"], "--", label="NH3 strong")
    axes[3].legend(); axes[3].grid(alpha=0.3); axes[3].set_ylabel("NH3")

    plt.tight_layout()
    plt.show()


def plot_control(raw_df, flock_id="FLOCK_001", days_to_plot=3):
    df = raw_df[raw_df["flock_id"] == flock_id].copy().sort_values("timestamp")
    df = df.iloc[:days_to_plot * 24 * 12]

    mode_map = {"IDLE": 0, "HEAT": 1, "VENTILATE_LIGHT": 2, "VENTILATE_STRONG": 3, "GAS_EMERGENCY": 4}

    fig, axes = plt.subplots(4, 1, figsize=(18, 10), sharex=True)

    axes[0].step(df["timestamp"], df["heating_light"], where="post")
    axes[0].set_ylabel("Heat"); axes[0].grid(alpha=0.3)

    axes[1].step(df["timestamp"], df["chilling_fan"], where="post")
    axes[1].set_ylabel("Chill"); axes[1].grid(alpha=0.3)

    axes[2].step(df["timestamp"], df["exhaust_fan"], where="post")
    axes[2].set_ylabel("Exhaust"); axes[2].grid(alpha=0.3)

    axes[3].step(df["timestamp"], df["mode"].map(mode_map), where="post")
    axes[3].set_yticks(list(mode_map.values()))
    axes[3].set_yticklabels(list(mode_map.keys()))
    axes[3].set_ylabel("Mode"); axes[3].grid(alpha=0.3)

    plt.tight_layout()
    plt.show()
```

## Step 11: Validator

This function validates the generated dataset by checking for consistency and coverage. It verifies the raw data for duplicates, confirms that control modes match actuator settings, and ensures adequate coverage of modes and gas warnings. The results are summarized in a validation report, with a final decision on dataset acceptance.

The function returns:

- A validation report
- A final decision on the dataset's quality

```py
def validate():
    raw_file = OUTPUT_DIR / "raw_sensor_table.csv"
    feat_file = OUTPUT_DIR / "derived_feature_table.csv"
    label_file = OUTPUT_DIR / "control_label_table.csv"
    forecast_file = OUTPUT_DIR / "forecast_target_table.csv"

    raw = pd.read_csv(raw_file)
    feat = pd.read_csv(feat_file)
    labels = pd.read_csv(label_file)
    forecast = pd.read_csv(forecast_file)

    raw["timestamp"] = pd.to_datetime(raw["timestamp"])
    feat["timestamp"] = pd.to_datetime(feat["timestamp"])
    labels["timestamp"] = pd.to_datetime(labels["timestamp"])
    forecast["timestamp"] = pd.to_datetime(forecast["timestamp"])

    rows = []

    def add(section, check, status, details):
        rows.append({"section": section, "check": check, "status": status, "details": details})

    # RAW
    add("RAW", "Only summer present", "PASS" if set(raw["season"].unique()) == {"summer"} else "FAIL", str(set(raw["season"].unique())))
    add("RAW", "Only semi_closed present", "PASS" if set(raw["house_type"].unique()) == {"semi_closed"} else "FAIL", str(set(raw["house_type"].unique())))
    add("RAW", "No duplicate flock/timestamp rows",
        "PASS" if raw.duplicated(subset=["flock_id", "timestamp"]).sum() == 0 else "FAIL",
        f"duplicates={raw.duplicated(subset=['flock_id', 'timestamp']).sum()}")

    # CONTROL
    def check_row(r):
        m = r["mode"]
        h, c, e = int(r["heating_light"]), int(r["chilling_fan"]), int(r["exhaust_fan"])
        if m == "IDLE":
            return (h, c, e) == (0, 0, 0)
        if m == "HEAT":
            return (h, c, e) == (1, 0, 0)
        if m == "VENTILATE_LIGHT":
            return (h, c, e) == (0, 1, 0)
        if m in ("VENTILATE_STRONG", "GAS_EMERGENCY"):
            return (h, c, e) == (0, 1, 1)
        return False

    inconsistent = (~labels.apply(check_row, axis=1)).sum()
    add("CONTROL", "Mode-actuator consistency", "PASS" if inconsistent == 0 else "FAIL", f"inconsistent_rows={inconsistent}")

    mode_share = labels["mode"].value_counts(normalize=True).to_dict()

    # scenario-specific thresholds for summer + semi-closed only
    add("CONTROL", "Enough IDLE coverage", "PASS" if mode_share.get("IDLE", 0) >= 0.03 else "FAIL", f"share={mode_share.get('IDLE', 0):.4f}")
    add("CONTROL", "Enough HEAT coverage", "PASS" if mode_share.get("HEAT", 0) >= 0.015 else "FAIL", f"share={mode_share.get('HEAT', 0):.4f}")
    add("CONTROL", "Enough VENTILATE_LIGHT coverage", "PASS" if mode_share.get("VENTILATE_LIGHT", 0) >= 0.03 else "FAIL", f"share={mode_share.get('VENTILATE_LIGHT', 0):.4f}")
    add("CONTROL", "Enough VENTILATE_STRONG coverage", "PASS" if mode_share.get("VENTILATE_STRONG", 0) >= 0.03 else "FAIL", f"share={mode_share.get('VENTILATE_STRONG', 0):.4f}")

    emergency_share = mode_share.get("GAS_EMERGENCY", 0)
    add("CONTROL", "Emergency coverage present", "PASS" if emergency_share >= 0.0003 else "WARN", f"share={emergency_share:.4f}")

    # gas coverage
    co2_warn_count = int((raw["co2"] >= raw["CO2_warning"]).sum())
    nh3_warn_count = int((raw["nh3"] >= raw["NH3_warning"]).sum())
    add("COVERAGE", "CO2 warning coverage", "PASS" if co2_warn_count >= 30 else "FAIL", f"count={co2_warn_count}")
    add("COVERAGE", "NH3 warning coverage", "PASS" if nh3_warn_count >= 30 else "FAIL", f"count={nh3_warn_count}")

    report_df = pd.DataFrame(rows)
    report_df.to_csv(OUTPUT_DIR / "validation_report.csv", index=False)

    n_fail = (report_df["status"] == "FAIL").sum()
    n_warn = (report_df["status"] == "WARN").sum()

    if n_fail == 0 and n_warn == 0:
        decision = "ACCEPT"
    elif n_fail == 0:
        decision = "ACCEPT WITH WARNINGS"
    else:
        decision = "REJECT / FIX GENERATOR"

    print("=== STATUS COUNTS ===")
    print(report_df["status"].value_counts().to_dict())
    print("\n=== FINAL DECISION ===")
    print(decision)

    return report_df, decision
```

## Step 12: Run Everything

This function runs the entire pipeline, generating the poultry data, summarizing it, plotting environmental and control data, exporting the results, and validating the dataset. It prints a summary of failed or warned rows and lists the saved files.

The function returns:

- The validation report and decision

```py
raw_df, feature_df, label_df, forecast_df = generate_multiple_flocks()

quick_summary(raw_df, label_df)

plot_environment(raw_df, flock_id="FLOCK_001", days_to_plot=3)
plot_control(raw_df, flock_id="FLOCK_001", days_to_plot=3)

export_outputs(raw_df, feature_df, label_df, forecast_df)

report_df, decision = validate()

print("\n=== FAIL / WARN ROWS ===")
cols = ["section", "check", "status", "details"]
display(report_df[report_df["status"].isin(["FAIL", "WARN"])][cols])

print("\nSaved files:")
for p in sorted(OUTPUT_DIR.iterdir()):
    print(p)
```

### Next Step: E-GRU Model Development

Once the synthetic data is generated, the next step is the development of the **E-GRU (Enhanced Gated Recurrent Unit)** model. This model will be trained on the generated dataset to predict future environmental conditions, such as temperature and humidity, with a 30-minute forecast horizon. The E-GRU model will help in making accurate predictions, assisting in the optimal control of the poultry house environment.
