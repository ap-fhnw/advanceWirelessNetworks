# Lab 01: Propagation Models

**Language Options:** Complete this lab in **C++ or Python** (choose one).  
**ns-3 version:** 3.40

---

## Objectives

By the end of this lab you will be able to:

1. Use Three ns-3 propagation‐loss models (Two-Ray, Cost231‐Hata, Friis).  
2. Measure application‐level throughput vs. distance in a two‐node WiFi link.  
3. Compare simulated path loss to real‐world measurements.  
4. Plot and analyze bit‐rate and path‐loss curves.  

---

## Prerequisites & Setup

See [common/setup.md](../common/setup.md) for installation and build instructions.  
Refer to [common/links.md](../common/links.md) for API and tutorial references.

---

## Part 1: Simulated Propagation Experiments

### Task (C++): Two-Ray Ground Model

![Two-Ray Ground Reflection Model](/common/images/twoRayGroundPropogationalLoss.png)  
*This visualization shows the power received under the Two-Ray Ground Reflection model, which accounts for both the direct line-of-sight path and the ground-reflected path between a transmitter and a receiver. The formula assumes constructive and destructive interference between the two rays and is especially suitable for long-range scenarios (beyond the crossover distance). NS-3 switches to this model beyond the Friis crossover point, and uses Friis for short distances due to oscillation artifacts at low ranges.*

![Two-Ray Ground Reflection Model](/common/images/500px-2-Ray_Ground_Reflection.png)  
*A geometric representation of the Two-Ray Ground Reflection model, showing the paths of the direct signal and the ground-reflected signal between transmitter and receiver. Key parameters include the horizontal distance, transmitter height, receiver height, reflection angles, and total path lengths. The figure demonstrates how constructive and destructive interference arises due to phase differences between the direct and reflected rays.*

1. **Define** distance set

   ```
   D = {dᵢ, 7dᵢ/8, 6dᵢ/8, …, dᵢ/8}
   ```
2. **For each** distance `d ∈ D`:

   * Edit `Lab1_Cpp_TwoRay.cc` to place nodes at `(0,0)` and `(d,0)`.
   * Run the simulation, capture UDP throughput.
   * Record bit-rate vs. distance.
3. **Plot** bit-rate (y-axis) vs. distance (x-axis).

**Likely Issues:**

* Mode name typo (`DsssRate5.5Mbps`): see [3.1 in common/troubleshooting.md](../common/troubleshooting.md#31-mode-name-typo).
* Zero throughput if no routing/mobility errors: see [2.3 in common/troubleshooting.md](../common/troubleshooting.md#23-zero-throughput-in-flowmonitor).

---

### Task (C++): Cost231-Hata Model
![COST231 Propagation Loss Model](/common/images/cost231PropogationLoss.png)  
*The COST231-Hata propagation loss model, a widely used empirical formula for predicting path loss in urban and suburban environments. The model extends the original Hata model to frequencies between 1500 and 2000 MHz, making it applicable to GSM and LTE networks. It includes corrections based on base station height, mobile antenna height, distance, frequency, and urban density. The formula supports environment-specific adjustments through correction factors for urban vs rural scenarios.*

Repeat steps 1–6 using `Lab1_Cpp_Cost231.cc` and the Cost231‐Hata model.

---

### Task (C++): Friis Model
![Friis Free-Space Propagation Model: Line-of-Sight Power Decay](/common/images/friisPropogationLossModel.png)  
  Friis propagation loss model, which calculates the received signal power under ideal free-space conditions (no obstacles or reflections). It is based on antenna gains, wavelength, and distance between transmitter and receiver. The model is only valid in the far-field region (typically when distance 𝑑>3𝜆/2𝜋), and breaks down at very short distances due to singularities.*

Repeat steps 1–6 using `Lab1_Cpp_Friis.cc` and the Friis model.

---

### Task (Python): Propagation Models

For each model above, repeat the experiments in Python:

1. **Copy** `code/Lab1_Py_TwoRay.py`, `Lab1_Py_Cost231.py`, `Lab1_Py_Friis.py` into `scratch/`.
2. **Ensure** `PYTHONPATH` is set:

   ```bash
   export PYTHONPATH=~/ns-allinone-3.40/ns-3.40/build/bindings:$PYTHONPATH
   ```
3. **Run** each script with distance parameter looping over `D`.
4. **Collect** and **plot** bit-rate vs. distance for all three models.

**Likely Issues:**

* Python binding import errors: see [1.3 in common/troubleshooting.md](../common/troubleshooting.md#13-importerror-no-module-named-nscore).
* Scheduler or File I/O errors: verify `scratch/` paths and permissions.

---

## Part 2: Real-World Propagation Measurements (**Optional**)

> **Recommended**: work with a partner and laptops in corridor.

### Task: Corridor RSSI & Path Loss

1. **Set up** an ad-hoc Wi-Fi link between two laptops.
2. **Measure** RSSI at distances 1 m, 2 m, … until at least 20 m (ping + Wireshark or RSSI tool).
3. **Compute** path loss (dB) = Tx power (dBm) − RSSI (dBm).
4. **Calculate** Friis path loss for the same distances.
5. **Plot** measured vs. Friis path loss on a single graph.
6. **Explain** discrepancies between measured and theoretical curves.

**Likely Issues:**

* Missing RSSI fields in Wireshark: enable Radiotap headers.
* Units confusion (dBm vs. mW): double-check conversions.

---

## Deliverables

See [`deliverables.md`](deliverables.md).

---

## Cross-References

* Shared setup: [common/setup.md](../common/setup.md)
* Troubleshooting: [common/troubleshooting.md](../common/troubleshooting.md)
* API & tutorial links: [common/links.md](../common/links.md)

---
