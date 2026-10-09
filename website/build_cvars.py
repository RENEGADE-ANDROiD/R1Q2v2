"""Generate the buildless reference page from the repository's CVARS.md.

No external dependencies. GitHub Pages runs this on each documentation update.
"""
from pathlib import Path
import html
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'CVARS.md'
REPO = 'https://github.com/RENEGADE-ANDROiD/R1Q2v2/blob/master/'

def repair_encoding(text):
    # A few older notes contain UTF-8 interpreted as Windows-1252.
    replacements = {'â€”':'—', 'â€“':'–', 'â€™':'’', 'â€œ':'“', 'â€\x9d':'”', 'â€':'”', 'â€¦':'…', 'â†’':'→', 'â‰¥':'≥', 'Ã—':'×', 'Â':'', 'â€“':'–'}
    for before, after in replacements.items():
        text = text.replace(before, after)
    return text

def slug(text):
    return re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')

def inline(text):
    text = html.escape(text)
    tokens = []
    def keep(match):
        tokens.append('<code>' + match.group(2).strip() + '</code>')
        return f'@@TOKEN{len(tokens)-1}@@'
    text = re.sub(r'(`+)(.*?)\1', keep, text)
    def link(match):
        label, url = match.group(1), html.unescape(match.group(2))
        if not url.startswith(('https://', 'http://', '#')):
            url = REPO + url
        if not url.startswith(('https://', 'http://', '#')):
            return label
        return f'<a href="{html.escape(url, quote=True)}">{label}</a>'
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link, text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
    text = re.sub(r'(?<!\*)\*([^*]+)\*(?!\*)', r'<em>\1</em>', text)
    for index, value in enumerate(tokens):
        text = text.replace(f'@@TOKEN{index}@@', value)
    return text

def split_row(line):
    return [cell.strip() for cell in line.strip().strip('|').split('|')]

def render_markdown(text):
    lines = text.splitlines()
    output, headings, categories = [], [], []
    index, current_section, count = 0, 'introduction', 0
    while index < len(lines):
        line = lines[index].strip()
        if not line:
            index += 1
            continue
        if line.startswith('```'):
            code = []
            index += 1
            while index < len(lines) and not lines[index].strip().startswith('```'):
                code.append(lines[index])
                index += 1
            output.append('<pre><code>' + html.escape('\n'.join(code)) + '</code></pre>')
        elif line.startswith('# '):
            pass  # The page's hero supplies the document title.
        elif line.startswith('## '):
            title = line[3:]
            current_section = slug(title)
            headings.append((current_section, title))
            output.append(f'<h2 id="{current_section}" data-section="{current_section}">{inline(title)}</h2>')
        elif line.startswith('### '):
            output.append(f'<h3>{inline(line[4:])}</h3>')
        elif line.startswith('|') and index + 1 < len(lines) and re.match(r'^\|[\s:|\-]+\|$', lines[index + 1].strip()):
            headers = split_row(line)
            is_cvar = headers[0].lower() == 'cvar'
            table_class = 'cvar-table' if is_cvar else 'data-table'
            output.append(f'<div class="reference-table-wrap" data-section="{current_section}"><table class="{table_class}"><thead><tr>')
            output.extend('<th scope="col">' + inline(cell) + '</th>' for cell in headers)
            output.append('</tr></thead><tbody>')
            index += 2
            if is_cvar and current_section not in categories:
                categories.append(current_section)
            while index < len(lines) and lines[index].strip().startswith('|'):
                cells = split_row(lines[index])
                row_attrs = ''
                if is_cvar:
                    names = re.findall(r'`([^`]+)`', cells[0])
                    primary = names[0] if names else re.sub(r'[*`]', '', cells[0])
                    row_attrs = f' data-cvar="{html.escape(primary, quote=True)}" data-section="{current_section}" id="cvar-{current_section}-{slug(primary)}"'
                    count += 1
                output.append(f'<tr{row_attrs}>')
                for number, cell in enumerate(cells):
                    content = inline(cell)
                    if is_cvar and number == 0:
                        content += f'<button class="copy-cvar" type="button" data-copy="{html.escape(primary, quote=True)}" aria-label="Copy {html.escape(primary, quote=True)}">Copy name</button>'
                    output.append('<td>' + content + '</td>')
                output.append('</tr>')
                index += 1
            output.append('</tbody></table></div>')
            continue
        elif re.match(r'^[-*] ', line):
            output.append(f'<ul data-section="{current_section}">')
            while index < len(lines) and re.match(r'^[-*] ', lines[index].strip()):
                output.append('<li>' + inline(lines[index].strip()[2:]) + '</li>')
                index += 1
            output.append('</ul>')
            continue
        elif line == '---':
            pass
        else:
            paragraph = [line]
            index += 1
            while index < len(lines) and lines[index].strip() and not re.match(r'^(#|\||```|[-*] |---)', lines[index].strip()):
                paragraph.append(lines[index].strip())
                index += 1
            output.append(f'<p data-section="{current_section}">' + inline(' '.join(paragraph)) + '</p>')
            continue
        index += 1
    return '\n'.join(output), headings, categories, count

