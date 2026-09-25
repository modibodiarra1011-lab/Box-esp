# Recipe engine

Le catalogue n'est pas duplique dans des milliers de fichiers. Le Builder combine:
1. board profile
2. composants dont `compatible_boards` intersecte la carte
3. protocol/pin constraints
4. bibliotheques connues
5. templates de code
6. recette de test

Une combinaison est marque `REQUIRES_DRIVER` lorsqu'un driver exact n'est pas connu. Le moteur ne doit jamais inventer une API.
