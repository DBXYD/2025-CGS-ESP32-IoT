```markdown
# 🌈 Rack Rainbow – Multiprise connectée 6 × RT314L (ESP32-C3)  
> Pilotage temps-réel depuis un site Django **ET** via un bouton physique

---

## Table of Contents
1. [Aperçu](#aperçu)
2. [Fonctionnalités](#fonctionnalités)
3. [Architecture](#architecture)
4. [Matériel](#matériel)
5. [Firmware (ESP-IDF v5.4)](#firmware)
6. [Backend Django](#backend)
7. [Frontend](#frontend)
8. [API REST](#api-rest)
9. [Déploiement rapide](#déploiement-rapide)
10. [Roadmap](#roadmap)
11. [Crédits & Licence](#crédits--licence)

---

## Aperçu
Rack Rainbow est une multiprise **6 sorties** commandées par relais RT314L,
pilotée par un **ESP32-C3**.  
Le projet fournit :

* Un **firmware ESP-IDF** :  
  – séquences d’allumage/​extinction (0 → 5 & 5 → 0)  
  – courriel visuel par **barre LED WS2812**  
  – bouton local pour toggler l’état global  
  – synchronisation HTTP(S) avec le serveur
* Un **site Django 5** permettant :  
  – contrôle instantané (bouton « Allumer / Éteindre »)  
  – affichage état individuel des 6 appareils  
  – supervision connexion (“Dernier ping < 15 s”)  
* Une **API REST** minimaliste (DRF) pour d’autres intégrations.

<p align="center">
  <img src="docs/assets/demo.gif" width="650" alt="Demo Rack Rainbow">
</p>

---

## Fonctionnalités
| Catégorie                | Détails                                                                    |
|--------------------------|----------------------------------------------------------------------------|
| **Relais**               | 6 RT314L commandés par un PCF8575 (I²C)                                    |
| **Indicateurs**          | 6 LED NeoPixel / WS2812 ↗ vert = ON, rouge = OFF                           |
| **Cycle soft-start**     | Délai `STEP_DELAY_MS = 1000 ms` entre sorties pour protéger l’alim         |
| **Bouton physique**      | Front ANYEDGE : toggle ↗ séquence complète                                 |
| **Keep-alive**           | Ping JSON toutes 10 s hors séquence (`{"name":"Rack_Rainbow","state":…}`)  |
| **mDNS**                 | Résolution `http://esp32.local`                                           |
| **Back-office Django**   | Authentification, historisation, filtrage par studio                       |
| **Auto-découverte**      | Adresse IP mise à jour à chaque ping (esp → site)                          |
| **Fail-safe**            | Pas de double-commande : flag `busy` partagé entre bouton & Wi-Fi          |

---

## Architecture
```

ESP32-C3                ↔        Wi-Fi        ↔          Django + DRF
┌──────────────────┐                 ┌─────────────────────────────┐
│  rack\_ctrl.c     │  POST /ping →  │  /api/esp/ping              │
│  api\_client.c    │  GET  /state ← │  /api/esp/status            │
└──────────────────┘                 └─────────────────────────────┘
▲   ▲                                        ▲       ▲
│   │ mDNS/Static IP       WebSockets (TODO) │       │ Frontend JS
│   └───────────────┐                        │       └─ control.html
│                   │                        │
NeoPixel + Relais   Bouton physique         Super-user UI

```

---

## Matériel
| Réf.            | Rôle                              | Pin / Bus     |
|-----------------|-----------------------------------|---------------|
| **ESP32-C3**    | MCU + Wi-Fi                       | —             |
| PCF8575         | Expander 16 bits (relais SET/RST) | I²C (0x20)    |
| 6 × RT314L(F)05 | Relais bistables 5 V              | PCF bits 0-11 |
| Barre WS2812B   | Indication état sorties           | GPIO 8        |
| Bouton poussoir | On/Off manuel                     | GPIO 10       |
| Alim 5 V 3 A    | Requis pour bobines + LED         | —             |

---

## Firmware
### Dossiers clés
```

components/
├─ rack\_ctrl/      log., I²C, séquences, bouton
├─ api\_client/     GET /state, POST /ping
├─ http\_client/    wrapper esp\_http\_client
└─ config/         wifi.h, secrets.h
main/
└─ app\_main.c      boucle 5 s + heartbeat

````

### Build & flash
```bash
idf.py set-target esp32c3
idf.py menuconfig      # configure Wi-Fi & OTA
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
````

---

## Backend

* **Python 3.12 · Django 5 · djangorestframework 3.15**
* Modèles :
  `StudioESP`, `StudioEspRackDevice`, `StudioEspDisplayDevice`
* Cron‐like (`reset_all_esp_connection_status`) coupe la connexion après
  15 s sans ping.
* Fichiers essentiels :
  `website/views.py`, `website/serializers.py`, `website/templates/`.

### Lancer en local

```bash
poetry install
poetry run python manage.py migrate
poetry run python manage.py createsuperuser
poetry run python manage.py runserver 0.0.0.0:8000
```

Visiter `http://localhost:8000/controle/` (auth requis).

---

## Frontend

* Template **Bootstrap 5**, un seul fichier `control.html`.
* JS pur ES6 (pas de React/Vue) :
  – long-poll 5 s (`refreshAll`)
  – gestion état/pending bouton
  – rendu dynamique des 6 devices.

---

## API REST

| Méthode | Endpoint                      | Corps / Params        | Rôle                                        |
| ------- | ----------------------------- | --------------------- | ------------------------------------------- |
| `GET`   | `/api/esp/status?id=<pk>`     | —                     | État global + liste appareils               |
| `POST`  | `/api/esp/<id>/toggle/`       | CSRF token            | Demande ON↔OFF                              |
| `POST`  | `/api/esp/ping`               | `{name,state?,mask?}` | Heartbeat ou fin de séquence (ESP → Django) |
| `GET`   | `/api/esp/state?name=<alias>` | — *(ESP)*             | Consigne à appliquer (Django → ESP)         |

---

## Déploiement rapide

1. **Backend** : Fly.io / Railway / Docker on-prem.
2. **ESP** : flasher OTA via `esp_https_ota()` ou port série TTL.
3. **DNS** : sous-domaine `rack.example.com`, certificat Let’s Encrypt.
4. **Firewall** : n’ouvrir que le port 443 sortant → API publique.

---

## Roadmap

* [ ] Passage au **WebSocket** pour push en temps réel
* [ ] Mesure **courant/puissance** via INA219
* [ ] Interface mobile « Dark Mode » complète
* [ ] Support multiprise **8 sorties** (PCF8574 + ULN2803)

---

## Crédits & Licence

* **Janus** – firmware & hardware
* **Ton nom** – backend Django / frontend
* Logo “Covent Garden” © 2025 Studio CGS

Code sous licence **MIT** (voir `LICENSE`).

> ✨ Have fun & hack safely!

```
::contentReference[oaicite:0]{index=0}
```
