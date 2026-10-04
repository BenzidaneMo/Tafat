# Guide de l'enseignant – Tafat

Ce guide explique l'utilisation de **Tafat Master** pendant un cours. Pour
l'installation de la salle, voir [install.html](install.html) ou
[DEPLOYMENT.md](DEPLOYMENT.md).

> Brouillon : les noms des boutons suivent les traductions françaises
> actuelles et peuvent encore changer. Merci de signaler les erreurs.

## 1. Démarrer

1. Ouvrez **Tafat Master** sur l'ordinateur de l'enseignant.
2. Choisissez la salle avec **Emplacements & ordinateurs** (en bas à gauche).
   Chaque vignette montre l'écran d'un élève. Une vignette grise signifie que
   l'ordinateur est éteint ou que Tafat n'y fonctionne pas.
3. Sélectionnez des ordinateurs d'un clic (Ctrl + clic pour en ajouter). Sans
   sélection, une action s'applique à **tous** les ordinateurs affichés.

Un nouvel ordinateur d'élève ? Installez-le avec l'**Installeur élève** (sur
une clé USB, double-clic sur l'ordinateur de l'élève), puis **Ajouter des
ordinateurs** → **Rechercher** → cochez-le → **Ajouter à la salle**.
**Paramètres** ouvre le configurateur (langue, etc.) ; *OK* enregistre et ferme.

Plus de place pour les vignettes : le bouton tout en bas à droite (*Masquer la
barre d'outils*) cache la barre des boutons, et le même bouton la réaffiche.
Barre cachée, un clic droit dans la zone des ordinateurs donne toutes les
fonctions. Clic droit sur la barre → *Afficher seulement les icônes* la rend
plus petite.

## 2. Début du cours : l'appel

- **Appel** : chaque élève saisit son nom et sa classe. Les noms s'affichent
  ensuite sur les vignettes, même si tous les élèves utilisent le même compte
  Windows.
- Dans la fenêtre de l'appel, *Importer la liste de la classe* (fichier CSV du
  tableur de l'établissement) affiche aussi les élèves **absents** en rouge.
- *Exporter l'appel* l'enregistre en CSV (s'ouvre dans Excel ou LibreOffice).

## 3. Voir et aider

| Bouton | Usage |
|---|---|
| **Surveiller** | vue normale des écrans |
| **Vue à distance** | agrandir l'écran d'un élève |
| **Contrôle à distance** | prendre la main pour aider |
| **Focalisation** | afficher seulement les ordinateurs sélectionnés |
| **Capture d'écran** | enregistrer l'écran d'un élève |

## 4. Montrer et attirer l'attention

- **Démo** : montrer l'écran de l'enseignant en plein écran ou dans une fenêtre.
- **Verrouiller** : écrans noirs, clavier et souris bloqués ; cliquez de
  nouveau pour déverrouiller.
- **Message** : afficher un court texte sur les écrans.
- **Ouvrir une page internet** / **Démarrer l'application** : ouvrir la même
  page ou le même programme chez tous les élèves.

## 5. Limiter

- **Bloquer les applis** : bloque les programmes de la liste ou autorise
  *seulement* ceux de la liste (par exemple `winword`, `excel`). Les boutons
  *Ajouter les navigateurs web* et *Ajouter les logiciels de bureautique*
  remplissent la liste. Options sous Windows :
  bloquer aussi les **clés USB** et l'**impression**.
- **Applications ouvertes** : voir quelles applications sont ouvertes sur
  chaque ordinateur, depuis quand, et lesquelles ont été utilisées puis
  fermées ; fermer une application sur un ou sur tous les ordinateurs.
- **Bloquer les sites** : liste de sites interdits, ou seulement les sites
  autorisés, pour Chrome, Edge, Brave, Chromium et Firefox (Firefox doit être
  redémarré). L'option *Bloquer aussi Internet pour tous les autres programmes*
  (Windows) coupe Internet sans couper le réseau de la salle.

Cliquez de nouveau sur le bouton pour lever le blocage.

## 6. Évaluer : le quiz

1. **Quiz** → *Nouveau quiz* (ou *Nouveau sondage*), un titre, une *Durée
   limite* si besoin, puis *Ajouter une question* : *Choix unique*, *Choix
   multiple*, *Réponse écrite* ou *Vrai ou faux*. Une réponse par ligne ;
   mettez `*` devant les bonnes réponses. Sans `*`, la question compte comme
   **sondage** (pas de note). *Points* donne plus de poids à une question.
2. Choisissez le quiz dans la liste et cliquez sur *Lancer*. Avec une durée
   limite, les élèves voient un compte à rebours et leurs réponses sont
   envoyées automatiquement à la fin.
3. Les résultats arrivent en direct (barres par réponse, note par élève).
   *Exporter (CSV)* les enregistre.

Les bonnes réponses ne sont jamais envoyées aux ordinateurs des élèves.

## 7. Communiquer : mains levées et chat

- **Mains levées et chat** → *Afficher la barre de l'élève* : chaque élève
  reçoit une petite barre avec **Lever la main**, le chat et *Remettre mon
  travail*. La flèche au bout de la barre la réduit à la seule main.
- Une main levée apparaît sur la vignette et dans la fenêtre du chat
  (*Ouvrir la fenêtre de chat*). Répondez à un élève ou *Envoyer à tous* ;
  *Baisser la main* quand c'est réglé.
- **Récompenses** → *Donner une étoile* : chaque élève sélectionné reçoit une
  étoile et voit « Bravo ! » avec son nombre d'étoiles. *Retirer une étoile*
  en reprend une. *Afficher les étoiles* montre les étoiles de toute la classe
  (par nom du registre), avec *Recommencer* et *Exporter (CSV)*.
  Chaque classe a ses propres étoiles : choisissez la classe en haut de la
  fenêtre (*Nouvelle classe* pour en ajouter une), sinon l'élève suivant sur le
  même ordinateur verrait les étoiles du précédent. La fenêtre s'ouvre à la
  première étoile de chaque séance pour vous le rappeler.

## 8. Fichiers

- **Distribuer** : envoyer des fichiers aux élèves.
- **Collecter** : récupérer les fichiers des élèves ; chaque élève a son dossier
  (nom de l'appel + ordinateur).
- Les élèves peuvent aussi **remettre** eux-mêmes leur travail depuis leur barre ;
  *Ouvrir les travaux remis* dans la fenêtre du chat.
- **Rendre les travaux** : choisissez le dossier des fichiers collectés (par
  exemple après correction) ; chaque élève reçoit les fichiers de son dossier.

## 9. Fin du cours

- **Déconnexion** des sessions, **Éteindre**, **Redémarrer** ou **Allumer**
  (réveil par le réseau, s'il est activé dans le BIOS).
- **Inventaire** : version de Windows, processeur, mémoire, disque et version
  de Tafat de chaque ordinateur, exportable en CSV.

## 10. En cas de problème

- **Vignette grise** : l'ordinateur est éteint, n'est pas dans le réseau ou le
  service Tafat ne fonctionne pas. Redémarrez l'ordinateur de l'élève.
- **Accès refusé** : l'ordinateur de l'élève n'a pas la clé de cet ordinateur
  enseignant. Recréez l'**Installeur élève** et relancez-le sur cet ordinateur.
- **Un site n'est pas bloqué** : le navigateur n'est pas pris en charge ou
  Firefox n'a pas été redémarré ; bloquez ce navigateur avec **Bloquer les
  applis**.

---

Tafat est développé par [BenzidaneMo](https://github.com/BenzidaneMo), sur la base de [Veyon](https://veyon.io).
