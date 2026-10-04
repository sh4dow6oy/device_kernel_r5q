#!/bin/bash
set -e

echo "Colectăm commiturile de la 2026-04-27 încolo..."
COMMITS=$(git log alfcraft/oneui/android16 --since="2026-04-27" --reverse --format="%H")

for commit in $COMMITS; do
    echo "=================================================="
    echo "Aplicăm commitul: $commit"
    echo "=================================================="
    git cherry-pick "$commit" || {
        echo ""
        echo "[!] CONFLICT DETECTAT la commitul $commit!"
        echo "Rezolvă conflictul, dă 'git add <fișiere>', apoi rulează 'git cherry-pick --continue' și reia scriptul."
        exit 1
    }
done

echo "Toate commiturile din data specificată s-au aplicat cu succes!"
