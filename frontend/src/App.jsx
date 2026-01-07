import { useEffect, useRef, useState } from 'react'
import SteamSearchMain from './assets/steamsearch_main.svg'
import enter from './assets/enter.svg'
import './App.css'
import Navbar from './Navbar'
import SearchGlass from './assets/searchglass.svg'
import Steam from './assets/steam.svg'
import Graphic from './assets/gaminggraphic.svg'

function App() {

    //Algorithm Picker
    const [activeAlgorithm, setActiveAlgorithm] = useState('Default');
    const secondFoldRef = useRef(null);
    const algorithms = ['Default', 'Jaccard', 'Cosine', 'Min-Hash'];

    //Fuzzy Search
    const [query, setQuery] = useState('');
    const [results, setResults] = useState([])

    const handleSearch = async (text) => {
        setQuery(text);

        if (!text.trim() || text.length < 2) {
            setResults([]);
            return;
        }

        try {
            const response = await fetch(`http://localhost:8080/search/${encodeURIComponent(text)}`);

            if (!response.ok) {
                setResults([]);
                return;
            }

            const data = await response.json();
            setResults(data);
        } catch (error) {
            console.error("Search failed:", error);
            setResults([]);
        }
    };

    const selectGame = (game) => {
        setQuery(game.name);
        setResults([]);
    };

    // Scrolling
    const scrollToSecondFold = () => {
        secondFoldRef.current?.scrollIntoView({ behavior: 'smooth' });
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
            <section className="first-fold">
                <div className="hero">
                    <img src={SteamSearchMain} className="SteamSearchMain" alt="Steam_Search_Main"/>
                </div>

                <div className="continuePrompt" onClick={scrollToSecondFold}>
                    <img src={enter} className="enter" alt="enter"/>
                    <p className="pressEnter">Press enter to continue</p>
                </div>
            </section>

            <section className="second-fold" ref={secondFoldRef}>
                <Navbar />
                <p className="callToAction"> <span className="highlight"> Search </span> across 100,000 games by entering a title below. </p>

                <div className="searchbar_container">
                    <input
                        type="text"
                        className="searchbar"
                        placeholder="Enter Game"
                        value={query}
                        onChange={(e) => handleSearch(e.target.value)} // Live search!
                    />
                    <img src={SearchGlass} className="searchGlass" alt="Search Glass"/>
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
                        <p
                            key={algo}
                            className={`algorithm ${activeAlgorithm === algo ? 'active' : ''}`}
                            onClick={() => setActiveAlgorithm(algo)}
                        >
                            {algo}
                        </p>
                    ))}
                </div>

                <img src={Steam} className="steam" alt="steam"/>
                <img src={Graphic} className="gaminggraphic" alt="graphic" />
            </section>
        </div>
    );
}

export default App;