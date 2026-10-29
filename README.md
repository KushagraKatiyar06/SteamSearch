<p align="center">
  <img src="screenshots/Hero.png" alt="Steam Search" width="100%">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/CMake-3.22%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.22+">
  <img src="https://img.shields.io/badge/React-19.2-61DAFB?style=for-the-badge&logo=react&logoColor=black" alt="React 19.2">
  <img src="https://img.shields.io/badge/Vite-7.2-646CFF?style=for-the-badge&logo=vite&logoColor=white" alt="Vite 7.2">
  <img src="https://img.shields.io/badge/Steam-100k%2B%20Games-1B2838?style=for-the-badge&logo=steam&logoColor=white" alt="Steam catalog">
  <img src="https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white" alt="Docker">
  <img src="https://img.shields.io/badge/Railway-0B0D0E?style=for-the-badge&logo=railway&logoColor=white" alt="Railway">
</p>

Steam Search is a game recommendation engine with access to over 100,000 games from Steam. Type in a game you like, and it finds similar ones in under 200ms — no ML model, no vector database, just a C++ backend doing the math directly over a hand-built binary index.

**Live demo:** https://steamsearch-production-622f.up.railway.app/

## What it does

1. **Search** for a game by name; the backend scans the in-memory catalog and returns the top 15 name matches with their box art.
2. **Pick a result** and the server scores every other game against it using three independent similarity measures, then blends them into a single ranked recommendation list.
3. **Results stream back** in well under 200ms, even across the full 100k+ game catalog, because every game's signature is pre-computed at index time — the recommendation endpoint is pure arithmetic over fixed-size binary structs, no disk I/O or re-parsing per request.

<p align="center">
  <a href="https://steamsearch-production-622f.up.railway.app/">
    <img src="screenshots/Demo.png" alt="Steam Search recommendation results" width="640">
  </a>
</p>

## How recommendations are scored

Each game is reduced to three fixed-size signatures, pre-computed once by the offline converter and baked into the binary index:

- **Jaccard tag similarity** — each game's tags are packed into a 256-bit bitset; similarity is `popcount(intersection) / popcount(union)` across two games' bitsets, computed with `__builtin_popcount`.
- **MinHash** — 150 independent random permutations of the tag space are hashed per game; similarity is the fraction of the 150 hash slots that agree between two games, a fast approximation of Jaccard that holds up well even as tag lists grow.
- **Weighted cosine similarity** — tag vote counts are hashed into a fixed 128-dimensional bucket vector and L2-normalized at index time, so similarity is a single dot product at query time.

The `/recommend/global/<id>` endpoint blends all three into one score — `0.5·cosine + 0.3·minhash + 0.2·jaccard` — and returns the top 90 matches above a relevance floor. Each algorithm is also exposed individually at `/recommend/<jaccard|minhash|cosine>/<id>` for comparison.

## Features

- **Sub-200ms recommendations** across 100k+ games, powered by pre-computed signatures and zero runtime parsing.
- **Three blendable similarity algorithms** (Jaccard tags, MinHash, weighted cosine), each independently queryable.
- **Compact binary catalog** — a 600MB raw JSON export of the Steam catalog is converted offline into ~125MB of fixed-size binary structs plus a string pool, cutting server RAM usage from ~1GB to ~300MB.
- **String interning** — names, images, developers, publishers, and genres are deduplicated into a single string pool and referenced by offset, instead of being duplicated per game.
- **Zero-dependency-setup backend build** — CMake's `FetchContent` pulls in Crow, Asio, and nlohmann/json automatically on first build; no vcpkg or system package manager required.
- **One-click Windows scripts** (`setup.bat` / `start.bat`) alongside a fully manual, OS-agnostic build path.
- **Dockerized deploy** — the same container that runs locally is what's deployed to Railway.

## Tech Stack

| Layer | Tech |
|---|---|
| Frontend | React, Vite, CSS, JavaScript |
| Backend | C++20, [Crow](https://crowcpp.org/) (web framework), [Asio](https://think-async.com/Asio/) (networking), [nlohmann/json](https://github.com/nlohmann/json) |
| Build | CMake 3.22+ |
| Deploy | Docker, Railway |

The backend's C++ dependencies (Crow, Asio, nlohmann/json) are fetched automatically by CMake the first time you build — no package manager (vcpkg, apt, etc.) setup required.

## What you need

| Tool | Why | Link |
|---|---|---|
| Git | Lets CMake fetch Crow/Asio automatically | https://git-scm.com/downloads |
| CMake 3.22+ | Builds the C++ backend | https://cmake.org/download/ |
| A C++20 compiler | Compiles the backend | [CLion](https://www.jetbrains.com/clion/) bundles one (MinGW), or install [MinGW-w64](https://www.mingw-w64.org/)/MSVC yourself |
| Node.js (npm) | Runs the frontend | https://nodejs.org/ |

If you already have CMake or a compiler installed system-wide, `setup.bat` uses those. If not, and you have CLion installed, it falls back to CLion's bundled copies automatically.

## Getting started

**1. Install the prerequisites above** if you don't already have them.

**2. Run setup.** On Windows, double-click **`setup.bat`** — it checks for the required tools, builds the backend, and installs frontend dependencies. First run takes a few minutes (it downloads Crow/Asio and compiles the server).

**3. Start it.** Double-click **`start.bat`** — it starts the backend, starts the frontend dev server, and opens the site in your browser.

That's it. Re-run `start.bat` any time to relaunch; only re-run `setup.bat` if you've wiped `node_modules` or the `cmake-build-debug` folder.

### Manual setup (any OS)

If you're not on Windows, or just want to run the steps yourself:

**Backend:**
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target steam_server
./steam_server          # macOS/Linux
.\steam_server.exe      # Windows
```
The server listens on port `8080` by default (override with the `PORT` env var), and expects `data/` (already included in this repo) relative to its working directory.

**Frontend:**
```bash
cd frontend
npm install
npm run dev
```
By default it points at `http://localhost:8080` for the API; override with a `VITE_API_URL` env var.

### Running via Docker

The backend can also be built and run exactly as it's deployed on Railway:
```bash
docker build -t steamsearch .
docker run -p 8080:8080 steamsearch
```

## Rebuilding the index

`data/games_1.bin`, `data/games_2.bin`, and `data/strings.bin` are pre-built and checked into this repo, so you don't need to regenerate them to run the app. If you're updating the source catalog (`data/games.json`), rerun the converter to regenerate the binaries:

```bash
cmake --build cmake-build-debug --target data_converter
./data_converter
```

It reads `data/games.json` and `data/tags.txt`, builds the Jaccard/MinHash/cosine signatures for every game, and writes the catalog out as two binary shards (split at 55,000 games) plus a deduplicated string pool.

## File Structure

| Folder | What is in it |
|---|---|
| `src/` | The C++ backend — HTTP server, game store, similarity algorithms, and the offline data converter |
| `frontend/` | The React + Vite frontend |
| `data/` | The pre-indexed binary game catalog the server loads on startup |
| `Legacy/` | Earlier implementation, kept for reference |
| `screenshots/` | Images used in this README |
| `CMakeLists.txt` | Backend build config, including the FetchContent dependency setup |
| `Dockerfile` | Container build used for local Docker runs and Railway deploys |
| `setup.bat` / `start.bat` | One-click Windows build and launch scripts |

### Developed by Kushagra Katiyar
