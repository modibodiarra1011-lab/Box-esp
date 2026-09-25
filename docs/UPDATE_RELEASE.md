# Release / OTA

Le MASTER n'accepte une release distante que si :

1. le manifest est accessible en HTTPS ;
2. la version distante est supérieure ;
3. l'URL du firmware est HTTPS ;
4. la SHA-256 fait 64 caractères ;
5. l'image ne dépasse pas la partition OTA ;
6. la SHA-256 du fichier téléchargé correspond au manifest ;
7. l'image est écrite dans la prochaine partition OTA ;
8. la partition de boot est basculée ;
9. au redémarrage, l'application confirme son démarrage.

Le bootloader ESP-IDF peut effectuer un rollback automatique si l'application OTA ne confirme pas sa validité selon la configuration de rollback activée.

## Manifest minimal

```json
{
  "version": "4.1.1",
  "master_url": "https://example.invalid/releases/ESP32_LAB_MASTER_v4.1.1.bin",
  "master_sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
  "notes": "Correctifs et améliorations"
}
```

## Notifications

Une release disponible déclenche une notification WhatsApp si CallMeBot est configuré et une information sur le dashboard. L'installation est déclenchée seulement après approbation privée.
