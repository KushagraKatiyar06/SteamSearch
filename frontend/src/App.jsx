import { useEffect, useRef, useState } from 'react'
import SteamSearchMain from './assets/steamsearch_main.svg'
import enter from './assets/enter.svg'
import './App.css'
import Navbar from './Navbar'
import SearchGlass from './assets/searchglass.svg'
import Steam from './assets/steam.svg'
import Graphic from './assets/gaminggraphic.svg'
import AlgorithmIcon from './assets/algorithm.svg'
import StopwatchIcon from './assets/stopwatch.svg'

function App() {
    const [activeAlgorithm, setActiveAlgorithm] = useState('Default');
    const [query, setQuery] = useState('');
    const [results, setResults] = useState([]);

    const [selected_game, set_selected_game] = useState(null);
    const [recommendations, set_recommendations] = useState([]);
    const [search_time, set_search_time] = useState(0);
    const [page, set_page] = useState(1);

    const secondFoldRef = useRef(null);
    const algorithms = ['Default', 'Jaccard', 'Cosine', 'Min-Hash'];

    const handleSearch = async (text) => {
        setQuery(text);
        if (!text.trim() || text.length < 2) {
            setResults([]);
            return;
        }
        try {
            const response = await fetch(`http://localhost:8080/search/${encodeURIComponent(text)}`);
            const data = await response.json();
            setResults(data);
        } catch (error) {
            console.error("Search failed:", error);
        }
    };

    const selectGame = async (game) => {
        setQuery(game.name);
        setResults([]);
        set_selected_game(game);
        set_page(1);

        const startTime = performance.now();

        try {
            const type = activeAlgorithm.toLowerCase().replace('-', '');
            const endpoint = type === 'default' ? 'global' : type;

            const response = await fetch(`http://localhost:8080/recommend/${endpoint}/${game.id}`);
            const data = await response.json();

            // FIX: Changed from setSearchTime to set_search_time to match your useState
            set_search_time((performance.now() - startTime).toFixed(0));
            set_recommendations(data);
        } catch (error) {
            console.error("Fetch failed:", error);
        }
    };

    const scrollToSecondFold = () => {
        secondFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
    };

    useEffect(() => {
        const handleKeyDown = (e) => { if (e.key === 'Enter') scrollToSecondFold(); };
        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);
    }, []);

    return (
        <div className="app-container">
            <section className="first-fold">
                <div className="hero">
                    <img src={SteamSearchMain} className="SteamSearchMain" alt="Main Logo"/>
                </div>
                <div className="continuePrompt" onClick={scrollToSecondFold}>
                    <img src={enter} className="enter" alt="enter"/>
                    <p className="pressEnter">Press enter to continue</p>
                </div>
            </section>

            <section className={`second-fold ${selected_game ? 'results_active' : ''}`} ref={secondFoldRef}>
                <Navbar />

                {!selected_game && (
                    <p className="callToAction"> <span className="highlight"> Search </span> across 100,000 games by entering a title below. </p>
                )}

                <div className="search_section">
                    <div className="searchbar_container">
                        <input
                            type="text"
                            className="searchbar"
                            placeholder="Enter Game"
                            value={query}
                            onChange={(e) => handleSearch(e.target.value)}
                        />
                        <img src={SearchGlass} className="searchGlass" alt="Search"/>
                    </div>

                    {results.length > 0 && (
                        <div className="search_results_dropdown">
                            {results.map((game) => (
                                <div key={game.id} className="search_result_item" onClick={() => selectGame(game)}>
                                    <img src={game.imageURL} alt={game.name} />
                                    <p>{game.name}</p>
                                </div>
                            ))}
                        </div>
                    )}

                    <div className="filters_container">
                        {algorithms.map((algo) => (
                            <p key={algo} className={`algorithm ${activeAlgorithm === algo ? 'active' : ''}`} onClick={() => setActiveAlgorithm(algo)}>
                                {algo}
                            </p>
                        ))}
                    </div>
                </div>

                {selected_game && (
                    <div className="results_display">
                        <div className="selected_game_card">
                            <img className = "gameImage_selected" src={selected_game.imageURL} alt={selected_game.name} />
                            <h3 className = "gameName_selected">{selected_game.name}</h3>
                        </div>

                        <div className="stats_container">
                            <div className="stat_item">
                                <img className = "algo_logo" src={AlgorithmIcon} alt="algo" />
                                <p className = "algorithm_text">{activeAlgorithm}</p>
                            </div>
                            <div className="stat_item">
                                <img className = "stopwatch" src={StopwatchIcon} alt="timer" />
                                <p className = "time">{search_time}ms</p>
                            </div>
                        </div>

                        <div className="game_grid">
                            {recommendations.slice((page - 1) * 9, page * 9).map((rec) => (
                                <div key={rec.id} className="recommendation_card">
                                    <img src={rec.imageURL} alt={rec.name} />
                                    <p>{rec.name}</p>
                                </div>
                            ))}
                        </div>

                        <div className="pagination_footer">
                            <button onClick={() => set_page(p => Math.max(1, p - 1))}>&lt;</button>
                            <span>{page}</span>
                            <button onClick={() => set_page(p => p + 1)}>&gt;</button>
                        </div>
                    </div>
                )}

                {!selected_game && (
                    <>
                        <img src={Steam} className="gameImage_recomendation" alt="image"/>
                        <img src={Graphic} className="gameName_recomendation" alt="name" />
                    </>
                )}
            </section>
        </div>
    );
}

export default App;