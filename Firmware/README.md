# 🎛️ ESP32 Music Room Automation – Firmware

Bienvenue dans le dépôt **Firmware** du projet de salles de musique connectées !  
Ce projet vise à automatiser et moderniser l'utilisation des salles de répétition grâce à des **PCB custom** et des **ESP32-C3 SuperMini** connectés en Wi-Fi.

## 🔧 Présentation du projet

Le système repose sur **trois types de cartes électroniques sur mesure**, chacune équipée d’un ESP32-C3, pour rendre l’usage des salles de musique plus simple, connecté et automatique.

### 🧠 1. [**PCB RACK**](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/firmware_PCB_Rack)

- 🗄️ **Fonction** : Contrôle des appareils audio installés dans un rack 19" (amplis, table de mixage, processeur, etc.).
- 🔌 **Contrôle** : Allumage/extinction des appareils via relais.
- 🌐 **Connectivité** : ESP32-C3 SuperMini connecté au Wi-Fi, commandable via site web ou API.

### 🎚️ 2. [**PCB RÉPÉTITION**](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/firmware_PCB_Repetition)

- 🚪 **Fonction** : Contrôle des équipements (amplis) et le couplage entré/sorties des signaux audio.
- 🔌 **Contrôle** : Allumage/extinction des amplificateurs et contrôl des couplage entrées sorties XLR via relais.
- 🌐 **Connectivité** : ESP32-C3 SuperMini avec accès Wi-Fi pour pilotage distant.

### 🖥️ 3. [**PCB AFFICHAGE**](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/firmware_PCB_Affichage)

- 📺 **Fonction** : Affichage dynamique dans la salle.
- 💡 **Éléments affichés** :
  - Heure actuelle
  - Barre de progression représentant l’avancement de la réservation
- 🔌 **Alimentation** : 5V (ESP32 + bandeau NeoPixel)
- 🌐 **Connectivité** :
  - Connexion Wi-Fi pour obtenir l'heure.
  - Accès à Google Calendar pour synchronisation automatique des horaires


## 📁 Contenu du dossier `Firmware/`

Firmware/  
├── Django_dir/  
│   └── … (Projets Django)  
├── firmware_PCB_Affichage/  
│   └── … (affichage LED + Google Calendar)  
├── firmware_PCB_Rack/  
│   └── … (firmware pour l'alimentation des équipements rack)  
├── firmware_PCB_Repetition/  
│   └── … (firmware pour alimentation et les E/S de la salle)  
├── Tests_firmware/  
│   └── … (Projet de test)  
└──   


## 🌐 [**Interface Web**](https://github.com/DBXYD/2025-CGS-ESP32-IoT/tree/master/Firmware/Django_dir)

Une interface web (hébergée localement ou sur le réseau local) permet :  
- Le contrôle des appareils à distance  
- La visualisation de l’état des équipements  
- La configuration et les mises à jour  

L’interface est construite avec **Django** pour plus de simplicité et de modernité.



## 🎯 Objectifs

- ✅ Automatiser la gestion des appareils d'une salle de musique  
- ✅ Afficher dynamiquement l’état et l’horaire en salle  
- ✅ Réduire la consommation d’énergie et les oublis d’allumage/extinction  
- ✅ Offrir un système simple, modulaire, réutilisable  



---
Rédigé par Capodagli Janus - 12/06/22025

