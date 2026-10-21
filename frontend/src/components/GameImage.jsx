import { useState } from 'react'

export const FALLBACK_IMAGE = "data:image/svg+xml;utf8," + encodeURIComponent(`
<svg xmlns="http://www.w3.org/2000/svg" width="460" height="215" viewBox="0 0 460 215">
  <rect width="460" height="215" fill="#2a475e"/>
  <g fill="none" stroke="#66c0f4" stroke-width="3" opacity="0.6">
    <rect x="150" y="60" width="160" height="95" rx="6"/>
    <circle cx="190" cy="95" r="10"/>
    <path d="M150 140 L205 100 L250 130 L290 95 L310 140 Z"/>
  </g>
  <text x="230" y="185" font-family="sans-serif" font-size="16" fill="#8f98a0" text-anchor="middle">No Image</text>
</svg>
`);

function GameImage({ src, alt, className }) {
    const [imgSrc, setImgSrc] = useState(src || FALLBACK_IMAGE);
    const [loaded, setLoaded] = useState(false);

    return (
        <img
            className={`${className} ${loaded ? 'img_loaded' : 'img_loading'}`}
            src={imgSrc}
            alt={alt}
            onLoad={() => setLoaded(true)}
            onError={() => { setImgSrc(FALLBACK_IMAGE); setLoaded(true); }}
        />
    );
}

export default GameImage;
