import { useEffect, useRef } from 'react' // Added useRef and useEffect
import SteamSearchMain from './assets/steamsearch_main.svg'
import enter from './assets/enter.svg'
import './App.css'

function App() {
    const secondFoldRef = useRef(null);
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

            </section>
        </div>
    );
}

export default App;