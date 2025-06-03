# 🚀 Bienvenue dans la section HARDWARE

Ce projet a pour objectif de concevoir jusqu’à **trois cartes électroniques (PCB)** et des pièces **imprimées en 3D** permettant d’automatiser et de connecter les salles de musique.

---

## 📌 Cahier des charges

### 🔹 Unité « Robot »

* Connexion Wi‑Fi assurée par un **ESP32‑C3 SuperMini**
* Alimentation secteur **centralisée** et sécurisée
* Allumage ciblé et temporisé des différents équipements de la salle à distance
* Affichage d’état et repères visuels (NeoPixels, LED, etc.)

---

## ⚙️ Matériel utilisé

| Composant                | Référence            | Rôle                                                |
| ------------------------ | -------------------- | --------------------------------------------------- |
| **Microcontrôleur**      | ESP32‑C3 SuperMini   | Connexion réseau & logique de commande              |
| **Relais SPDT bistable** | RT34F05              | Mise sous/hors tension des équipements              |
| **Expandeur d’E/S**      | PCF8575              | Bus I²C → sorties GPIO vers drivers & indicateurs   |
| **Driver de puissance**  | DRV8428E             | Double pont H pour piloter deux relais (ou moteurs) |
| **Module AC/DC**         | Mornsun MP‑LDE‑20B05 | 230 VAC → 5 VDC isolé                               |
| **LED adressables**      | WS2812B (NeoPixels)  | Indication visuelle & rétro‑éclairage               |

---

## 📝 Notes techniques

* Toutes les lignes de commande haute‑puissance sont isolées de la logique 3,3 V via les relais bistables et/ou le DRV8428E.
* Le PCF8575 étend le nombre de GPIO disponibles : il commande à la fois les relais et les canaux d’affichage lumineux.
* Les PCB seront empilables afin de faciliter la maintenance et l’évolutivité.
* Les boîtiers 3D prévoient la circulation d’air et des points de fixation normalisés type rack 19″.

---

## 🔄 Prochaines étapes

1. Finaliser le schéma électronique de la carte « Power » (relais + DRV8428E).
2. Déterminer la consommation totale pour choisir la bonne section des pistes et le dissipateur du MP‑LDE‑20B05.
3. Prototyper l’ensemble avec un banc d’essai (ESP32 + breadboard) pour valider le firmware de base.
4. Concevoir les boîtiers et guides‑câbles en impression 3D (PET‑G ou ASA).

---

*Document rédigé : 3 juin 2025*
