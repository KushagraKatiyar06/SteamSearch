# SteamSearch: A C++ Game Recommendation Engine

Steam Search is a C++ based game recommendation web application which allows users to input a game and retrieve recommendations
for games based on six different algorithms, all favoring different values and data on games. There are around 100,000 games in the database dating to 2023.  

<<<<<<< HEAD
## Hosted on Google Cloud
Project is deployed on Google Cloud: https://steamsearch-58195528645.southamerica-east1.run.app/
- For Full functionality for the project -> All 6 algorithms and visualizations, run this project locally following the set
  up below.

## Key Features

* **Fast Recommendations**: Built on the lightweight **Crow C++ microframework** for high-performance web service handling.

* **Multiple Algorithms**:

  * Jaccard Similarity
  * Jaccards Similarity Weighted
  * Cosine Similarity (Vector Dot Product)
  * Min-Hashing (Approximation Technique)
  * Multi-Feature Weighted Scoring 
  * Decision Tree 
  
* **Fuzzy Search**: Fast, robust game searching with typo tolerance provided by the **RapidFuzz** library.

* **Frontend**: A single-page web interface (`index.html`) built on JavaScript, HTML and CSS.

* **Deployment Ready**: Supports self-contained local execution and single-command Docker deployment.

## Dataset

Due to its large file size, the primary dataset (`games.json`) is not included in this repository.


