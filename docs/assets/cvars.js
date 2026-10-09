const search = document.getElementById('cvar-search');
const category = document.getElementById('cvar-category');
const rows = [...document.querySelectorAll('[data-cvar]')];
const content = document.getElementById('reference-content');
const status = document.getElementById('search-status');
const params = new URLSearchParams(location.search);
search.value = params.get('q') || '';
category.value = params.get('category') || '';
const searchableText = new Map(rows.map(row => [row, row.textContent.toLowerCase()]));
function filterSettings() {
  const query = search.value.trim().toLowerCase();
  const selected = category.value;
  const filtered = Boolean(query || selected);
  let matches = 0;
  rows.forEach(row => {
    const visible = (!selected || row.dataset.section === selected) && (!query || searchableText.get(row).includes(query));
    row.hidden = !visible;
    if (visible) matches++;
  });
  const matchingSections = new Set(rows.filter(row => !row.hidden).map(row => row.dataset.section));
  [...content.children].forEach(element => {
    if (element.id === 'empty-search') return;
    if (!filtered) { element.hidden = false; return; }
    if (element.classList.contains('reference-table-wrap')) {
      element.hidden = !element.querySelector('.cvar-table') || ![...element.querySelectorAll('[data-cvar]')].some(row => !row.hidden);
    } else {
      element.hidden = element.tagName !== 'H2' || !matchingSections.has(element.dataset.section);
    }
  });
  document.getElementById('empty-search').hidden = matches > 0;
  status.textContent = filtered ? `${matches} matching ${matches === 1 ? 'setting' : 'settings'} of ${rows.length}.` : `${rows.length} documented settings. Defaults and notes from CVARS.md.`;
  const next = new URL(location.href);
  query ? next.searchParams.set('q', search.value.trim()) : next.searchParams.delete('q');
  selected ? next.searchParams.set('category', selected) : next.searchParams.delete('category');
  history.replaceState(null, '', next);
}
search.addEventListener('input', filterSettings);
category.addEventListener('change', filterSettings);
document.getElementById('cvar-search-form').addEventListener('submit', event => event.preventDefault());
filterSettings();
document.querySelectorAll('[data-copy]').forEach(button => {
  button.addEventListener('click', async () => {
    try {
      await navigator.clipboard.writeText(button.dataset.copy);
      button.textContent = 'Copied';
      setTimeout(() => { button.textContent = 'Copy name'; }, 1500);
    } catch {
      button.textContent = 'Select to copy';
      const selection = window.getSelection();
      const range = document.createRange();
      range.selectNodeContents(button.parentElement.querySelector('code'));
      selection.removeAllRanges(); selection.addRange(range);
    }
  });
});