def build():
    text = repair_encoding(SOURCE.read_text(encoding='utf-8-sig'))
    content, headings, categories, count = render_markdown(text)
    sidebar = '\n'.join(f'<a href="#{key}">{html.escape(title)}</a>' for key, title in headings)
    titles = dict(headings)
    options = '\n'.join(f'<option value="{key}">{html.escape(titles[key])}</option>' for key in categories)
    home = (ROOT / 'docs/index.html').read_text(encoding='utf-8')
    support = re.search(r'(<section class="section support-section".*?</section>)', home, re.S).group(1)
    footer = re.search(r'(<footer class="site-footer">.*?</footer>)', home, re.S).group(1)
    page = f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><meta name="theme-color" content="#111311"><title>CVAR guide — R1Q2v2</title><meta name="description" content="Search R1Q2v2 console settings for display, graphics, HUD, crosshair, input, sound, networking and demos. Generated from CVARS.md."><link rel="canonical" href="https://renegade-android.github.io/R1Q2v2/cvars.html"><link rel="icon" href="assets/mark.svg"><link rel="stylesheet" href="assets/site.css"><script src="assets/cvars.js" defer></script></head>
<body class="reference-page"><a class="skip-link" href="#reference-content">Skip to reference</a><header class="site-header"><a class="brand" href="./"><img src="assets/mark.svg" width="32" height="32" alt=""><span>R1Q2<span class="brand-version">v2</span></span></a><nav aria-label="Main navigation"><a href="./#features">Features</a><a href="./#menus">Menus</a><a href="cvars.html" aria-current="page">CVAR guide ↗</a><a class="nav-download" href="./#download">Download ↓</a></nav></header>
<main><section class="reference-hero"><p class="eyebrow">THE CONSOLE / REFERENCE</p><h1>Your settings.<br>Your way.</h1><p>Search useful R1Q2v2 console variables, learn what they do, and find their defaults. This page is generated from the repository’s CVARS.md. It covers commonly useful settings, rather than every engine variable.</p><a class="text-link" href="{REPO}CVARS.md">View the original CVARS.md ↗</a></section>
<form class="reference-toolbar" role="search" id="cvar-search-form"><div><label for="cvar-search">FIND A SETTING</label><input type="search" id="cvar-search" name="q" placeholder="Try crosshair, widescreen, gl_less_yellow…" autocomplete="off" aria-controls="reference-content"></div><div><label for="cvar-category">CATEGORY</label><select id="cvar-category" name="category" aria-controls="reference-content"><option value="">All categories</option>{options}</select></div><p class="search-meta" id="search-status" role="status" aria-live="polite">{count} documented settings. Defaults and notes from CVARS.md.</p></form>
<div class="reference-layout"><nav class="reference-sidebar" aria-label="Reference sections">{sidebar}</nav><article class="reference-content" id="reference-content">{content}<p class="empty-search" id="empty-search" hidden>No matching settings. Try a shorter name or choose another category.</p></article></div>
{support}</main>{footer}</body></html>'''
    (ROOT / 'docs/cvars.html').write_text(page, encoding='utf-8')
    print(f'Generated docs/cvars.html: {count} settings, {len(categories)} searchable categories.')

if __name__ == '__main__':
    build()
