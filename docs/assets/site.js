/* Buildless, GitHub Pages-compatible interactions. */
const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
const lightbox = document.querySelector('.lightbox');
if (lightbox && typeof lightbox.showModal === 'function') {
  document.querySelectorAll('[data-lightbox]').forEach(link => {
    link.addEventListener('click', event => {
      event.preventDefault();
      const source = link.querySelector('img') || link.closest('figure')?.querySelector('img');
      lightbox.querySelector('img').src = link.href;
      lightbox.querySelector('img').alt = source?.alt || 'R1Q2v2 screenshot';
      lightbox.querySelector('p').textContent = source?.alt || 'R1Q2v2 screenshot';
      lightbox.showModal();
    });
  });
  lightbox.querySelector('button').addEventListener('click', () => lightbox.close());
  lightbox.addEventListener('click', event => {
    const bounds = lightbox.getBoundingClientRect();
    if (event.target === lightbox && (event.clientX < bounds.left || event.clientX > bounds.right || event.clientY < bounds.top || event.clientY > bounds.bottom)) lightbox.close();
  });
}
const tabs = [...document.querySelectorAll('[role="tab"]')];
function selectTab(tab) {
  tabs.forEach(item => {
    const selected = item === tab;
    item.setAttribute('aria-selected', String(selected));
    item.tabIndex = selected ? 0 : -1;
    document.getElementById(item.getAttribute('aria-controls')).hidden = !selected;
  });
}
tabs.forEach((tab, index) => {
  tab.addEventListener('click', () => selectTab(tab));
  tab.addEventListener('keydown', event => {
    let next;
    if (event.key === 'ArrowRight' || event.key === 'ArrowDown') next = (index + 1) % tabs.length;
    if (event.key === 'ArrowLeft' || event.key === 'ArrowUp') next = (index + tabs.length - 1) % tabs.length;
    if (event.key === 'Home') next = 0;
    if (event.key === 'End') next = tabs.length - 1;
    if (next !== undefined) { event.preventDefault(); selectTab(tabs[next]); tabs[next].focus(); }
  });
});
const demos = [...document.querySelectorAll('[data-demo]')];
function setAnimation(demo, playing) {
  demo.dataset.playing = String(playing);
  demo.querySelector('img').src = playing ? demo.dataset.animation : demo.dataset.poster;
  const button = demo.querySelector('button');
  if (!button.dataset.label) button.dataset.label = button.getAttribute('aria-label').replace(/^Play /, '');
  button.textContent = playing ? 'Pause GIF  Ⅱ' : 'Play GIF  ▶';
  button.setAttribute('aria-pressed', String(playing));
  button.setAttribute('aria-label', `${playing ? 'Pause' : 'Play'} ${button.dataset.label}`);
}
demos.forEach(demo => {
  setAnimation(demo, false);
  demo.querySelector('button').addEventListener('click', () => {
    demo.dataset.manual = 'true';
    setAnimation(demo, demo.dataset.playing !== 'true');
  });
});
if ('IntersectionObserver' in window) {
  const observer = new IntersectionObserver(entries => {
    entries.forEach(({target, isIntersecting}) => {
      if (target.dataset.manual !== 'true') setAnimation(target, isIntersecting && !prefersReducedMotion.matches);
    });
  }, {threshold: 0.35});
  demos.forEach(demo => observer.observe(demo));
}
prefersReducedMotion.addEventListener?.('change', () => { if (prefersReducedMotion.matches) demos.forEach(demo => setAnimation(demo, false)); });
// Upgrade the verified release fallback when a newer release is available.
if (document.querySelector('[data-release-download]')) {
  fetch('https://api.github.com/repos/RENEGADE-ANDROiD/R1Q2v2/releases/latest', {headers: {'Accept': 'application/vnd.github+json'}})
    .then(response => { if (!response.ok) throw new Error('Release lookup unavailable'); return response.json(); })
    .then(release => {
      const zip = release.assets?.find(asset => /^R1Q2v2-\d+-win32\.zip$/i.test(asset.name));
      if (!zip || !zip.browser_download_url?.startsWith('https://github.com/RENEGADE-ANDROiD/R1Q2v2/releases/download/')) return;
      document.querySelectorAll('[data-release-download]').forEach(link => { link.href = zip.browser_download_url; });
      if (release.html_url?.startsWith('https://github.com/RENEGADE-ANDROiD/R1Q2v2/releases/tag/')) document.querySelectorAll('[data-release-notes]').forEach(link => { link.href = release.html_url; });
      const checksum = release.assets.find(asset => asset.name === `${zip.name}.sha256`);
      if (checksum?.browser_download_url?.startsWith('https://github.com/RENEGADE-ANDROiD/R1Q2v2/releases/download/')) document.querySelectorAll('[data-release-checksum]').forEach(link => { link.href = checksum.browser_download_url; });
    }).catch(() => { /* Verified static download links remain usable without the API. */ });
}
