# Discord post for one game's release (run by zip-notify.yml on the push F7 makes):
#   ## D2x-SLikeNet / **New D2x-SLikeNet Release** / the pack and game zip links / **Update History**
# then "- Label: README title" for the entries new in this release plus the ones the previous
# release sent, so every post repeats the last updates. The same file is in D1s and D2s.
# usage: zip-notes.py <D1|D2> <game zip url> <models url>
import difflib, re, subprocess, sys

LABELS = {'Multiplayer': 'Multiplayer', 'Running several copies on one PC': 'Multiplayer', 'Observers': 'Multiplayer',
          'Demos': 'Demos', 'HUD and options': 'Hud', 'Features': 'Features', 'Options': 'Options',
          'Menu Options': 'Menu Options', 'DXA Graphics': 'DXA Graphics', 'BugFixes': 'BugFixes'}
ORDER = ['Multiplayer', 'Demos', 'Hud', 'Features', 'Options', 'Menu Options', 'DXA Graphics', 'BugFixes']

def git(*a):
    return subprocess.run(['git'] + list(a), capture_output=True, text=True, encoding='utf-8').stdout

def title(line):
    # No backticks: Discord would draw them as code boxes
    return re.sub(r'[.:]+$', '', re.match(r'- \*\*([^*]+)\*\*', line).group(1)).replace(chr(96), '').strip()

def sections(rev):
    # README title -> the "## " section it sits in
    section, where = None, {}
    for l in git('show', rev + ':README.md').splitlines():
        if l.startswith('## '): section = l[3:].strip()
        elif l.startswith('- **'): where.setdefault(title(l), section)
    return where

def new_titles(prev, cur):
    # Titles added between two commits. An added title that nearly matches a removed one in the same
    # section is a fixed typo, so it is not new; an entry moved to another section counts as new.
    removed, added = [], []
    for l in git('diff', prev, cur, '--', 'README.md').splitlines():
        if l.startswith('-- **'): removed.append(title(l[1:]))
        elif l.startswith('+- **'): added.append(title(l[1:]))
    if not added: return []
    was, where = sections(prev), sections(cur)
    for t in list(added):
        old = next((r for r in removed if was.get(r) == where.get(t) and
                    difflib.SequenceMatcher(None, r.lower(), t.lower()).ratio() >= 0.8), None)
        if old is not None:
            removed.remove(old)
            added.remove(t)
    return [(LABELS.get(where.get(t), where.get(t) or 'Other'), t) for t in dict.fromkeys(added)]

game, zip_url, models_url = sys.argv[1:4]
name = '%sx-SLikeNet' % game
zip_path = 'download/%sx-Redux-SLikeNet.zip' % game
# Release points, newest first: this push, then each earlier game-zip commit
head = git('rev-parse', 'HEAD').strip()
points = ['HEAD'] + [z for z in git('log', '-n', '20', '--format=%H', '--', zip_path).split() if z != head]
releases = [new_titles(points[k + 1], points[k]) for k in range(len(points) - 1)]
current = releases[0] if releases else []
previous = next((r for r in releases[1:] if r), [])  # what the last release with updates sent
# Repeat those only while the README still has them, labelled by the section they sit in now,
# so an entry moved to another section or renamed isn't posted under its old label
now = {t.lower(): s for t, s in sections(points[0]).items()}
previous = [(LABELS.get(now[t.lower()], now[t.lower()]), t) for _, t in previous if t.lower() in now]

rows = []
for row in current + previous:
    if row[1].lower() not in {t.lower() for _, t in rows}:
        rows.append(row)

msg = '\n'.join(['## ' + name, '**New %s Release**' % name,
                 '- Models-n-Textures: ' + models_url, '- %sx-Redux-SLikeNet: %s' % (game, zip_url),
                 '**Update History**'])
for label in ORDER + sorted({l for l, _ in rows} - set(ORDER)):
    for l, t in rows:
        if l != label: continue
        line = '\n- %s: %s' % (l, t)
        if len(msg) + len(line) > 1900:
            print(msg + '\n- ...and more in the README')
            sys.exit(0)
        msg += line
print(msg)
