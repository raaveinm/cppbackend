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

## 5. Deployment
The server is deployed using Docker, leveraging a multi-stage build process:

### Build Stage
1.  **Base Image**: `ubuntu:22.04`
2.  **Dependencies**: Installs `g++`, `cmake`, `ninja-build`, `python3-pip`, `git`.
3.  **Conan**: Installs Conan (version 1.62.0) and uses `conan install` to fetch and build C++ dependencies.
4.  **CMake Configuration**: Configures the project with CMake using Ninja as the generator and Conan's toolchain file.
5.  **Build**: Compiles the `game_server` executable.

### Runtime Stage
1.  **Base Image**: `ubuntu:22.04`
2.  **Dependencies**: Installs `python3` and `python3-pip` for the web exporter.
3.  **Application Files**: Copies the compiled `game_server` executable, `data` directory, `static` directory, and `web_exporter.py` from the build stage.
4.  **Exposed Ports**: Exposes port `8080` for the game server and `9200` for the Prometheus web exporter.
5.  **Entrypoint**: The `CMD` executes the `game_server` with a configuration file, static content path, and a ticker interval, piping its output to the `web_exporter.py` script.

## 6. Monitoring
Monitoring is set up using Prometheus. The `prometheus.yml` configuration indicates:
*   **Scrape Interval**: Metrics are scraped every 30 seconds.
*   **Targets**:
    *   `node-exporter:9100`: For host-level metrics.
    *   `game-server:9200`: For application-specific metrics exposed by the `web_exporter.py` script.

The `web_exporter.py` script likely parses the game server's output (piped from `stdout`) and exposes relevant metrics in a Prometheus-compatible format on port 9200.
