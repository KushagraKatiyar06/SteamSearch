import { useEffect, useRef, useState } from 'react'
import SteamSearchMain from './assets/steamsearch_main.svg'
import enter from './assets/enter.svg'
import './App.css'
import SearchGlass from './assets/searchglass.svg'
import Steam from './assets/steam.svg'
import Graphic from './assets/gaminggraphic.svg'
import AlgorithmIcon from './assets/algorithm.svg'
import StopwatchIcon from './assets/stopwatch.svg'

const JaccardVenn = ({ selectedBits, recBits }) => {
    let intersect = 0;
    let union = 0;
    for (let i = 0; i < 8; i++) {
        const s = selectedBits[i] || 0;
        const r = recBits[i] || 0;
        intersect += (s & r).toString(2).split('1').length - 1;
        union += (s | r).toString(2).split('1').length - 1;
    }

    return (
        <div className="viz-box">
            <h4>Jaccard Tag Overlap</h4>
            <div className="venn-diag">
                <div className="venn-circle left">Selected</div>
                <div className="venn-circle right">Rec</div>
            </div>
            <div className="intersection-count">{intersect} Shared Tags</div>
            <p className="stat-text">Union: {union} Total Unique Tags</p>
        </div>
    );
};

const CosineAngle = ({ score }) => {
    const angleRad = Math.acos(Math.max(0, Math.min(1, score)));
    const angleDeg = (angleRad * 180 / Math.PI).toFixed(1);

    return (
        <div className="viz-box">
            <h4>Cosine Vector Angle</h4>
            <svg width="100" height="100" viewBox="0 0 100 100">
                <circle cx="50" cy="50" r="40" fill="none" stroke="#444" strokeDasharray="2,2" />
                <line x1="50" y1="50" x2="50" y2="10" stroke="white" strokeWidth="2" />
                <line x1="50" y1="50" x2="50" y2="10" stroke="#66c0f4" strokeWidth="3"
                      style={{ transform: `rotate(${angleDeg}deg)`, transformOrigin: '50px 50px', transition: 'transform 1s' }} />
            </svg>
            <p className="stat-text">{angleDeg}° Offset</p>
        </div>
    );
};

const MinHashMatch = ({ selectedSig, recSig }) => {
    return (
        <div className="viz-box">
            <h4>MinHash Signature Alignment</h4>
            <div className="sig-strip">
                {selectedSig?.map((val, i) => (
                    <div key={i} className={`sig-pixel ${val === recSig[i] && val !== 0 ? 'match' : ''}`} />
                ))}
            </div>
            <p className="stat-text">150 Hashes Compared</p>
        </div>
    );
};

function App() {
    const [activeAlgorithm, setActiveAlgorithm] = useState('Default');
    const [query, setQuery] = useState('');
    const [results, setResults] = useState([]);

    const [selected_game, set_selected_game] = useState(null);
    const [recommendations, set_recommendations] = useState([]);
    const [search_time, set_search_time] = useState(0);
    const [page, set_page] = useState(1);
    const [detailGame, setDetailGame] = useState(null);

    const secondFoldRef = useRef(null);
    const firstFoldRef = useRef(null);
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

    useEffect(() => {
        if (selected_game) {
            selectGame(selected_game);
        }
    }, [activeAlgorithm]);

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
            let data = await response.json();

            const cappedData = data.slice(0, 90);

            set_search_time((performance.now() - startTime).toFixed(0));
            set_recommendations(cappedData);

            secondFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
        } catch (error) {
            console.error("Fetch failed:", error);
        }
    };

    const nextPage = () => {
        const maxPages = Math.ceil(recommendations.length / 9);
        if (page < Math.min(maxPages, 10)) {
            set_page(prev => prev + 1);
        }
    };

    const scrollToSecondFold = () => {
        secondFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
    };

    const resetToHome = () => {
        set_selected_game(null);
        set_recommendations([]);
        setQuery('');
        setResults([]);
        firstFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
    };

    useEffect(() => {
        const handleKeyDown = (e) => {
            if (e.key === 'Enter') scrollToSecondFold();
        };
        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);
    }, []);

    return (
        <div className="app-container">

            <section className="first-fold" ref={firstFoldRef}>
                <div className="hero">
                    <img src={SteamSearchMain} className="SteamSearchMain" alt="Steam Search" />
                </div>
                <div className="continuePrompt" onClick={scrollToSecondFold}>
                    <p className="pressEnter">Press Enter to Search</p>
                    <img src={enter} className="enter" alt="Enter Icon" />
                </div>
            </section>

            <section className="second-fold" ref={secondFoldRef}>
                {!selected_game && (
                    <p className="callToAction">
                        <span className="highlight"> Search </span> across 100,000 games by entering a title below.
                    </p>
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
                        {/* --- MODIFIED: onClick triggers reset --- */}
                        <img
                            src={SearchGlass}
                            className="searchGlass"
                            alt="Search"
                            onClick={resetToHome}
                        />
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
                            <img className="gameImage_selected" src={selected_game.imageURL} alt={selected_game.name} />
                            <p className="gameName_selected">{selected_game.name}</p>
                        </div>

                        <div className="stats_container">
                            <div className="stat_item">
                                <img className="algo_logo" src={AlgorithmIcon} alt="algo" />
                                <p className="algorithm_text">{activeAlgorithm}</p>
                            </div>
                            <div className="stat_item">
                                <img className="stopwatch" src={StopwatchIcon} alt="timer" />
                                <p className="time">{search_time}ms</p>
                            </div>
                        </div>


                        <div className="game_grid">
                            {recommendations.slice((page - 1) * 9, page * 9).map((rec) => (
                                <div key={rec.id} className="recommendation_card" onClick={() => setDetailGame(rec)}>
                                    <img className="gameImage_recomendation" src={rec.imageURL} alt={rec.name} />
                                    <p className="gameName_recomendation">{rec.name}</p>
                                </div>
                            ))}
                        </div>

                        {detailGame && (
                            <div className="modal-overlay" onClick={() => setDetailGame(null)}>
                                <div className="modal-card" onClick={e => e.stopPropagation()}>
                                    <h2 className="highlight">{detailGame.name}</h2>
                                    <div className="scroll-area">
                                        {(activeAlgorithm === 'Default' || activeAlgorithm === 'Jaccard') &&
                                            <JaccardVenn selectedBits={selected_game.tagBits} recBits={detailGame.tagBits} />}

                                        {(activeAlgorithm === 'Default' || activeAlgorithm === 'Cosine') &&
                                            <CosineAngle score={detailGame.score} />}

                                        {(activeAlgorithm === 'Default' || activeAlgorithm === 'Min-Hash') &&
                                            <MinHashMatch selectedSig={selected_game.minHash} recSig={detailGame.minHash} />}
                                    </div>
                                    <button className="algorithm" style={{marginTop: '1rem', alignSelf: 'center'}} onClick={() => setDetailGame(null)}>Close</button>
                                </div>
                            </div>
                        )}

                        <div className="pagination_footer">
                            <button onClick={() => set_page(p => Math.max(1, p - 1))}>&lt;</button>
                            <span>{page}</span>
                            <button onClick={nextPage}>&gt;</button>
                        </div>
                    </div>
                )}

                {!selected_game && (
                    <>
                        <img src={Steam} className="steam" alt="steam_logo"/>
                        <img src={Graphic} className="gaminggraphic" alt="graphic_art" />
                    </>
                )}
            </section>
        </div>
    );
}

export default App;