# Manuel d'Utilisation du Programme

Ce programme est conçu pour simuler et optimiser les trajets logistiques entre différentes villes en Côte d'Ivoire. Il utilise un modèle de graphe pour représenter les routes et leurs attributs.

---

## Contenu du projet

- `main.c` — Code source principal du programme
- `graph.json` — Fichier de configuration du graphe
- `cJSON/` — Dossier contenant la bibliothèque cJSON pour lire les fichiers JSON

---

## Installation

### 1. Prérequis

- Compilateur C (`gcc` recommandé) utilisé pour compiler le programme
- Bibliothèque [`cJSON`](https://github.com/DaveGamble/cJSON) utilisée pour parser les fichiers JSON

### 2. Compilation

```bash
cd "/c/emplacement fichier" && gcc main.c cJSON/cJSON.c -o main && "/c/emplacement fichier"main
```

## Fonctionnalités

- **Affichage du graphe** : Le programme affiche les villes et leurs connexions avec les distances et coûts associés.
- **Calcul des plus courts chemins** : Utilise l'algorithme de Floyd-Warshall pour trouver les plus courts chemins entre toutes les paires de villes.
- **Optimisation logistique** : Intègre un modèle gloutonne pour affecter les colis aux véhicules en fonction des distances et des capacités.
