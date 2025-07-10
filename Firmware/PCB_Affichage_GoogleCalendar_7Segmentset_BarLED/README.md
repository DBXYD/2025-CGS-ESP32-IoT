# 📟 Horloge NeoPixel + Barre de progression Google Calendar (ESP32-C3)

> Projet ESP-IDF v5.4 – 96 LED WS2812B en série

---

## 1 ▪ Présentation

Une **horloge connectée Wi-Fi** pilotée par un PCB custom **ESP32-C3 Supermini** et un seul ruban de  
**96 NeoPixels** :

| Zone | LED | Rôle |
|------|-----|------|
| Chiffre 0 | 0 – 20 | Dizaines d’heures (7 segments × 3 LED) |
| Chiffre 1 | 21 – 41 | Unités d’heures |
| Deux-points | 42 – 43 | « : » qui clignote chaque seconde |
| Chiffre 2 | 44 – 64 | Dizaines de minutes |
| Chiffre 3 | 65 – 85 | Unités de minutes |
| **Barre** | **86 – 95** | 10 LED indiquant l’avancement de l’évènement **Google Calendar en cours** |

La barre démarre **entièrement verte** ; au fur et à mesure, les LED passent **violet** de  
**gauche (LED 95) → droite (LED 86)**.

---

## 2 ▪ Fonctionnalités

* Mise à l’heure automatique via **SNTP** (`pool.ntp.org`).
* Authentification **OAuth 2 Device Code Flow** (validation une seule fois).  
  Le *refresh token* est conservé en NVS.
* Lecture du **calendrier primaire Google** et détection de l’évènement couvrant l’instant présent.
* Retours visuels :
  * LED embarquée qui clignote pendant un évènement.
  * Barre 10 LED indiquant le pourcentage écoulé.
* Pilotage du ruban avec le composant **`espressif/led_strip` v3** (RMT, pas de bit-bang).
* Architecture modulaire : `wifi/`, `oauth/`, `calendar/`, `led_display/`, etc.

---

## 3 ▪ Matériel

| Élément | Détails |
|---------|---------|
| **PCB Afficheur**  | Inclus ESP32 C3 Supermini, condo 100nF, Résistance et LED verte, entrée alim et sortie alim+commande du ruban LED |
| 96 × **WS2812B** | par ex. 1 m / 144 LED/m recoupé (3 LEDs/segment, 1/point, 10/barre) |
| Alim 5 V / ≥ 2 A | commune carte + ruban |

```

ESP32-C3          Ruban WS2812B

---

5 V   ───────────► 5 V
GND  ───────────► GND
GPIO 5 ───────────► DIN

````
*(LED 0 = première LED après DIN)*

---

## 4 ▪ Compilation & flash

```bash
# 1) Cloner
git clone https://github.com/votre-login/esp32c3-neopixel-clock
cd esp32c3-neopixel-clock

# 2) Installer/activer ESP-IDF 5.4
. $HOME/esp/esp-idf/export.sh

# 3) Configurer (Wi-Fi, identifiants Google…)
idf.py menuconfig          # ou éditer components/config/config.h

# 4) Compiler & flasher
idf.py -p /dev/ttyACM0 flash monitor
````

### Paramétrage Google OAuth 2

1. Dans Google Cloud Console, créer un **Client OAuth “TV and Limited Input device”**.
2. Copier **Client ID** et **Client Secret** dans `config.h`.
3. Au premier démarrage, le moniteur série affiche une *URL de vérification* et un *code*.
   Visiter l’URL, saisir le code et autoriser l’accès en lecture au calendrier.
4. Le *refresh token* est sauvegardé ; les démarrages suivants sont automatiques.

---

## 5 ▪ Arborescence (résumé)

```
components/
 ├─ wifi/           Connexion Wi-Fi + SNTP
 ├─ oauth/          Device-flow & refresh
 ├─ calendar/       Wrapper REST Google Calendar
 ├─ led_display/    Mappage 7-segments et dessin
 ├─ time_utils/     Pour obtenir l'heure local
 └─ app_state/      Variables partagées (event_active…)
main/
 └─ PCB_Affichage_GoogleCalendar_et_BarLED.c     3 tâches RTOS : gcal / display / blink
```

---

## 6 ▪ Personnalisation

| Pour changer…         | Modifier…                                                     |
| --------------------- | ------------------------------------------------------------- |
| Couleurs / luminosité | constantes RGB dans `display_task()`                          |
| Ordre des LED         | fonction `digit_base()` et indices barre dans `led_display.c` |
| Fuseau horaire        | chaîne `TZ` dans `app_main()`                                 |

---

Rédigé par C.Janus le 10/07/2025