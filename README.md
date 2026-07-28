# Game Server Design

*languages: [ru](README.ru.md)*

## 1. Project Overview
This project implements a C++ game server designed to handle game logic, manage player data, and serve HTTP requests. It includes functionalities for loading game configurations, processing client requests, and providing monitoring endpoints.

## 2. Technology Stack
*   **Language**: C++20
*   **Build System**: CMake
*   **Dependency Management**: Conan
*   **Networking**: Boost.Beast (implied by `http_server.cpp` and Boost dependency)
*   **Logging**: Boost.Log
*   **JSON Handling**: Boost.JSON
*   **Containerization**: Docker
*   **Monitoring**: Prometheus (with a Python web exporter)

## 3. Architecture
The game server is structured around several key components:
*   **HTTP Server (`net/http_server.h/.cpp`)**: Handles incoming HTTP connections and dispatches requests.
*   **Request Handler (`net/request_handler.h/.cpp`)**: Processes specific HTTP requests, interacting with the game model.
*   **Game Model (`model/model.h/.cpp`, `model/player.h`)**: Contains the core game logic, data structures for game entities (like players), and state management.
*   **JSON Loader (`loader/json_loader.h/.cpp`, `loader/boost_json.cpp`)**: Responsible for loading game configurations and data from JSON files using Boost.JSON.
*   **Utilities (`util/sdk.h`, `util/tagged.h`, `util/logging.h/.cpp`, `util/token_generator.h`, `util/ticker.h`)**: Provides common utilities such as logging, unique ID generation, and timing mechanisms.

## 4. Build System
The project uses CMake for its build system, managing the compilation of C++ source files and linking dependencies. Conan is integrated to handle third-party C++ dependencies, ensuring a consistent build environment.

## 5. Getting Started & Deployment

### Prerequisites
- [Docker](https://docs.docker.com/get-docker/) and [Docker Compose](https://docs.docker.com/compose/install/)

### Quick Start (Docker Compose)
To build and run the entire stack (Game Server, Prometheus, Grafana, Node Exporter) in detached mode:

```bash
docker compose up -d --build

```

To stop all services and remove containers:

```bash
docker compose down

```

### Services & Exposed Ports

Once the containers are up, the following endpoints are available:

| Service                 | Port   | Description                                     |
|-------------------------|--------|-------------------------------------------------|
| **Game Server**         | `8080` | Main HTTP API & static Web content              |
| **Prometheus Exporter** | `9200` | Application metrics parsed by `web_exporter.py` |
| **Prometheus**          | `9090` | Time-series database & metrics collection UI    |
| **Grafana**             | `3000` | Metrics visualization dashboards                |
| **Node Exporter**       | `9100` | Host hardware & OS metrics                      |

### Verification

Verify that the server is running properly by fetching the list of maps:

```bash
curl -X GET http://localhost:8080/api/v1/maps

```

---

### Local Build (Development)

If you want to build and run the executable natively on your host machine without Docker:

1. **Install Dependencies**:
```bash
pip install conan==1.62.0

```


2. **Configure & Build**:
```bash
conan install . --build=missing -s build_type=Debug -if build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build --target game_server

```


3. **Run Server**:
```bash
./build/game_server -c data/config.json -w static -t 50

```

## 6. Monitoring
Monitoring is set up using Prometheus. The `prometheus.yml` configuration indicates:
*   **Scrape Interval**: Metrics are scraped every 30 seconds.
*   **Targets**:
    *   `node-exporter:9100`: For host-level metrics.
    *   `game-server:9200`: For application-specific metrics exposed by the `web_exporter.py` script.

The `web_exporter.py` script parses the game server's output (piped from `stdout`) and exposes relevant metrics in a Prometheus-compatible format on port 9200.

```text
game_server/
 ├── CMakeLists.txt                      # Main CMake build file
 ├── CMakeUserPresets.json               # User CMake/Conan presets
 ├── Dockerfile                          # Multi-stage server image build
 ├── conan_clion_setup.cmake             # Automatic Conan integration script for CLion
 ├── conanfile.txt                       # C++ dependencies (Boost)
 ├── docker-compose.yml                  # Infrastructure orchestration (Server, Prometheus, Grafana, Node-Exporter)
 ├── prometheus.yml                      # Metric collection configuration for Prometheus
 ├── web_exporter.py                     # Python exporter for server stdout metrics to Prometheus
 ├── src/                                # C++ source code
 │   ├── main.cpp                        # Application entry point
 │   ├── loader/                         # Data loading module
 │   │   ├── boost_json.cpp              # Boost.JSON implementation setup
 │   │   ├── json_loader.cpp             # JSON config loader implementation
 │   │   └── json_loader.h               # Loader header file
 │   ├── model/                          # Game model and logic
 │   │   ├── model.cpp                   # Game objects and state implementation
 │   │   ├── model.h                     # Definitions of game entities and maps
 │   │   └── player.h                    # Player logic and state
 │   ├── net/                            # Network layer (Boost.Beast)
 │   │   ├── http_server.cpp             # Low-level HTTP server implementation
 │   │   ├── http_server.h               # Session and network connection handling
 │   │   ├── request_handler.cpp         # Routing and business logic request handling
 │   │   └── request_handler.h           # Request handler header file
 │   └── util/                           # Helper utilities
 │       ├── logging.cpp                 # Logging setup (Boost.Log)
 │       ├── logging.h                   # Logger and JSON log formatting
 │       ├── sdk.h                       # Platform-dependent / base macros
 │       ├── tagged.h                    # Strong typing helper (Strong Types)
 │       ├── ticker.h                    # Game timer/ticker with regular intervals
 │       └── token_generator.h           # Authorization token generation
```