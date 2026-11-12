// Pušinka's sketches as pictures (APP.md, The menus): each menu's HTML here, at 1920 × 1080, a
// PNG a screen in the language asked, into <lang>/ beside this file or the folder given.
//
//     node app/mockups/render.js [en|cs] [out]
//
// Playwright's Chromium does the drawing (npm's playwright, its browser installed once).
const path = require('path');
const fs = require('fs');
const { chromium } = require('playwright');

// a menu's file and its screens, ?s= and the picture's name
const MENUS = {
  join: [['a', '']],
  overview: [['a', '']],
  unit: [['a', '']],
  calibration: [['a', '']],
  measurement: [['a', 'a-choice'], ['b', 'b-overview'], ['c', 'c-curves'], ['d', 'd-numbers']],
  data: [['a', 'a-list'], ['b', 'b-download'], ['c', 'c-format']],
  diagnostics: [['a', '']],
  settings: [['a', 'a-language'], ['b', 'b-about'], ['c', 'c-new-units'], ['d', 'd-templates'],
             ['e', 'e-firmware'], ['f', 'f-measurement']],
};

(async () => {
  const lang = process.argv[2] || 'en';
  const out = process.argv[3] || path.join(__dirname, lang);
  fs.mkdirSync(out, { recursive: true });
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
  for (const [menu, screens] of Object.entries(MENUS)) {
    for (const [s, name] of screens) {
      const url = 'file://' + path.join(__dirname, menu + '.html') + `?s=${s}&lang=${lang}`;
      await page.goto(url);
      const file = path.join(out, `${menu}${name ? '-' + name : ''}.png`);
      await page.screenshot({ path: file });
      console.log(file);
    }
  }
  await browser.close();
})();
