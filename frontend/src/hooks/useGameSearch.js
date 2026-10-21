import { useEffect, useRef, useState } from 'react'

// Debounced title search against /search, with its own loading/error state.
export function useGameSearch(apiBase) {
    const [query, setQuery] = useState('');
    const [results, setResults] = useState([]);
    const [isSearching, setIsSearching] = useState(false);
    const [searchError, setSearchError] = useState(null);

    const searchTimeoutRef = useRef(null);

    const performSearch = async (text) => {
        setIsSearching(true);
        setSearchError(null);
        try {
            const response = await fetch(`${apiBase}/search/${encodeURIComponent(text)}`);
            if (!response.ok) throw new Error(`Search request failed (${response.status})`);
            const data = await response.json();
            setResults(data);
        } catch (error) {
            console.error("Search failed:", error);
            setResults([]);
            setSearchError("Couldn't load search results. Please try again.");
        } finally {
            setIsSearching(false);
        }
    };

    const handleSearch = (text) => {
        setQuery(text);
        if (searchTimeoutRef.current) clearTimeout(searchTimeoutRef.current);

        if (!text.trim() || text.length < 2) {
            setResults([]);
            setSearchError(null);
            setIsSearching(false);
            return;
        }

        searchTimeoutRef.current = setTimeout(() => performSearch(text), 250);
    };

    const clearResults = () => {
        if (searchTimeoutRef.current) clearTimeout(searchTimeoutRef.current);
        setResults([]);
        setSearchError(null);
        setIsSearching(false);
    };

    useEffect(() => {
        return () => {
            if (searchTimeoutRef.current) clearTimeout(searchTimeoutRef.current);
        };
    }, []);

    return { query, setQuery, results, isSearching, searchError, handleSearch, clearResults };
}
