/*
 * Emby Now Playing card.
 *
 * Renders the `sessions` attribute of sensor.emby_now_playing (from the
 * emby_now_playing custom integration). Progress ticks locally from
 * position + position_updated_at, so HA only pushes state on play/pause/seek.
 *
 *   type: custom:emby-now-playing-card
 *   entity: sensor.emby_now_playing
 *   library:            # optional stat tiles; each may have a `url` attribute
 *     - sensor.emby_movies
 *     - sensor.emby_episodes
 *   show_overview: true # optional, default true
 *
 * All Emby-provided text is set with textContent; nothing is parsed as HTML.
 */

const ICONS = {
  playing: 'M8 5.14v13.72a1 1 0 0 0 1.52.85l10.29-6.86a1 1 0 0 0 0-1.7L9.52 4.29A1 1 0 0 0 8 5.14Z',
  paused: 'M7 5h3.5v14H7zM13.5 5H17v14h-3.5z',
};

const pad = (n) => String(n).padStart(2, '0');

const formatTime = (seconds) => {
  const s = Math.max(0, Math.floor(seconds));
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  return h ? `${h}:${pad(m)}:${pad(s % 60)}` : `${pad(m)}:${pad(s % 60)}`;
};

const formatDate = (iso) => {
  if (!iso) return null;
  const date = new Date(`${iso}T00:00:00`);
  if (Number.isNaN(date.getTime())) return null;
  return date.toLocaleDateString(undefined, { month: 'short', day: 'numeric', year: 'numeric' });
};

// Only allow http(s) links from session data.
const safeUrl = (url) => (typeof url === 'string' && /^https?:\/\//i.test(url) ? url : null);

const el = (tag, attrs = {}, ...children) => {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs)) {
    if (value === null || value === undefined || value === false) continue;
    if (key === 'class') node.className = value;
    else if (key === 'dataset') Object.assign(node.dataset, value);
    else node.setAttribute(key, value);
  }
  for (const child of children.flat()) {
    if (child === null || child === undefined || child === false || child === '') continue;
    node.append(child instanceof Node ? child : document.createTextNode(String(child)));
  }
  return node;
};

const link = (url, text, title) => {
  const href = safeUrl(url);
  if (!href) return el('span', {}, text);
  return el('a', { href, target: '_blank', rel: 'noopener noreferrer', title }, text);
};

const icon = (path) => {
  const ns = 'http://www.w3.org/2000/svg';
  const svg = document.createElementNS(ns, 'svg');
  svg.setAttribute('viewBox', '0 0 24 24');
  const p = document.createElementNS(ns, 'path');
  p.setAttribute('d', path);
  svg.append(p);
  return svg;
};

const join = (parts, sep = ' · ') =>
  parts.filter(Boolean).flatMap((part, i) => (i ? [el('span', { class: 'sep' }, sep), part] : [part]));

const STYLE = `
  ha-card { padding: 12px; }
  .library { display: flex; gap: 8px; margin-bottom: 8px; }
  .stat {
    flex: 1; display: flex; flex-direction: column; align-items: center;
    padding: 6px 4px; border-radius: 8px; text-decoration: none; color: inherit;
    background: var(--secondary-background-color);
  }
  a.stat:hover { color: var(--primary-color); }
  .stat-value { font-size: 1.05em; color: var(--primary-text-color); }
  a.stat:hover .stat-value { color: inherit; }
  .stat-label {
    font-size: 0.7em; font-weight: 700; text-transform: uppercase;
    letter-spacing: 0.04em; color: var(--secondary-text-color);
  }
  .sessions { display: flex; flex-direction: column; gap: 8px; }
  .session {
    display: flex; gap: 10px; padding: 10px; border-radius: 8px;
    background: var(--secondary-background-color);
  }
  .poster { flex-shrink: 0; width: 64px; }
  .poster img {
    width: 64px; border-radius: 6px; display: block;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
  }
  .details { flex: 1; min-width: 0; font-size: 0.9em; line-height: 1.4; }
  .title { font-weight: 600; color: var(--primary-text-color); }
  .meta { margin-top: 2px; display: flex; flex-wrap: wrap; align-items: center; gap: 0 2px; }
  .code { font-weight: 600; }
  .state svg { width: 16px; height: 16px; fill: currentColor; vertical-align: -3px; }
  .progress {
    height: 3px; margin-top: 6px; border-radius: 2px; overflow: hidden;
    background: var(--divider-color);
  }
  .progress > div { height: 100%; background: var(--primary-color); }
  .device { margin-top: 4px; font-size: 0.85em; color: var(--secondary-text-color); }
  .info { margin-top: 4px; font-size: 0.85em; color: var(--secondary-text-color); }
  .overview {
    display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;
  }
  .sep { margin: 0 2px; color: var(--secondary-text-color); }
  a { color: inherit; text-decoration: none; }
  a:hover { color: var(--primary-color); }
  .empty { padding: 8px 4px; font-style: italic; color: var(--secondary-text-color); }
`;

class EmbyNowPlayingCard extends HTMLElement {
  setConfig(config) {
    if (!config.entity) throw new Error('entity is required');
    this._config = { show_overview: true, library: [], ...config };
    this._renderedKey = null;
    if (this._hass) this._render();
  }

  set hass(hass) {
    this._hass = hass;
    this._render();
  }

  connectedCallback() {
    this._timer = setInterval(() => this._tick(), 1000);
  }

  disconnectedCallback() {
    clearInterval(this._timer);
  }

  getCardSize() {
    return 1 + 2 * (this._sessions?.length || 1);
  }

  getGridOptions() {
    return { columns: 12, min_columns: 6 };
  }

