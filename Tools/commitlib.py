"""Shared by the commit scripts: stage files plus CODEMAP edits applied to HEAD's CODEMAP (so other uncommitted CODEMAP
edits stay out of the commit), commit, and apply the same edits to the working copy."""
import subprocess
import sys

ROOT = r'C:\Dev\AI_Looter_Shooter'
TRAILER = '\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n'


def git(*args, input=None):
    r = subprocess.run(['git', *args], cwd=ROOT, input=input, capture_output=True)
    if r.returncode != 0:
        sys.stderr.write(r.stderr.decode('utf-8', 'replace'))
        raise SystemExit('git %s failed' % ' '.join(args))
    return r.stdout


def apply(text, edits):
    """Each edit is (old, new): old must occur exactly once. An edit with old None appends new after the anchor given as
    new[0] (old=None, new=(anchor, added))."""
    for old, new in edits:
        if old is None:
            anchor, added = new
            if text.count(anchor) != 1:
                raise SystemExit('CODEMAP anchor not found once: %r' % anchor[:90])
            text = text.replace(anchor, anchor + added)
            continue
        if text.count(old) != 1:
            raise SystemExit('CODEMAP edit not found once: %r' % old[:90])
        text = text.replace(old, new)
    return text


def commit(files, edits, title, body, removed=()):
    if git('diff', '--cached', '--name-only').strip():
        raise SystemExit('the index already has staged changes')
    head = git('show', 'HEAD:CODEMAP.md').decode('utf-8').replace('\r\n', '\n')
    staged_map = apply(head, edits) if edits else None
    # Check the working copy takes the same edits before touching the index.
    working = open(ROOT + '\\CODEMAP.md', encoding='utf-8').read().replace('\r\n', '\n')
    # An agent may have written its CODEMAP lines into the working copy already: those edits are skipped there.
    pending = [(old, new) for old, new in edits or ()
               if not (old is not None and old not in working and isinstance(new, str) and new in working)]
    working_new = apply(working, pending) if pending else working
    if files:
        git('add', '--', *files)
    if removed:
        git('rm', '-q', '--cached', '--', *removed)
    if staged_map is not None:
        blob = git('hash-object', '-w', '--stdin', '--path', 'CODEMAP.md', input=staged_map.encode('utf-8')).decode().strip()
        git('update-index', '--add', '--cacheinfo', '100644,%s,CODEMAP.md' % blob)
    git('commit', '-q', '-F', '-', input=(title + '\n\n' + body + TRAILER).encode('utf-8'))
    if edits and working_new != working:
        with open(ROOT + '\\CODEMAP.md', 'w', encoding='utf-8', newline='\n') as f:
            f.write(working_new)
    print(git('log', '-1', '--format=%h %s').decode().strip())
    print(git('show', '--stat', '--format=', 'HEAD').decode().strip().splitlines()[-1])
