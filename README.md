# CSCE 412 Project 3 – Load Balancer Simulation

## Overview

This project simulates a dynamic web load balancer in C++. Incoming requests are queued and distributed to a pool of web servers. The system automatically adds or removes servers based on queue depth, and blocks requests from specified IP ranges.

## Files

- main.cpp – Program entry point, handles user input and summary output
- `Config.h/cpp` – Loads simulation settings from config.txt
- `Request.h/cpp` – Defines the request struct and random request generation
- `WebServer.h/cpp` – Simulates individual web servers
- `IPBlocker.h/cpp` – Implements IP range blocking
- `LoadBalancer.h/cpp` – Core simulation logic, queue management, scaling, logging
- Makefile – Build, run, clean, and docs targets

## How to Build and Run

```bash
make           # builds the project
make run       # runs the simulation
make clean     # removes binaries and object files
make docs      # generates Doxygen documentation (requires doxygen)
```
Alternatively,
```bash
make                     # builds the project
./load_balancer_sim      # runs the simulation
```

## Configuration

Edit config.txt to set:
- `initialServers` – starting number of servers
- `simulationCycles` – number of cycles to run
- `initialQueueMultiplier` – initial queue size per server
- `scalingCooldownCycles` – cycles to wait between scaling events
- `minRequestTime` / `maxRequestTime` – request processing time range
- `blocked_ranges` – comma-separated list of blocked IPs/ranges (e.g. `10.0.0.0/8,192.168.1.1-192.168.1.20`)

## Output

- **Terminal:** Color-coded status, scaling, and block events
- **Log file:** Detailed event log (load_balancer.log)
- **Summary:** Printed at end of simulation and written to log

## Documentation

Run `make docs` to generate HTML documentation in the docs folder.

---

**Author:** Karan Bhagat  
**Course:** CSCE 412  
