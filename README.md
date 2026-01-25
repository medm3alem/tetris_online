# 🎮 Tetris Online – C++ / Raylib / WSL

Projet de jeu **Tetris en C++**, jouable :
- en **mode solo** ;
- en **mode multijoueur en ligne**, basé sur une architecture **client–serveur**.

Le client utilise la bibliothèque **Raylib** pour l’interface graphique et l’audio,  
tandis que le serveur gère les connexions réseau via des **sockets TCP**.

---

## 1. Architecture générale

Le projet est organisé autour de **deux composants principaux** : un client graphique et un serveur réseau.

```text
tetris_online/
├── client/
│   ├── objets.h
│   ├── game.h
│   ├── network.h
│   ├── network.cpp
│   ├── main.cpp
│   ├── CMakeLists.txt
│   └── sounds/
│       ├── *.wav
│       └── *.mp3
│
├── server/
│   ├── server.cpp
│   └── server.h
│
├── docs/
│   ├── installation_guide.tex
│   ├── installation_guide.pdf
│   └── ensta.png
│
└── README.md
```


---

## 2. Dossier `client/`

Le dossier `client` contient l’ensemble du **jeu Tetris côté joueur**, incluant :
- la logique du jeu,
- l’affichage graphique,
- la gestion du son,
- la communication réseau avec le serveur.

### 2.1 `main.cpp`
Point d’entrée du client.

Responsabilités principales :
- initialisation de la fenêtre graphique (Raylib) ;
- gestion de la boucle principale du jeu ;
- gestion des entrées clavier et souris ;
- affichage de l’interface (score, niveau, pièces, chat) ;
- gestion des modes **solo / online** ;
- communication avec le serveur (envoi / réception des messages).

---

### 2.2 `objets.h`
Définit la classe `object`, représentant :
- la **grille de jeu** ;
- les **pièces de Tetris**.

Fonctionnalités principales :
- création des pièces (I, O, T, L, J, S, Z) ;
- déplacements (gauche, droite, bas, haut) ;
- rotation des pièces (matrice de rotation) ;
- détection des collisions ;
- suppression des lignes complètes ;
- dessin des cellules avec Raylib.

Ce fichier contient le **cœur mathématique et géométrique** du jeu.

---

### 2.3 `game.h`
Définit la classe `Game`, qui encapsule la **logique complète du jeu**.

Responsabilités :
- gestion du score et du niveau ;
- génération des pièces courantes et suivantes ;
- gestion des états du jeu (pause, game over, victoire) ;
- logique du mode multijoueur (attaque par lignes, fin de partie) ;
- gestion du chat rapide entre joueurs ;
- gestion des sons et de la musique.

La classe `Game` sert d’interface entre :
- la logique du jeu (`object`) ;
- le réseau (`network`) ;
- l’affichage (`main.cpp`).

---

### 2.4 `network.h` / `network.cpp`
Gère la **communication réseau côté client**.

Fonctionnalités :
- connexion au serveur TCP ;
- envoi de messages (`READY`, `LINES`, `CHAT`, `GAMEOVER`) ;
- réception asynchrone des messages dans un thread dédié ;
- mise en file des messages reçus ;
- déconnexion propre.

Le réseau est **non bloquant** pour garantir la fluidité du jeu.

---

### 2.5 `sounds/`
Contient les fichiers audio utilisés par le jeu :
- musique de fond ;
- effets sonores (rotation, destruction de lignes).

Formats utilisés : `.wav`, `.mp3`.

---

### 2.6 `CMakeLists.txt`
Fichier de configuration pour la compilation du client avec **CMake** :
- génération de l’exécutable ;
- lien avec la bibliothèque Raylib ;
- configuration compatible Linux / WSL.

---

## 3. Dossier `server/`

Le dossier `server` contient le **serveur multijoueur**.

### 3.1 `server.cpp`
Point d’entrée du serveur.

Responsabilités :
- création de la socket TCP ;
- acceptation des connexions clientes ;
- gestion de plusieurs clients via des threads ;
- synchronisation des joueurs (message `MATCH_START`) ;
- retransmission des messages entre clients ;
- gestion des déconnexions.

Le serveur agit comme **relai**, sans logique de jeu interne.

---

### 3.2 `server.h`
Déclarations et constantes utilisées par le serveur.

---

## 4. Fonctionnement du mode multijoueur

1. Les clients se connectent au serveur TCP (port 4242).
2. Chaque client envoie le message `READY`.
3. Lorsque deux joueurs sont prêts, le serveur envoie `MATCH_START`.
4. Les clients échangent :
   - lignes à ajouter (`LINES|n`) ;
   - messages de chat (`CHAT|...`) ;
   - fin de partie (`GAMEOVER`).
5. Le serveur diffuse les messages à l’adversaire concerné.

---

## 5. Technologies utilisées

- **Langage** : C++
- **Graphisme & audio** : Raylib
- **Réseau** : Sockets TCP (POSIX)
- **Compilation** : CMake
- **Environnement** : Linux / WSL

---

## 6. Auteur(s)

Projet réalisé dans le cadre de l’UE **IN04**.

- Mohamed Eladeb  
- Mahdi Abid
