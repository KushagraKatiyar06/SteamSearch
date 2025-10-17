# SteamSearch: A C++ Game Recommendation Engine

Steam Search is a C++ based game recommendation web application which allows users to input a game and retrieve recommendations
for games based on six different algorithms, all favoring different values and data on games. There are around 100,000 games in the database dating to 2023.  

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

## Project Structure
The project has a clean and logical structure, with all core C++ files in the root directory and the front end in a separate folder.

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
