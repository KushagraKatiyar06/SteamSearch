import SteamSearchNavbar from './assets/steamsearch_navbar.svg'
import './Navbar.css'

function Navbar() {
    return (
        <nav className = "navbar">
            <img src = {SteamSearchNavbar} className = "SteamSearchNavbar" alt = "Steam_Search_Navbar"/>
            <ul className = "navLinks">
                <p className = "recommendations_navbar">Recommendations</p>
                <p className = "about_navbar">About</p>
            </ul>
        </nav>
    );
}

export default Navbar;