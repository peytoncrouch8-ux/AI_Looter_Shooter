#!/bin/bash
# hashcheck.sh <HASHES.txt>: every listed original still matches Source (either "hash path" or "path hash" per line).
bad=0
while read -r a b rest; do
  [ -z "$a" ] && continue; case "$a" in \#*) continue;; esac
  if [[ "$a" =~ ^[0-9a-f]{40}$ ]]; then hash=$a; path=$b; else path=$a; hash=$b; fi
  [[ "$hash" =~ ^[0-9a-f]{40}$ ]] || continue
  [ -f "$path" ] || path="Source/AI_Looter_Shooter/$path"
  case "$path" in *Source/*) ;; *) continue;; esac
  [ "$(git hash-object "$path" 2>/dev/null)" != "$hash" ] && { echo "CHANGED $path"; bad=1; }
done < "$1"
echo "bad=$bad"; exit $bad
