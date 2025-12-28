## 1. Introduction

Ce fichier fait l'état de la cartogrpahie mémoire du TP Exam, avec les éventuels choix que nous avons pu faire.

## 2. Découpage global de l’espace physique

L'espace d'adressage physique du noyau se situe sur les 4 premiers Mo de la mémoire.
Sur ces 4 premiers Mo, on place la PGD destinée à l'espace noyau et celles destinées aux espaces utilisateur.

| Adresse physique             | Taille  | Usage            |
| ---------------------------- | ------- | ---------------- |
| `0x00000000` -> `0x003FFFFF` | 4 Mo    | Noyau            |
| `0x00100000`                 | 4 Ko    | PGD noyau        |
| `0x00180000`                 | 4 Ko    | PGD user 1       |
| `0x00200000`                 | 4 Ko    | PGD user 2       |
| `0x00800000`                 | 4 Ko    | Mémoire partagée |

## 3. Espace virtuel noyau

### 3.1 Identity-mapping

Le noyau est **identité-mappé** sur les 4 premiers Mo.

| Adresse virtuelle           | Adresse physique | Droits    | Usages                  |
| --------------------------- | ---------------- | --------- | ----------------------- |
| `0x00000000` → `0x003FFFFF` | Identique        | Kernel    | Code et piles noyau     |

### 3.2. Piles noyau

| Tâche | Pile noyau   |
| ----- | ------------ |
| user1 | `0x002F0000` |
| user2 | `0x002A0000` |


## 4. Espace virtuel utilisateur

Pour ce qui est de la mémoire partagée entre les deux processus utilisateurs,
nous avons fait attention de donner à l'utilisateur uniquement les droits en lecture,
car nul besoin de l'écriture sur cette adresse pour l'affichage.

Le cahier des charges imposait l'identity-mapping sur l'espace d'adressage des tâches utilisateur.

Nous avons aussi fait le choix de mettre les pages noyau en lecture seule pour éviter qu'une tâche utilisateur (ring 3) 
vienne écrire à cet endroit et donc potentiellement modifier le code noyau qui s'exécute en mode privilégié (ring 0).

### 4.1 Espace virtuel de `user1`

#### 4.1.1 Schéma

| Adresse virtuelle           | Adresse physique | Droits    | Usage            |
| --------------------------- | ---------------- | --------- | ---------------- |
| `0x00000000` → `0x003FFFFF` | Identique        | Kernel    | Accès noyau      |
| `0x00400000` → `0x007FFFFF` | Identique        | User RW   | Code utilisateur |
| `0x00F100000`               | `0x00800000`     | User RW   | Compteur partagé |
| `0x00500000`                | Identique        | User RW   | Pile utilisateur |

#### 4.1.2 Choix

- `user1` a le droit d’écriture sur le compteur

### 4.2 Espace virtuel de `user2`

### 4.1.1 Schéma

| Adresse virtuelle           | Adresse physique | Droits    | Usage            |
| --------------------------- | ---------------- | --------- | ---------------- |
| `0x00000000` → `0x003FFFFF` | Identique        | Kernel    | Accès noyau      |
| `0x00400000` → `0x007FFFFF` | Identique        | User RW   | Code utilisateur |
| `0x00F200000`               | `0x00800000`     | User RO   | Compteur partagé |
| `0x00600000`                | Identique        | User RW   | Pile utilisateur |

### 4.2.2 Choix

- Même structure que pour `user1`
- `user2` a le droit en **lecture seule** sur le compteur