  _render() {
    if (!this._config || !this._hass) return;

    const stateObj = this._hass.states[this._config.entity];
    const sessions = stateObj?.attributes?.sessions || [];
    const library = this._config.library.map((id) => this._hass.states[id]).filter(Boolean);

    // hass is set on every state change anywhere; only rebuild when our data changed.
    const key = JSON.stringify([
      stateObj ? stateObj.last_updated : null,
      library.map((s) => [s.entity_id, s.state, s.attributes.url]),
    ]);
    if (key === this._renderedKey) return;
    this._renderedKey = key;
    this._sessions = sessions;

    if (!this.shadowRoot) {
      this.attachShadow({ mode: 'open' });
      this._card = document.createElement('ha-card');
      if (this._config.title) this._card.setAttribute('header', this._config.title);
      this.shadowRoot.append(el('style', {}, STYLE), this._card);
    }

    const content = [];
    if (library.length) {
      content.push(el('div', { class: 'library' }, library.map((s) => this._stat(s)), this._stat(null, sessions.length)));
    }
    if (!stateObj) {
      content.push(el('div', { class: 'empty' }, `Entity not found: ${this._config.entity}`));
    } else if (!sessions.length) {
      content.push(el('div', { class: 'empty' }, 'Nothing playing.'));
    } else {
      content.push(el('div', { class: 'sessions' }, sessions.map((s) => this._session(s))));
    }
    this._card.replaceChildren(...content);
    this._tick();
  }

  _stat(stateObj, nowPlaying) {
    const value = stateObj ? Number(stateObj.state).toLocaleString() : String(nowPlaying);
    const label = stateObj
      ? (stateObj.attributes.friendly_name || stateObj.entity_id).replace(/^Emby\s+/i, '')
      : 'Now Playing';
    const href = stateObj ? safeUrl(stateObj.attributes.url) : null;
    const children = [el('span', { class: 'stat-value' }, value), el('span', { class: 'stat-label' }, label)];
    return href
      ? el('a', { class: 'stat', href, target: '_blank', rel: 'noopener noreferrer' }, children)
      : el('div', { class: 'stat' }, children);
  }

  _session(s) {
    let title;
    let meta = [];
    if (s.type === 'episode') {
      title = join([link(s.series_url, s.series), link(s.url, s.title)]);
      meta = [
        s.season != null && link(s.season_url, `S${pad(s.season)}`, s.series),
        s.episode != null && link(s.url, `E${pad(s.episode)}`, s.title),
      ].filter(Boolean);
      meta.forEach((node) => node.classList.add('code'));
    } else if (s.type === 'audio') {
      title = join([link(s.artist_url, s.artist), link(s.album_url, s.album), link(s.url, s.title)]);
      if (s.track != null) meta = [el('span', {}, `Track ${s.track}`)];
    } else {
      title = [link(s.url, s.year ? `${s.title} (${s.year})` : s.title)];
    }

    const timer = el(
      'span',
      { class: 'timer' },
      el('span', { class: 'position' }),
      ' / ',
      s.duration ? formatTime(s.duration) : '--:--',
    );
    const state = el('span', { class: 'state', title: s.state }, icon(ICONS[s.state] || ICONS.playing));

    const external = [
      s.imdb_url && link(s.imdb_url, 'IMDb'),
      s.tmdb_url && link(s.tmdb_url, 'TMDb'),
      s.tvdb_url && link(s.tvdb_url, 'TheTVDB'),
    ].filter(Boolean);
    const info = join([formatDate(s.date), ...external], ' — ');
    const overview = this._config.show_overview && s.type !== 'audio' && s.overview;

    const poster = safeUrl(s.image)
      ? el(
          'div',
          { class: 'poster' },
          link(s.series_url || s.album_url || s.url, el('img', { src: safeUrl(s.image), alt: '', loading: 'lazy' })),
        )
      : null;

    return el(
      'div',
      {
        class: 'session',
        dataset: {
          state: s.state,
          position: s.position,
          duration: s.duration || 0,
          updatedAt: s.position_updated_at,
        },
      },
      poster,
      el(
        'div',
        { class: 'details' },
        el('div', { class: 'title' }, title),
        el('div', { class: 'meta' }, join([...meta, timer]), state),
        el('div', { class: 'progress' }, el('div')),
        el('div', { class: 'device' }, join([s.device, s.client, s.play_method, s.user])),
        info.length ? el('div', { class: 'info' }, info) : null,
        overview ? el('div', { class: 'info overview' }, overview) : null,
      ),
    );
  }

  _tick() {
    if (!this.shadowRoot) return;
    const now = Date.now();
    for (const node of this.shadowRoot.querySelectorAll('.session')) {
      const { state, position, duration, updatedAt } = node.dataset;
      let seconds = Number(position) || 0;
      if (state === 'playing' && updatedAt) {
        seconds += (now - new Date(updatedAt).getTime()) / 1000;
      }
      const total = Number(duration);
      if (total) seconds = Math.min(seconds, total);
      node.querySelector('.position').textContent = formatTime(seconds);
      node.querySelector('.progress > div').style.width = total ? `${(seconds / total) * 100}%` : '0';
    }
  }

  static getStubConfig() {
    return { entity: 'sensor.emby_now_playing', library: ['sensor.emby_movies', 'sensor.emby_episodes'] };
  }
}

customElements.define('emby-now-playing-card', EmbyNowPlayingCard);

window.customCards = window.customCards || [];
window.customCards.push({
  type: 'emby-now-playing-card',
  name: 'Emby Now Playing',
  description: 'What Emby clients are playing right now.',
});
