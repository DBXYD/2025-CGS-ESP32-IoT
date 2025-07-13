# 🎶 Projet d’automatisation des salles de musique

Bienvenue ! Ce dépôt rassemble **tout** le nécessaire — électronique, firmware et mécano — pour rendre une salle de répétition entièrement connectée.

> **But :** concevoir trois cartes électroniques + leurs supports imprimés 3D et le code ESP32 associé pour piloter l’alimentation, l’affichage et la connectique d’un studio.

---

## 🗂️ Arborescence du dépôt

| Dossier       | Contenu                                                                                                    | Status        |
| ------------- | ---------------------------------------------------------------------------------------------------------- | ------------- |
| `hardware/`   | Schémas KiCad, PCB, BOM, README détaillé <br>(➡️ voir [`HARDWARE`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Hardware)) | ✅ Terminé |
| `firmware/`   | Code ESP32‑C3 (Wi‑Fi, MQTT/HTTP, logique de séquençage, etc.), et amélioration/adaptation du site avec Django   ➡️ voir [`FIRMWARE`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware)                                           | ✅ Terminé |
| `mechanical/` | Pièces STL des 3D rack et afficheur, fichiers DXF pour le boitier mural    ➡️ voir [`MECHANICAL`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Mechanical)                                                        | ✅ Terminé    |
---

## 🛠️ Les trois cartes principales

### 1️⃣ **Rack Power Controller**

* 3D format **rack 19″ 1 U** ; se visse directement dans le châssis.
* **ESP32‑C3 SuperMini** connecté au LAN (Wi‑Fi).
* 1 entrée secteur pour alimenter les appareils.
* 6 sorties secteur sur relais bistables **RT34F05** (zéro consommation à l’état stable).
* Bouton **ON/OFF** en façade : enclenche / coupe l’alim avec séquençage chronométré (ex. : table ➜ +3 s ➜ ampli).
* Bouton d'arrêt urgent pour couper l'alimentation secteur des appareils.
* LEDs **NeoPixel** pour retour visuel (vert=alimenté; rouge=non alimenté).

### 2️⃣ **Wall Display & Timer**

* Boitier mural 500 x 280 × 90 mm découpé au laser.
* afficheur 7 segment et barre fait à partie de ruban de néopixels (96 en tout/boitier).
* ESP32‑C3 : récupère l’heure (NTP/API Google) et l’info de réservation ; affiche le temps restant avec une barre de 10 néopixels.

### 3️⃣ **Contrôle entrées/sorties**

* Module 2 U destiné à la scène : alimente les trois amplificateurs, et commute les sorties depuis le site web. 
* Relais signaux / DRV8428E pour commuter les retours, + prises XLR/TRS en face avant.
* ESP32‑C3 pour mise sous / hors tension et contrôler les sorties.

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
| Jeu 12 (J8) | Finalisation du projet [`ESP32-Google_Calendar`](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/Tests_firmware/PCB_Affichage_Google_Calendar). Ajout des pinheaders libres sur les PCB (finalisation V2 des PCB). Ajout et MAJ des README |
| Ven 13 (J9) |  Réalistion de la liste des commandes pour les composants, Début code combinant la lecture du Google Calendar avec le contrôl des Néopixels|


**Récapitulatif – Semaine 3**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Lun 16 (J10) | Relectures et correstions eds PCB, amélioration du code Google_Calendar pour une reconnexion automatique après reboot |
| Mar 17 (J11) | Début apprentissage Django|
| Mer 18 (J12) | Fin apprentissage Django et début test Django avec le site   |
| Jeu 19 (J13) | Ajout des connexions ESP avec le site, affichage des état de connexion et de foncitonnement des esp32   |
| Ven 20 (J14) | Ajout des contrôles des ESP32 : allumer éteindre la LED bleue |

**Récapitulatif – Semaine 4**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Lun 23 (J15) | Début de la mise en place d'une nouvelle base de donnée du site pour l'adapter aux ESP32, création des pages "contrôl" et "Gestion Réservations" |
| Mar 24 (J16) | Mise en place de la nouvelle base de donnée. Bouton "Clean Calendar" permettant deremplir les crénaux vide et supprimer les évènements sans titre automatiquement |
| Mer 25 (J17) | Début modélisation 3D pour l'afficheur et la bar LED   |
| Jeu 26 (J18) | Finalisation 3D pour l'afficheur 7 segments, début 3D pour le PCB Rack. Début impresssions et tests.   |
| Ven 27 (J19) | Ajout des contrôles des ESP32 : allumer éteindre la LED bleue |

**Récapitulatif – Semaine 5**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Lun 30 (J15) | Début 3D du PCB Répétition, impression V1 de la 3D Rack |
| Mar 01 (J16) | Soudure afficheur 7 segments |
| Mer 02 (J17) | Modifications et finalisation de la 3D du PCB Rack   |
| Jeu 03 (J18) | Rédaction code pour contrôler le PCB Rack alimenté en 230VAC   |
| Ven 04 (J19) | finalisation boitier de l'afficheur |

**Récapitulatif – Semaine 6**

| Jour        | Tâche principale                   |
| ----------- | ---------------------------------- |
| Lun 07 (J15) | Soudure afficheur 7 segments, finalisation firmware PCB afficheur avec google calendar |
| Mar 08 (J16) | Soudure des PCB afficheur restant, finalisation firmware PCB Rack appliqué au site |
| Mer 09 (J17) | Soudure des PCB Rack restant   |
| Jeu 10 (J18) | Finalisation de la 3D du PCB de la salle de répétition   |
| Ven 11 (J19) | Rengement, développement firmware PCB repetition, finalisation et assemblage boitier afficheur   |

---





rédigé le 03/06/2025 -- Capodagli Janus
