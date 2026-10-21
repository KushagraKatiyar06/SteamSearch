import GameImage from './GameImage'

function GameDetailModal({ game, onClose }) {
    return (
        <div className="detail_modal_overlay" onClick={onClose}>
            <div className="detail_modal" onClick={(e) => e.stopPropagation()}>
                <button className="detail_modal_close" onClick={onClose} aria-label="Close">&times;</button>
                <GameImage key={game.imageURL} className="detail_modal_image" src={game.imageURL} alt={game.name} />
                <p className="detail_modal_name">{game.name}</p>

                {game.genres && (
                    <p className="detail_modal_genres">{game.genres.split(',').filter(Boolean).join(' • ')}</p>
                )}

                <div className="detail_modal_stats">
                    {game.score != null && <p>Match Score: {(game.score * 100).toFixed(0)}%</p>}
                    {game.price != null && <p>Price: ${game.price.toFixed(2)}</p>}
                    {game.reviewScore != null && <p>{game.reviewScore.toFixed(0)}% Positive</p>}
                    {game.metacriticScore != null && <p>Metacritic: {game.metacriticScore}</p>}
                </div>

                {(game.developer || game.publisher) && (
                    <div className="detail_modal_credits">
                        {game.developer && <p>Developer: {game.developer}</p>}
                        {game.publisher && <p>Publisher: {game.publisher}</p>}
                    </div>
                )}
            </div>
        </div>
    );
}

export default GameDetailModal;
