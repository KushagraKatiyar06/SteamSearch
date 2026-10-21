import { useEffect, useRef, useState } from 'react'
import SteamSearchMain from './assets/steamsearch_main.svg'
import enter from './assets/enter.svg'
import './App.css'
import SearchGlass from './assets/searchglass.svg'
import Steam from './assets/steam.svg'
import Graphic from './assets/gaminggraphic.webp'
import AlgorithmIcon from './assets/algorithm.svg'
import StopwatchIcon from './assets/stopwatch.svg'
import GameImage from './components/GameImage'
import GameDetailModal from './components/GameDetailModal'
import { useGameSearch } from './hooks/useGameSearch'
import { useRecommendations } from './hooks/useRecommendations'

const API_BASE = import.meta.env.VITE_API_URL || 'http://localhost:8080';
const ALGORITHMS = ['Default', 'Jaccard', 'Cosine', 'Min-Hash'];

function App() {
    const [activeAlgorithm, setActiveAlgorithm] = useState('Default');
    const [detailGame, setDetailGame] = useState(null);

    const secondFoldRef = useRef(null);
    const firstFoldRef = useRef(null);

    const search = useGameSearch(API_BASE);
    const recs = useRecommendations(API_BASE, activeAlgorithm);

    const handleSelectGame = (game) => {
        search.setQuery(game.name);
        search.clearResults();
        recs.selectGame(game);
    };

    const scrollToSecondFold = () => {
        secondFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
    };

    const resetToHome = () => {
        recs.reset();
        search.setQuery('');
        search.clearResults();
        firstFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
    };

    useEffect(() => {
        const handleKeyDown = (e) => {
            if (e.key === 'Enter') scrollToSecondFold();
            if (e.key === 'Escape') setDetailGame(null);
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
                {!recs.selectedGame && (
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
                            value={search.query}
                            onChange={(e) => search.handleSearch(e.target.value)}
                        />

                        <img
                            src={SearchGlass}
                            className="searchGlass"
                            alt="Search"
                            onClick={resetToHome}
                        />
                    </div>

                    {(search.isSearching || search.searchError || search.results.length > 0) && (
                        <div className="search_results_dropdown">
                            {search.isSearching && <p className="search_status">Searching...</p>}
                            {!search.isSearching && search.searchError && <p className="search_status search_status_error">{search.searchError}</p>}
                            {!search.isSearching && !search.searchError && search.results.map((game) => (
                                <div key={game.id} className="search_result_item" onClick={() => handleSelectGame(game)}>
                                    <GameImage key={game.imageURL} src={game.imageURL} alt={game.name} />
                                    <p>{game.name}</p>
                                </div>
                            ))}
                        </div>
                    )}

                    <div className="filters_container">
                        {ALGORITHMS.map((algo) => (
                            <p key={algo} className={`algorithm ${activeAlgorithm === algo ? 'active' : ''}`} onClick={() => setActiveAlgorithm(algo)}>
                                {algo}
                            </p>
                        ))}
                    </div>
                </div>

                {recs.selectedGame && (
                    <div className="results_display">

                        <div className="selected_game_card">
                            <GameImage key={recs.selectedGame.imageURL} className="gameImage_selected" src={recs.selectedGame.imageURL} alt={recs.selectedGame.name} />
                            <p className="gameName_selected">{recs.selectedGame.name}</p>
                        </div>

                        <div className="stats_container">
                            <div className="stat_item">
                                <img className="algo_logo" src={AlgorithmIcon} alt="algo" />
                                <p className="algorithm_text">{activeAlgorithm}</p>
                            </div>
                            <div className="stat_item">
                                <img className="stopwatch" src={StopwatchIcon} alt="timer" />
                                <p className="time">{recs.searchTime}ms</p>
                            </div>
                        </div>


                        {recs.isLoading && <p className="recs_status">Loading recommendations...</p>}
                        {!recs.isLoading && recs.error && <p className="recs_status recs_status_error">{recs.error}</p>}

                        {!recs.isLoading && !recs.error && (
                            <>
                                <div className="game_grid">
                                    {recs.recommendations.slice((recs.page - 1) * 9, recs.page * 9).map((rec) => (
                                        <div key={rec.id} className="recommendation_card" onClick={() => setDetailGame(rec)}>
                                            {rec.score != null && <span className="score_badge">{(rec.score * 100).toFixed(0)}% match</span>}
                                            <GameImage key={rec.imageURL} className="gameImage_recomendation" src={rec.imageURL} alt={rec.name} />
                                            <p className="gameName_recomendation">{rec.name}</p>
                                        </div>
                                    ))}
                                </div>

                                <div className="pagination_footer">
                                    <button onClick={() => recs.setPage(p => Math.max(1, p - 1))}>&lt;</button>
                                    <span>{recs.page}</span>
                                    <button onClick={recs.nextPage}>&gt;</button>
                                </div>
                            </>
                        )}
                    </div>
                )}

                {!recs.selectedGame && (
                    <>
                        <img src={Steam} className="steam" alt="steam_logo"/>
                        <img src={Graphic} className="gaminggraphic" alt="graphic_art" />
                    </>
                )}
            </section>

            {detailGame && <GameDetailModal game={detailGame} onClose={() => setDetailGame(null)} />}
        </div>
    );
}

export default App;
