# 🎶 Projet d’automatisation des salles de musique

Bienvenue ! Ce dépôt rassemble **tout** le nécessaire — électronique, firmware et mécano — pour rendre une salle de répétition entièrement connectée.

> **But :** concevoir trois cartes électroniques + leurs supports imprimés 3D et le code ESP32 associé pour piloter l’alimentation, l’affichage et la connectique d’un studio.

---

## 🗂️ Arborescence du dépôt

| Dossier       | Contenu                                                                                                    | Status        |
| ------------- | ---------------------------------------------------------------------------------------------------------- | ------------- |
| `hardware/`   | Schémas KiCad, PCB, BOM, README détaillé <br>(➡️ voir [`HARDWARE`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Hardware)) | ✅ Terminé |
| `firmware/`   | Code ESP32‑C3 (Wi‑Fi, MQTT/HTTP, logique de séquençage, etc.)   ➡️ voir [`FIRMWARE`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware)                                           | ✅ en cours |
| `mechanical/` | Pièces STL/STEP pour montage 19" et boîtiers muraux                                                        | 🚧 à démarrer   |
---

## 🛠️ Les trois cartes principales

### 1️⃣ **Rack Power Controller (RPC‑01)**

* Format **rack 19″ 1 U** ; se visse directement dans le châssis.
* **ESP32‑C3 SuperMini** connecté au LAN (Wi‑Fi ou Ethernet‑PHY en option).
* 8 sorties secteur sur relais bistables **RT34F05** (zéro consommation à l’état stable).
* Bouton **ON/OFF** en façade : enclenche / coupe l’alim avec séquençage chronométré (ex. : table ➜ +3 s ➜ ampli).
* LED status et anneau **NeoPixel** pour retour visuel.

### 2️⃣ **Wall Display & Timer (WDT‑01)**

* Plaque murale 120 × 80 mm imprimée 3D.
* Écran TFT 2,4″ + barre **NeoPixel 24 LED**.
* ESP32‑C3 : récupère l’heure (NTP/API Google) et l’info de réservation ; affiche le temps restant.
* Mode veille auto + capteur de présence (option).

### 3️⃣ **Stage Patch Hub (SPH‑01)**

* Module 2 U destiné à la scène : rassemble console, multipaire et amplis.
* Relais audio / DRV8428E pour commuter les retours, + prises XLR/TRS en face avant.
* ESP32‑C3 pour mise sous / hors tension et télémétrie température & courant.

---


## 📅 Feuille de route détaillée (6 semaines – 2 juin → 11 juillet 2025)



**Récapitulatif – Semaine 1**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Lun 02 (J1) | Kick‑off, revue cahier des charges, listing des composants et début projet Kicad |
| Mar 03 (J2) | Finalisation composants PCB, Rédaction README du GitHub et planning du projet |
| Mer 04 (J3) | Finalisation Schématique du PCB RACK, prise en compte des contraintes 3D  |
| Jeu 05 (J4) | Finalisation Routage du PCB RACK  |
| Ven 06 (J5) | Finalisation Schématique PCB salle de répétition, début routage  |


**Récapitulatif – Semaine 2**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Mar 10 (J6) | Finalisation routage du PCB salle de répétition, Début et fin du schématique et routage PCB affichage. Finalisation tableur pour les commandes des composants. |
| Mer 11 (J7) | Début Code ESP32. Finalisation du code pour connecter au wifi et obtenir l'heure. Passage de 2 couches à 4 couches pour le PCB des salles de répétition |
| Jeu 12 (J8) | Finalisation du projet [`ESP32-Google_Calendar`](hhttps://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/Projet_PCB_Affichage/Google_Calendar). Ajout des pinheaders libres sur les PCB (finalisation V2 des PCB).  |
| Ven 13 (J9) |   |



---





rédigé le 03/06/2025 -- Capodagli Janus
