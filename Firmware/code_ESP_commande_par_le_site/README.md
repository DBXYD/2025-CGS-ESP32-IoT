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

Rack Rainbow est une multiprise **6 sorties** commandées par relais RT314L, pilotée par un **ESP32-C3**.

* **Firmware ESP-IDF**
  – Séquences d’allumage/extinction (0 → 5 & 5 → 0)
  – Feedback visuel par **barre LED WS2812**
  – Bouton local pour toggler l’état global
  – Synchronisation HTTP(S) avec le serveur

* **Site Django 5**
  – Contrôle instantané (bouton « Allumer / Éteindre »)
  – Affichage état individuel des 6 appareils
  – Supervision connexion (« Dernier ping < 15 s »)

* **API REST** minimaliste (DRF) pour intégrations tierces

---

## Fonctionnalités

| Catégorie            | Détails                                                           |
| -------------------- | ----------------------------------------------------------------- |
| **Relais**           | 6 × RT314L commandés par PCF8575 (I²C)                            |
| **Indicateurs**      | 6 LED NeoPixel (vert = ON, rouge = OFF)                           |
| **Cycle soft-start** | Délai `STEP_DELAY_MS = 1000 ms` entre sorties                     |
| **Bouton physique**  | Front ANYEDGE : toggle → séquence complète                        |
| **Keep-alive**       | Ping JSON toutes 10 s hors séquence                               |
| **mDNS**             | Résolution `http://esp32.local`                                   |
| **Back-office**      | Authentification Django, historisation, filtrage par studio       |
| **Fail-safe**        | Pas de double-commande : flag partagé `busy` entre bouton & Wi-Fi |

---

## Architecture

```
ESP32-C3               ↔      Wi-Fi      ↔            Django + DRF
┌──────────────────┐              ┌─────────────────────────────────────┐
│  rack_ctrl.c     │ POST /ping →│ /api/esp/ping                       │
│  api_client.c    │ GET /state ←│ /api/esp/status                     │
└──────────────────┘              └─────────────────────────────────────┘
      ▲   ▲                                      ▲          ▲
      │   │ mDNS / IP fixe       WebSockets TODO │          │ Frontend JS
      │   └───────────────┐                    UI Super-user┘
NeoPixel + Relais   Bouton physique
```

---

## Matériel

| Réf.             | Rôle                              | Connexion     |
| ---------------- | --------------------------------- | ------------- |
| **ESP32-C3**     | MCU + Wi-Fi                       | —             |
| **PCF8575**      | Expander 16 bits SET/RESET relais | I²C @0x20     |
| **RT314L(F)05**  | Relais bistables 5 V × 6          | PCF bits 0-11 |
| **WS2812B**      | Barre 6 LED état sorties          | GPIO 8 (RMT)  |
| **Bouton**       | On/Off manuel                     | GPIO 10       |
| **Alim 5 V 3 A** | Bobines + LED                     | —             |

---

## Firmware

### Organisation

```
components/
 ├─ rack_ctrl/        séquences, bouton, I²C
 ├─ api_client/       GET /state, POST /ping
 ├─ http_client/      wrapper esp_http_client
 └─ config/           wifi.h, secrets.h
main/
 └─ app_main.c        boucle 5 s + heartbeat
```

### Build & flash

```bash
idf.py set-target esp32c3
idf.py menuconfig        # Wi-Fi & OTA
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Backend

* **Python 3.12 – Django 5 – DRF 3.15**
* Modèles : `StudioESP`, `StudioEspRackDevice`, `StudioEspDisplayDevice`
* Cron-like : déconnexion après 15 s sans ping.
* Fichiers clés : `website/views.py`, `website/templates/`.

### Lancer en local

```bash
poetry install
poetry run python manage.py migrate
poetry run python manage.py createsuperuser
poetry run python manage.py runserver 0.0.0.0:8000
```

Visiter `http://localhost:8000/controle/` (auth requise).

---

## Frontend

* **Bootstrap 5** + JS ES6 pur (`control.html`)
* Long-poll 5 s : `refreshAll()`
* Gestion `pending` pour retour visuel instantané

---

## API REST

| Méthode | Endpoint                      | Corps / Params        | Rôle                                        |
| ------- | ----------------------------- | --------------------- | ------------------------------------------- |
| GET     | `/api/esp/status?id=<pk>`     | —                     | État global + liste appareils               |
| POST    | `/api/esp/<id>/toggle/`       | CSRF                  | Demande ON ↔ OFF                            |
| POST    | `/api/esp/ping`               | `{name,state?,mask?}` | Heartbeat ou fin de séquence (ESP → Django) |
| GET     | `/api/esp/state?name=<alias>` | — *(ESP)*             | Consigne à appliquer (Django → ESP)         |


---

Rédigé par C.Janus le 10/07/2025
