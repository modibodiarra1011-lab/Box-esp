# Project Builder

Les templates et projets d'origine sont conservés dans `projects/PREPARED` et `projects/IMPORTED_35`.
Le script `scripts/build_ino.py` compile les `.ino` avec Arduino CLI et place les résultats dans `BUILD_OUTPUT/`.
Le script `scripts/prepare_sd.py` transforme ensuite ce résultat en structure de microSD.