1. Download the dataset from Kaggle: [Steam Games Dataset](https://www.kaggle.com/datasets/fronkongames/steam-games-dataset).

2. Place the downloaded `games.json` file in the **root directory** of the project:

## Local Setup

### Prerequisites

* C++17 or newer compiler (MSVC on Windows, GCC/Clang on Linux)

* [CMake](https://cmake.org/download/) (v3.29+)

* [Git](https://git-scm.com/downloads)

* [Vcpkg](https://vcpkg.io/en/getting-started.html) (required for Crow and Asio dependency management)

## File Structure

```bash
SteamSearch/
├── README.md
  
├── games.json (Place this dataset after downloading from kaggle)
├── public/
│   └── index.html         
├── CMakeLists.txt         
├── Dockerfile              
├── tags.txt               
├── decoder.txt      
├── dataset.txt   
                
├── algorithms_B.h
├── algorithms_B.cpp
├── cosineSimilarity.h
├── cosineSimilarity.cpp
├── Game.h
├── Game.cpp
├── jaccardsSimilarity.h
├── jaccardsSimilarity.cpp
├── main.cpp
├── minHash.h
├── minHash.cpp
├── multiFeatureSimilarity.h
├── multiFeatureSimilarity.cpp
├── RapidFuzzle.h
├── RapidFuzzle.cpp
├── readJson.h
├── readJson.cpp

├── CLI.cpp (This makes the program only CLI, but it is required to adjust the CMake for it to work.)
```

### Setup Steps for Windows

```bash
# 1. Clone the main repository
git clone [https://github.com/](https://github.com/)<your-username>/SteamSearch.git
cd SteamSearch

# 2. Clone vcpkg and bootstrap the toolchain
git clone [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)
.\vcpkg\bootstrap-vcpkg.bat

# 3. Install core dependencies (Crow and Asio)
.\vcpkg\vcpkg.exe install crow asio

# 4. Configure and build the project with CMake
# Note: Ensure you are using the correct generator/build system if not using CLion's defaults.
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=.\vcpkg\scripts\buildsystems\vcpkg.cmake

cmake --build build --config Release

# 5. Run the application
# The executable is often placed in a 'Release' subdirectory on Windows.
.\build\Release\SteamSearch.exe
```


### Setup Steps for Ubuntu (Linux)

```bash
# 1. Clone the main repository
git clone [https://github.com/](https://github.com/)<your-username>/SteamSearch.git
cd SteamSearch

# 2. Clone vcpkg and bootstrap the toolchain
git clone [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)
./vcpkg/bootstrap-vcpkg.sh

# 3. Install core dependencies (Crow and Asio)
./vcpkg/vcpkg install crow asio

# 4. Configure and build the project with CMake
# The CMAKE_TOOLCHAIN_FILE links Vcpkg to your build process.
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release

# 5. Run the application
./build/SteamSearch
```

## Docker
The repository also contains a docker file which can package the entire application in an enviroment, installing 
all required dependencies. Ensure the games.json is in the root directory of the entire project before running
the docker file. 

```bash
# 1. Build the Docker image
# This may take several minutes as it installs Vcpkg and compiles all dependencies.
docker build -t steamsearch .

# 2. Run the container
# -p 8080:18080 maps the container's internal port (18080) to your host's port (8080).
docker run -p 8080:18080 steamsearch
```

=======
SteamSearch: A Game Recommendation Engine
Project Overview
SteamSearch is a C++-based web application that provides personalized video game recommendations. This project demonstrates and compares various data structures and algorithms, including Jaccard similarity, Min-Hashing, and a multi-feature decision tree, to find games similar to a user's selection.

The application is built as a web server using the Crow C++ framework and a custom-built front end. It features a responsive search bar with fuzzy matching, a list of recommendations, and dynamic visualizations for each algorithm to help users understand how their recommendations were generated.

How It Works
The project's core functionality is a C++ backend that processes a large dataset of games from Steam. It uses a variety of algorithms to generate recommendations, which are then served to the front end via a RESTful API.

Key Features
Fuzzy Search: Find games even with typos, powered by the RapidFuzz C++ library.

Multiple Recommendation Algorithms:

Jaccard Similarity: Calculates similarity based on the overlap of game tags, with a weighted version that considers tag popularity.

Cosine Similarity: Measures the similarity between games by treating their tags as vectors and finding the cosine of the angle between them.

Min-Hashing: An approximation algorithm that uses "fingerprints" to quickly estimate Jaccard similarity, ideal for large datasets.

Multi-Feature Similarity: A hybrid approach that combines multiple features (tags, developers, publishers, review scores) to create a more comprehensive similarity score.

Rule-Based Decision Tree: A logical, tiered approach that prioritizes games based on rules like shared developers or publishers before considering other metrics.

Visualizations: The front end provides dynamic visualizations for each algorithm, making it easy to understand the "why" behind each recommendation.

How to Build and Run (Windows)
This project is configured to be built using CMake and is designed for the CLion IDE. The C++ framework, Crow, and its dependencies will be installed using the Vcpkg package manager.

Prerequisites
A C++ compiler (supporting C++20), such as MSVC.

CMake (version 3.29 or higher).

Git.

CLion (recommended for ease of setup).

Step-by-Step Instructions
Install and Integrate Vcpkg

Open your command prompt or PowerShell.

Clone the Vcpkg repository:

Bash

git clone https://github.com/microsoft/vcpkg.git
Navigate into the vcpkg directory and run the bootstrap script:

Bash

cd vcpkg
.\bootstrap-vcpkg.bat
Integrate Vcpkg with your system to allow your build environment to find packages automatically:

Bash

.\vcpkg integrate install
Install the Crow Framework

With Vcpkg set up, install Crow. Vcpkg will automatically fetch Crow and its dependencies (e.g., Boost).

Bash

.\vcpkg install crow
Clone and Build the Project

Clone the SteamSearch repository from GitHub:

Bash

git clone https://github.com/your-username/SteamSearch.git
Open the project in CLion. CLion will detect the CMakeLists.txt file and handle the configuration. The find_package(Crow) line will now succeed because Vcpkg has installed the necessary files. The other libraries like nlohmann/json and RapidFuzz will be fetched automatically by CMake.

Click the "Build" button (a hammer icon) in the CLion toolbar.

Run the Executable

Click the "Run" button (a green play icon) in the toolbar.

The server will start, and the terminal will output the URL: http://localhost:18080.

Access the Application

Open your web browser and navigate to http://localhost:18080 to start searching for game recommendations!

Project Structure
The project has a clean and logical structure, with all core C++ files in the root directory and the front end in a separate folder.

/SteamSearch
├── .clion/              # CLion-specific files
├── public/
│   └── index.html       # The single-page front-end application
├── algorithms_B.cpp
├── algorithms_B.h
├── CMakeLists.txt
├── cosineSimilarity.cpp
├── cosineSimilarity.h
├── decoder.txt          # Mapping of game IDs to names
├── games.json           # The dataset of games
├── Game.cpp
├── Game.h
├── jaccardsSimilarity.cpp
├── jaccardsSimilarity.h
├── main.cpp             # The web server and API routes
├── minHash.cpp
├── minHash.h
├── multiFeatureSimilarity.cpp
├── multiFeatureSimilarity.h
├── RapidFuzzie.cpp      # Fuzzy string matching logic
├── RapidFuzzie.h
├── readJson.cpp
├── readJson.h
└── tags.txt             # Indexed list of all tags
>>>>>>> 748332278415ce0dcb421fb55e7dd5538f114cc5

