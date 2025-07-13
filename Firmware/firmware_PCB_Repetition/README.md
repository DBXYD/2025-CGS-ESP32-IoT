# Rack v2 — ESP32-C3 SuperMini Power & Audio Router  
*(Firmware `firmware_PCB_Repetition`, ESP-IDF v5.4)*  

> **TL;DR**  
> • 3 × 230 V bistable relays + 6 × XLR router  
> • WS2812 feedback  
> • REST JSON API + mDNS  
 🔥  

---

## 1 . Fonctionnalités

| Bloc | Détails |
|------|---------|
| **Alimentation** | 3 relais Finder RT314F05, pilotés par un **PCF8575DBR** (I²C). |
| **Routage audio** | 6 relais bistables IM43GR commandés par un **ZXBM5210-SP** (GPIO). Modes : **DIRECT** ou **BUS A+B**. |
| **Retour visuel** | 5 LED WS2812 : <br>• LED 0-2 : état secteur (vert = ON / rouge = OFF) <br>• LED 3-4 : routage (bleu = BUS AB / jaune = DIRECT) |
| **Contrôles loc.** | • **POWER** : bouton GPIO10 (actif bas) <br>• **ROUTE** : bouton GPIO1 (front montant) |
| **API** | `/api/esp/status/` & `/api/esp/ping/` — JSON, mDNS `janusbot.local` ou IP fixe. |
| **Connectivité** | Wi-Fi STA, SNTP, mDNS. Power-save désactivé pour une latence mini. |

---

## 2 . Schéma des broches (SuperMini)

| GPIO | Rôle | Notes |
|------|------|-------|
| **5** | WS2812 din |
| **6** | ZXBM5210 REV (DIRECT) |
| **7** | ZXBM5210 FWD (BUS AB) |
| **8** | I²C SDA (PCF8575) |
| **9** | I²C SCL (PCF8575) |
| **10** | Bouton POWER (active LOW) |
| **1** | Bouton ROUTE (rising edge) |
| *autres* | nc / debug |

### PCF8575 → relais secteur

| PCF8575 bit | Action | Relais |
|-------------|--------|--------|
| P14 / P15 | SET / RST | Relais #1 |
| P16 / P17 | SET / RST | Relais #2 |
| P2  / P3  | SET / RST | Relais #3 |
| autres | NC |

---

## 3 . Firmware : structure

```

components/
├─ api/          ↳ REST client (esp\_http\_client)
├─ rack\_ctrl/    ↳ logiques relais / router / LEDs
├─ wifi/         ↳ init Wi-Fi (PS OFF, max TX power)
└─ config/       ↳ SSID, API endpoints, SNTP…
main/             ↳ app\_main(), watchdog & tasks

````

*Chaque séquence (ON/OFF, routage) pousse un `api_send_ping_full()` unique afin de ne pas spammer le backend.*

---

## 4 . Compilation & flash

```bash
git clone https://…/firmware_PCB_Repetition.git
cd firmware_PCB_Repetition
idf.py set-target esp32c3
idf.py menuconfig   # adapter SSID / mot de passe
idf.py -p /dev/ttyACM0 flash monitor
````

---

Rédigé par C.Janus le 13/07/2025