# 🎮 Tetris Online – C++ / Raylib / TCP

Projet de Tetris en C++ comprenant :
- un mode **solo**
- un mode **multijoueur en ligne (1v1)** via TCP
- une interface graphique réalisée avec **Raylib**
- un serveur dédié pour le matchmaking et la synchronisation

Projet réalisé dans le cadre du module **IN204 – ENSTA Paris**.

---

## ✨ Fonctionnalités

- 🎮 Mode solo classique
- 🌐 Mode en ligne (2 joueurs)
- ⏸️ Pause désactivée en ligne
- 🎵 Musique et effets sonores
- 📊 Score, niveau, prochaine pièce
- 🔌 Serveur TCP multi-clients
- 🧵 Communication réseau multi-threadée

---

## 🛠️ Technologies utilisées

- **Langage** : C++17  
- **Graphique / Audio** : Raylib  
- **Réseau** : TCP (sockets POSIX)  
- **Build** : CMake  
- **OS** : Linux / Windows (via WSL)

---

## 📁 Structure du projet

```text
tetris_online/
 ├── client/        # Client graphique (Raylib)
 ├── server/        # Serveur TCP
 ├── docs/          # Guide d'installation (PDF + LaTeX)
 └── README.md
