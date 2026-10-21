import { useEffect, useState } from 'react'

// Fetches /recommend for the selected game + active algorithm, with paging
// over the result set.
export function useRecommendations(apiBase, activeAlgorithm) {
    const [selectedGame, setSelectedGame] = useState(null);
    const [recommendations, setRecommendations] = useState([]);
    const [searchTime, setSearchTime] = useState(0);
    const [page, setPage] = useState(1);
    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState(null);

    const selectGame = async (game) => {
        setSelectedGame(game);
        setPage(1);
        setIsLoading(true);
        setError(null);

        const startTime = performance.now();
        try {
            const type = activeAlgorithm.toLowerCase().replace('-', '');
            const endpoint = type === 'default' ? `global/${game.id}` : `${type}/${game.id}`;
            const url = `${apiBase}/recommend/${endpoint}`;

            console.log("Fetching from:", url);
            const response = await fetch(url);
            if (!response.ok) throw new Error(`Recommend request failed (${response.status})`);
            const data = await response.json();

            setSearchTime((performance.now() - startTime).toFixed(0));
            setRecommendations(data.slice(0, 90));
        } catch (err) {
            console.error("Fetch failed:", err);
            setRecommendations([]);
            setError("Couldn't load recommendations. Please try again.");
        } finally {
            setIsLoading(false);
        }
    };

    useEffect(() => {
        if (selectedGame) {
            selectGame(selectedGame);
        }
    }, [activeAlgorithm]);

    const nextPage = () => {
        const maxPages = Math.ceil(recommendations.length / 9);
        if (page < Math.min(maxPages, 10)) {
            setPage(prev => prev + 1);
        }
    };

    const reset = () => {
        setSelectedGame(null);
        setRecommendations([]);
    };

    return { selectedGame, recommendations, searchTime, page, setPage, isLoading, error, selectGame, nextPage, reset };
}
