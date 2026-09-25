# Agent, Internet et WhatsApp

Le système est conçu pour fonctionner en mode local sans IA distante. La connexion Internet est une option administrable.

## Configuration depuis le panneau privé

- SSID STA
- mot de passe STA
- numéro WhatsApp
- clé CallMeBot
- endpoint IA compatible OpenAI-style
- clé IA
- endpoint de recherche
- manifest OTA HTTPS

Les champs secrets ne sont jamais renvoyés par l'API de lecture de configuration.

## Test WhatsApp

Le bouton **Envoyer « coucou » WhatsApp** appelle l'endpoint CallMeBot avec HTTPS. Le résultat HTTP est retourné au panneau privé.

## Notifications OTA

Avant une mise à jour disponible :

```text
release détectée
      ↓
dashboard
      ↓
WhatsApp (si configuré)
      ↓
approbation privée
      ↓
SHA-256
      ↓
OTA
```

## Mémoire

Les événements de l'agent sont ajoutés sur SD dans :

```text
/sd/AI/memory.jsonl
```

Cela constitue une mémoire opérationnelle (historique), pas un entraînement automatique d'un modèle IA.
