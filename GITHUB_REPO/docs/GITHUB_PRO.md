# Publier ESP32 LAB sur GitHub

## Créer le dépôt

1. Créer un nouveau dépôt vide sur GitHub.
2. Ne pas ajouter de README distant si le dépôt local en contient déjà un.
3. Depuis la racine du projet :

```powershell
git init
git add .
git status
git commit -m "feat: ESP32 LAB initial IDF 6.1 release"
git branch -M main
git remote add origin https://github.com/<USER>/<REPO>.git
git push -u origin main
```

## Contrôler avant publication

```powershell
python scripts\verify_project.py
python scripts\verify_100k.py
```

Puis :

```powershell
git grep -n -I -E "AP_PASSWORD_PLACEHOLDER|CALLMEBOT_API_PLACEHOLDER|AI_KEY_PLACEHOLDER"
```

Cette commande doit rester vide pour les secrets réels.

## CI

`.github/workflows/ci.yml` est prévue pour :

- Python/JSON/static checks ;
- ESP-IDF v6.1 ;
- cible `esp32s3` ;
- build du MASTER.

## Releases

Après avoir réellement compilé le firmware :

```powershell
python scripts\release.py --version 4.2.0 --bin .\build\esp32_lab_master.bin --url https://github.com/<USER>/<REPO>/releases/download/v4.2.0/esp32_lab_master.bin
```

Le script génère le manifest et la SHA-256 utilisée par l'OTA.
