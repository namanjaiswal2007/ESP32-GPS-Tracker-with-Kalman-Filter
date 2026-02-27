# ESP32 GPS Tracker with Kalman Filter

## Overview
This project is a real-time GPS tracking system built using ESP32 and L89 GPS module.  
It sends live coordinate data to a ThingsBoard dashboard and applies Kalman filtering to reduce noise in GPS readings.

---

## Hardware Used
- ESP32
- L89 GPS Module
- LiPo Battery
- TP4056 Charging Module
- Connecting wires & breadboard

---

## Features
- Real-time GPS coordinate acquisition
- WiFi-based data transmission
- ThingsBoard dashboard visualization
- Kalman filter for coordinate smoothing
- Portable battery-powered system

---

## System Flow
GPS Module → ESP32 → WiFi → ThingsBoard → Live Route Map

---

## Filtering Strategy

- Rejects coordinates near (0,0)
- Rejects jumps > 50 meters (spike filtering)
- Freezes position if movement < 2 meters
- Applies Kalman filtering when stationary
- Sends updates only when movement threshold is crossed

---

## What I Learned
- Parsing NMEA GPS data
- Working with latitude & longitude
- Kalman filtering basics
- IoT dashboard integration
- Noise reduction challenges in real-world data

---

## Future Improvements
- Power optimization
- Sleep mode implementation
- Improved filtering model
