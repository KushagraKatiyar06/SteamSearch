# SteamSearch
Steam Games recommendation engine supporting over 100,000 games circa 2024. Features 5 algorithms to produce n similar games.

Source of our Steam data:
  https://www.kaggle.com/datasets/fronkongames/steam-games-dataset

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

Made by Kushagra Katiyar, Bayan Mahmoodi, Agnivesh Kaundinya
