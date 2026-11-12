// Pušinka's sketches, what they share: the language and the screen from ?lang= and ?s=, the
// menu along the bottom, the set's units with their places' codes, a helper for SVG.
const Q = new URLSearchParams(location.search);
const LANG = Q.get('lang') || 'en';
const SCREEN = Q.get('s') || 'a';
document.documentElement.lang = LANG;
const $ = id => document.getElementById(id);

const MENU = {
  en: ['Overview', 'Calibration', 'Measurement', 'Data', 'Diagnostics', 'Settings'],
  cs: ['Přehled', 'Kalibrace', 'Měření', 'Data', 'Diagnostika', 'Nastavení'],
};
// the template's codes, English in every language, and the places' names (APP.md, What it must do, 4)
const PLACES = {
  en: { head: ['HD', 'head'], withers: ['WI', 'withers'], pelvis: ['PE', 'pelvis'],
        lumbar: ['LS', 'lumbar spine'], thoracic: ['TS', 'thoracic spine'], lf: ['LF', 'left fore'],
        rf: ['RF', 'right fore'], lh: ['LH', 'left hind'], rh: ['RH', 'right hind'] },
  cs: { head: ['HD', 'hlava'], withers: ['WI', 'kohoutek'], pelvis: ['PE', 'pánev'],
        lumbar: ['LS', 'bederní páteř'], thoracic: ['TS', 'hrudní páteř'], lf: ['LF', 'levá přední'],
        rf: ['RF', 'pravá přední'], lh: ['LH', 'levá zadní'], rh: ['RH', 'pravá zadní'] },
};
// the set of the sketches: place, kind and number
const SET = [['head', 'B1'], ['withers', 'B2'], ['pelvis', 'B3'], ['lumbar', 'B4'],
             ['thoracic', 'B5'], [null, 'B6'], ['lf', 'K7'], ['rf', 'K8'], ['lh', 'K9'],
             [null, 'K10'], ['rh', 'Z11'], [null, 'Z12']];
const codeOf = key => key ? (PLACES[LANG] || PLACES.en)[key][0] : '--';

function pick(texts) { return texts[LANG] || texts.en; }

// the title, the screens' tabs if any, the menu with the current one, an action at its end
function chrome({ title, right, menu, tabs, action }) {
  $('ttl').textContent = title;
  $('tright').innerHTML = right || '';
  if (tabs) {
    $('subtabs').innerHTML = tabs.map((t, i) => `<div class="subtab${'abcd'[i] === SCREEN ? ' cur' : ''}">`
      + `<b>${'ABCD'[i]}</b>${t}</div>`).join('');
  }
  $('keys').innerHTML = pick(MENU).map((t, i) =>
    `<div class="key${i === menu ? ' cur' : ''}"><b>${i + 1}</b><span>${t}</span></div>`).join('')
    + (action ? `<div class="act ${action[0]}">${action[1]}</div>` : '');
}

const NS = 'http://www.w3.org/2000/svg';
function el(svg, name, attrs, text) {
  const e = document.createElementNS(NS, name);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  if (text !== undefined) e.textContent = text;
  svg.appendChild(e);
  return e;
}
