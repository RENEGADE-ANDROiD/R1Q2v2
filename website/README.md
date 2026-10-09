# R1Q2v2 website

GitHub Pages serves `docs/`. The site is buildless HTML/CSS/JavaScript.

Generate the searchable reference whenever `CVARS.md` changes:

```text
python website/build_cvars.py
```

The Pages workflow regenerates it automatically. For a local preview:

```text
python -m http.server 4174 --directory docs
```

Downloads start with a verified Build 8065 asset URL and refresh from GitHub's
latest-release API when it is available. If the API is unavailable or rate
limited, the existing download continues to work. Public copy describes the
client's features without build numbers. Review captures when their
demonstrated behavior changes.

## Media provenance

The gameplay PNGs and GIF frames are genuine 1920×1080 captures of the published
Build 8065 executable and renderer. The Advanced Settings and Crosshair Setup
PNGs are native 1514×848 window captures taken with Computer Use. GIFs encode
actual moving gameplay at its original resolution, using a generated palette
and dithering. The HUD clip uses Installation / Super Shotgun, and the
crosshair clip uses Main Gate / Railgun. Each clip is 1.36 seconds, showing
only the barrel shot and explosion, and loops automatically without controls.
The Lighting section uses the two user-supplied 1920×1080 JPEGs, copied
unchanged from `20261005215014_1.jpg` and `20261005214947_1.jpg`. These
show sunlight/shadows and colored lighting, rather than a before/after pair.
The hero uses Outer Base. The clean capture copy contained only
the original `pak0.pak` and `pak1.pak`, plus the released game module in
`baseq2/`. Custom PAK/PKZ archives and loose replacements were excluded. The
user's normal game installation was not edited or renamed.

- Release ZIP SHA-256: `9da4a94e07415bc0943c5a03bbd90b6320e3ca6277addc0bcd5ae1b417115947`
- `pak0.pak` SHA-256: `1ce99eb11e7e251ccdf690858effba79836dbe5e32a4083ad00a13ecda491679`
- `pak1.pak` SHA-256: `678210ecd1b27dde1c645660333a1a7b139d849425793859657f804d379b62ad`

Menu screenshots: Advanced Settings, Crosshair Setup, Video and HUD, Options,
R1Q2 Settings, Multiplayer Settings. Preview images preserve their aspect ratio
and are capped at 720 CSS pixels wide; original files open in the lightbox.
Feature GIFs show the stock HUD and crosshair during barrel shots and
explosions. Lighting screenshots have full-resolution Enlarge links. The captures are from a local single-player session;
they do not establish multiplayer behavior or performance.
Mouse input is disabled and keyboard bindings are cleared in the temporary
capture copy. Camera movement and feature changes are scripted, so desktop
mouse movement cannot change the recorded view.

Support links match the public Shufflebox website: PayPal and
`https://cash.app/$renegadeandroid`.

The multiplayer server browser section uses the user-supplied
`20261008200530_1.jpg`, copied unchanged at 1920×1080. Its compact preview
opens the original image through Enlarge. Server listings are a captured
snapshot, rather than a live feed on the website.

The Multiplayer settings panel uses two unchanged user-supplied 1920×1080
JPEGs: `20261008200713_1.jpg` for the menu and `20261008200732_1.jpg` for
team/enemy colors during gameplay. Both offer full-resolution enlargement.
