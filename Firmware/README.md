# 🎛️ ESP32 Music Room Automation – Firmware

Bienvenue dans le dépôt **Firmware** du projet de salles de musique connectées !  
Ce projet vise à automatiser et moderniser l'utilisation des salles de répétition grâce à des **PCB custom** et des **ESP32-C3 SuperMini** connectés en Wi-Fi.

## 🔧 Présentation du projet

Le système repose sur **trois types de cartes électroniques sur mesure**, chacune équipée d’un ESP32-C3, pour rendre l’usage des salles de musique plus simple, connecté et automatique.

### 🧠 1. PCB RACK

- 🗄️ **Fonction** : Contrôle des appareils audio installés dans un rack 19" (amplis, table de mixage, processeur, etc.).
- 🔌 **Contrôle** : Allumage/extinction des appareils via relais.
- 🌐 **Connectivité** : ESP32-C3 SuperMini connecté au Wi-Fi, commandable via site web ou API.

### 🎚️ 2. PCB RÉPÉTITION (Entrées / Sorties)

- 🚪 **Fonction** : Contrôle des équipements d’environnement dans la salle (prises, lumières, etc.).
- 🌐 **Connectivité** : ESP32-C3 SuperMini avec accès Wi-Fi pour pilotage distant.

### 🖥️ 3. PCB AFFICHAGE

- 📺 **Fonction** : Affichage dynamique dans la salle.
- 💡 **Éléments affichés** :
  - Heure actuelle
  - Barre de progression représentant l’avancement de la réservation
- 🔌 **Alimentation** : 5V (ESP32 + bandeau NeoPixel)
- 🌐 **Connectivité** :
  - Connexion Wi-Fi
  - Accès à Google Calendar pour synchronisation automatique des horaires
  - Allumage intelligent des équipements selon les réservations

## 📁 Contenu du dossier `Firmware/`

Firmware/  
├── pcb_rack/  
│   └── … (firmware pour les équipements rack)  
├── pcb_repetition/  
│   └── … (firmware pour les E/S de la salle)  
├── pcb_affichage/  
│   └── … (affichage LED + Google Calendar)  
├── lib/  
│   └── … (librairies partagées si nécessaires)  
└── web/  
    └── … (interface web améliorée avec Jungo)  


## 🌐 Interface Web

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

