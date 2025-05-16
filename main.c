#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_COLIS 100
#define MAX_VEHICULES 10
#define MAX_TOURNEE 50
#define MAX_VILLES 14
#define FLT_MAX 3.40282347E+38F // Valeur maximale pour un float
#define INF 1e9                 // Représente l'infini

#include "cJSON/cJSON.h" // Inclure la bibliothèque cJSON

// Définition des structures
// Structure pour les attributs d’une arete
typedef struct EdgeAttr
{
    float distance;    // en kilometres
    float baseTime;    // en minutes (temps nominal)
    float cost;        // cout monetaire
    int roadType;      // type de route (0: asphalte, 1: laterite , etc.)
    float reliability; // indice de fiabilite [0,1]
    int restrictions;  // restrictions codees en bits
    int toll;          // nombre de peages
} EdgeAttr;

// Structure pour un noeud de la liste d’adjacence
typedef struct AdjListNode
{
    int dest;                 // identifiant du noeud destination
    EdgeAttr attr;            // attributs de l’arete
    struct AdjListNode *next; // pointeur vers le prochain noeud
} AdjListNode;

// Structure pour la liste d’adjacence
typedef struct AdjList
{
    AdjListNode *head; // tete de la liste
} AdjList;

typedef struct Graph
{
    int V;
    AdjList *array;
    char **cityNames; // Tableau des noms des villes
} Graph;

// ---------- STRUCTURES DE DONNEES GLOUTONNE  ----------
typedef struct Colis
{
    int id;
    int villeDest;
    float poids;
    float volume;
    int urgent; // 1 si urgent, 0 sinon
} Colis;

typedef struct Vehicule
{
    int id;
    float capaciteMax;
    float capaciteRestante;
    int villeActuelle;
    int tournee[MAX_TOURNEE];
    int nbLivraisons;
} Vehicule;

typedef struct Carte
{
    float distances[MAX_VILLES][MAX_VILLES]; // Matrice de distances entre villes
} Carte;

// >>>>>>>>>> Graphe <<<<<<<<<<<
Graph *createGraph(int V);
void addEdge(Graph *graph, int src, int dest, EdgeAttr attr);
void printGraph(Graph *graph);
void freeGraph(Graph *graph);

// Function to read a file and return its content
char *readFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Erreur : impossible d'ouvrir le fichier %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *content = (char *)malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);
    return content;
}

// Function to load a graph from a JSON file
Graph *loadGraphFromJSON(const char *filename)
{
    char *jsonData = readFile(filename);
    if (!jsonData)
        return NULL;

    cJSON *json = cJSON_Parse(jsonData);
    if (!json)
    {
        printf("Erreur lors du parsing JSON.\n");
        free(jsonData);
        return NULL;
    }

    // Lire le nombre de sommets
    int V = cJSON_GetObjectItem(json, "vertices")->valueint;
    Graph *graph = createGraph(V); // No more implicit declaration error

    // Lire les noms des villes
    cJSON *nodes = cJSON_GetObjectItem(json, "nodes");
    if (nodes)
    {
        cJSON *node;
        cJSON_ArrayForEach(node, nodes)
        {
            int index = atoi(node->string); // Convertir la clé en entier
            if (index >= 0 && index < V)
            {
                graph->cityNames[index] = strdup(node->valuestring); // Copier le nom
            }
            else
            {
                printf("Erreur : index de ville invalide (%d).\n", index);
            }
        }
    }
    else
    {
        printf("Erreur : 'nodes' manquant dans le fichier JSON.\n");
    }

    // Lire la liste des arêtes
    cJSON *edges = cJSON_GetObjectItem(json, "edges");
    int edgeCount = cJSON_GetArraySize(edges);

    for (int i = 0; i < edgeCount; i++)
    {
        cJSON *edge = cJSON_GetArrayItem(edges, i);

        int src = cJSON_GetObjectItem(edge, "src")->valueint;
        int dest = cJSON_GetObjectItem(edge, "dest")->valueint;
        EdgeAttr attr;
        attr.distance = (float)cJSON_GetObjectItem(edge, "distance")->valuedouble;
        attr.baseTime = (float)cJSON_GetObjectItem(edge, "baseTime")->valuedouble;
        attr.cost = (float)cJSON_GetObjectItem(edge, "cost")->valuedouble;
        attr.roadType = cJSON_GetObjectItem(edge, "roadType")->valueint;
        attr.reliability = (float)cJSON_GetObjectItem(edge, "reliability")->valuedouble;
        attr.restrictions = cJSON_GetObjectItem(edge, "restrictions")->valueint;

        addEdge(graph, src, dest, attr); // No more implicit declaration error
    }

    cJSON_Delete(json);
    free(jsonData);
    return graph;
}

// >>>>>>>>>> DFS <<<<<<<<<<<
void dfsUtil(Graph *graph, int v, bool *visited);
void dfs(Graph *graph, int startVertex);

// >>>>>>>>>> BFS <<<<<<<<<<<
void bfs(Graph *graph, int startVertex);

// >>>>>>>>>> Floyd-Warshall <<<<<<<<<<<
void floydWarshall(Graph *graph, float dist[][graph->V]);
void printFloydWarshall(Graph *graph, float dist[][graph->V]);

// >>>>>>>>>> Bellman-Ford <<<<<<<<<<<
void bellmanFord(Graph *graph, int src, float *dist, int *pred, float maxTime);
void printBellmanFord(Graph *graph, int src, float *dist, int *pred);

// >>>>>>>>>> GLOUTONNE <<<<<<<<<<<
void affecterColis(Vehicule *vehicules, int nbVehicules, Colis *colis, int nbColis, Carte *carte, Graph *graph);
void afficherTournees(Vehicule *vehicules, int nbVehicules, Graph *graph);

// ---------- EXEMPLE DE DONNEES ----------
void initialiserCarte(Carte *carte)
{
    for (int i = 0; i < MAX_VILLES; i++)
        for (int j = 0; j < MAX_VILLES; j++)
            carte->distances[i][j] = (i == j) ? 0 : (rand() % 50 + 1); // distances aléatoires
}

void chargerColis(Colis *colis, int *nbColis)
{
    *nbColis = 20;
    srand(time(NULL)); // Seed for random number generation
    for (int i = 0; i < *nbColis; i++)
    {
        colis[i].id = i;
        colis[i].villeDest = rand() % MAX_VILLES;
        colis[i].poids = (float)(rand() % 50 + 1);
        colis[i].volume = (float)(rand() % 20 + 1);
        colis[i].urgent = rand() % 2;
    }
}

void chargerVehicules(Vehicule *vehicules, int *nbVehicules)
{
    *nbVehicules = 10;
    srand(time(NULL)); // Seed for random number generation
    for (int i = 0; i < *nbVehicules; i++)
    {
        vehicules[i].id = i;
        vehicules[i].capaciteMax = (float)(rand() % 100 + 1);
        vehicules[i].capaciteRestante = vehicules[i].capaciteMax;
        vehicules[i].villeActuelle = rand() % MAX_VILLES;
        vehicules[i].nbLivraisons = 0;
    }
}

// Détection de cycles
bool detectCycleDFS(Graph *graph, int v, bool *visited, int parent)
{
    visited[v] = true;

    AdjListNode *node = graph->array[v].head;
    while (node)
    {
        int neighbor = node->dest;
        if (!visited[neighbor])
        {
            if (detectCycleDFS(graph, neighbor, visited, v))
                return true;
        }
        else if (neighbor != parent)
        {
            return true; // Cycle détecté
        }
        node = node->next;
    }
    return false;
}

bool detectCycles(Graph *graph)
{
    bool *visited = malloc(graph->V * sizeof(bool));
    if (!visited)
    {
        printf("Erreur : allocation mémoire échouée pour le tableau 'visited'.\n");
        return false;
    }

    for (int i = 0; i < graph->V; i++)
        visited[i] = false;

    for (int i = 0; i < graph->V; i++)
    {
        if (!visited[i] && detectCycleDFS(graph, i, visited, -1))
        {
            free(visited);
            return true;
        }
    }

    free(visited);
    return false;
}

// Composantes connexes (BFS)
void findConnectedComponents(Graph *graph)
{
    bool *visited = malloc(graph->V * sizeof(bool));
    if (!visited)
    {
        printf("Erreur : allocation mémoire échouée pour le tableau 'visited'.\n");
        return;
    }

    for (int i = 0; i < graph->V; i++)
        visited[i] = false;

    int *queue = malloc(graph->V * sizeof(int));
    if (!queue)
    {
        printf("Erreur : allocation mémoire échouée pour la file.\n");
        free(visited);
        return;
    }

    int componentCount = 0;
    for (int i = 0; i < graph->V; i++)
    {
        if (!visited[i])
        {
            int front = 0, rear = 0;
            queue[rear++] = i;
            visited[i] = true;
            componentCount++;

            printf("Composante %d: ", componentCount);
            while (front < rear)
            {
                int current = queue[front++];
                printf("%s ", graph->cityNames[current]);

                AdjListNode *node = graph->array[current].head;
                while (node)
                {
                    if (!visited[node->dest])
                    {
                        visited[node->dest] = true;
                        queue[rear++] = node->dest;
                    }
                    node = node->next;
                }
            }
            printf("\n");
        }
    }

    printf("Total: %d composantes\n", componentCount);
    free(queue);
    free(visited);
}

// Accessibilité entre deux nœuds
bool isAccessible(Graph *graph, int src, int dest)
{
    if (src < 0 || src >= graph->V || dest < 0 || dest >= graph->V)
    {
        printf("Erreur : sommets source ou destination invalides.\n");
        return false;
    }

    bool *visited = malloc(graph->V * sizeof(bool));
    if (!visited)
    {
        printf("Erreur : allocation mémoire échouée pour le tableau 'visited'.\n");
        return false;
    }

    for (int i = 0; i < graph->V; i++)
        visited[i] = false;

    int *queue = malloc(graph->V * sizeof(int));
    if (!queue)
    {
        printf("Erreur : allocation mémoire échouée pour la file.\n");
        free(visited);
        return false;
    }

    int front = 0, rear = 0;
    queue[rear++] = src;
    visited[src] = true;

    while (front < rear)
    {
        int current = queue[front++];
        if (current == dest)
        {
            free(queue);
            free(visited);
            return true;
        }

        AdjListNode *node = graph->array[current].head;
        while (node)
        {
            if (!visited[node->dest])
            {
                visited[node->dest] = true;
                queue[rear++] = node->dest;
            }
            node = node->next;
        }
    }

    free(queue);
    free(visited);
    return false;
}

// Statistiques de connectivité
void calculateConnectivityStats(Graph *graph)
{
    int totalEdges = 0;
    for (int i = 0; i < graph->V; i++)
    {
        AdjListNode *node = graph->array[i].head;
        while (node)
        {
            totalEdges++;
            node = node->next;
        }
    }

    if (graph->V > 1)
    {
        float density = (totalEdges * 100.0) / (graph->V * (graph->V - 1));
        printf("Densité: %.2f%%\n", density);
    }
    else
    {
        printf("Densité: Non applicable (graphe avec moins de 2 sommets).\n");
    }
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> FONCTION PRINCIPALE
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int main(int argc, char *argv[])
{

    Graph *graph = loadGraphFromJSON("graph.json");
    if (!graph)
    {
        printf("Erreur lors du chargement du graphe.\n");
        return 1;
    }

    // printGraph(graph);
    // freeGraph(graph);

    // Appel de DFS à partir du sommet 0 (par exemple, Abidjan)
    dfs(graph, 0);

    // Appel de BFS à partir du sommet 0 (par exemple, Abidjan)
    bfs(graph, 0);

    // Détecter les cycles dans le réseaus
    printf("\n=== Analyse du réseau ===\n");
    printf("Cycles détectés: %s\n", detectCycles(graph) ? "OUI" : "NON");
    printf("\nComposantes connexes:\n");
    findConnectedComponents(graph);
    printf("\nAccessibilité Abidjan -> San-Pédro: %s\n",
           isAccessible(graph, 0, 3) ? "OUI" : "NON");
    printf("\nStatistiques:\n");
    calculateConnectivityStats(graph);

    // >>>>>>>>> Floyd-Warshall <<<<<<<<<<<
    float distFW[graph->V][graph->V];
    floydWarshall(graph, distFW);
    printFloydWarshall(graph, distFW);

    // >>>>>>>>>> Bellman-Ford <<<<<<<<<<<
    int src = 0;         // Abidjan
    float maxTime = 300; // En minutes, par exemple

    float distBF[graph->V];
    int pred[graph->V];

    bellmanFord(graph, src, distBF, pred, maxTime);
    printBellmanFord(graph, src, distBF, pred);

    // >>>>>>>>>> GLOUTONNE <<<<<<<<<<<
    Colis colis[MAX_COLIS];
    Vehicule vehicules[MAX_VEHICULES];
    Carte carte;
    int nbColis, nbVehicules;

    initialiserCarte(&carte);
    chargerColis(colis, &nbColis);
    chargerVehicules(vehicules, &nbVehicules);

    affecterColis(vehicules, nbVehicules, colis, nbColis, &carte, graph);
    afficherTournees(vehicules, nbVehicules, graph);

    return 0;
}

// Fonction pour créer un graphe
Graph *createGraph(int V)
{
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    if (!graph)
    {
        printf("Erreur : allocation mémoire échouée pour le graphe.\n");
        return NULL;
    }

    graph->V = V;
    graph->array = (AdjList *)malloc(V * sizeof(AdjList));
    graph->cityNames = (char **)malloc(V * sizeof(char *));
    if (!graph->array || !graph->cityNames)
    {
        printf("Erreur : allocation mémoire échouée pour les structures internes.\n");
        free(graph->array);
        free(graph->cityNames);
        free(graph);
        return NULL;
    }

    for (int i = 0; i < V; i++)
    {
        graph->array[i].head = NULL;
        graph->cityNames[i] = NULL; // Initialiser à NULL
    }

    return graph;
}

// Fonction pour ajouter une arête au graphe
void addEdge(Graph *graph, int src, int dest, EdgeAttr attr)
{
    if (src < 0 || src >= graph->V || dest < 0 || dest >= graph->V)
    {
        printf("Erreur : arête invalide (%d -> %d).\n", src, dest);
        return;
    }

    AdjListNode *newNode = (AdjListNode *)malloc(sizeof(AdjListNode));
    if (!newNode)
    {
        printf("Erreur : allocation mémoire échouée pour un nœud d'adjacence.\n");
        return;
    }

    newNode->dest = dest;
    newNode->attr = attr;
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;
}

// Fonction pour afficher le graphe
void printGraph(Graph *graph)
{
    printf("--------------------------------------------------------\n");
    printf("|\t\t\tGRAPHE DES VILLES\t\t\t|\n");
    printf("--------------------------------------------------------\n\n");

    for (int i = 0; i < graph->V; i++)
    {
        printf("Ville de depart : %s\n", graph->cityNames[i] ? graph->cityNames[i] : "Inconnue");
        AdjListNode *pCrawl = graph->array[i].head;

        if (pCrawl == NULL)
        {
            printf("   Aucune destination disponible depuis cette ville.\n");
        }
        else
        {
            printf("   Destinations:\n");
            while (pCrawl)
            {
                printf("   -> %s (Distance: %.2f km, Cost: %.2f XOF)\n",
                       graph->cityNames[pCrawl->dest] ? graph->cityNames[pCrawl->dest] : "Inconnue",
                       pCrawl->attr.distance, pCrawl->attr.cost);
                pCrawl = pCrawl->next;
            }
        }
        printf("\n");
    }
    printf("--------------------------------------------------------\n");
}

// Fonction pour libérer la mémoire allouée au graphe
void freeGraph(Graph *graph)
{
    for (int i = 0; i < graph->V; i++)
    {
        AdjListNode *pCrawl = graph->array[i].head;
        while (pCrawl)
        {
            AdjListNode *temp = pCrawl;
            pCrawl = pCrawl->next;
            free(temp); // Libère chaque nœud
        }
        free(graph->cityNames[i]); // Libère chaque nom de ville
    }
    free(graph->cityNames);
    free(graph->array); // Libère le tableau de listes
    free(graph);        // Libère la structure du graphe
}

/**
 * - **Pile implicite (DFS)** :
 *   L'algorithme DFS utilise une pile implicite via l'appel récursif de la fonction `dfsUtil`.
 *   Ce choix est justifié par la simplicité et la lisibilité du code, car la pile d'exécution
 *   du programme gère automatiquement les appels récursifs. Cependant, cette approche peut
 *   entraîner un dépassement de pile (stack overflow) pour des graphes très grands ou très profonds.
 *
 * - **File explicite (BFS)** :
 *   L'algorithme BFS utilise une file explicite implémentée à l'aide d'un tableau dynamique
 *   (int *queue) et de deux indices (`front` et `rear`) pour gérer les opérations d'enfilement
 *   et de défilement. Ce choix est justifié par la nécessité de traiter les sommets dans l'ordre
 *   dans lequel ils sont découverts, ce qui est une caractéristique clé du BFS. L'utilisation
 *   d'un tableau dynamique permet une gestion simple et efficace de la file.
 *
 * @note Les structures auxiliaires (pile implicite et file explicite) sont adaptées aux besoins
 * spécifiques des algorithmes DFS et BFS respectivement, garantissant une implémentation efficace
 * et conforme aux principes de ces parcours.
 */

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> DFS ALGORITHM
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Fonction pour effectuer un parcours en profondeur (DFS) à partir d'un sommet donné
void dfsUtil(Graph *graph, int v, bool *visited)
{
    // Marquer le sommet actuel comme visité
    visited[v] = true;
    printf("%s ", graph->cityNames[v]);

    // Parcourir tous les voisins du sommet actuel
    AdjListNode *node = graph->array[v].head;
    while (node)
    {
        if (!visited[node->dest])
        {
            dfsUtil(graph, node->dest, visited);
        }
        node = node->next;
    }
}

// Fonction principale pour effectuer un DFS sur tout le graphe
void dfs(Graph *graph, int startVertex)
{
    // Tableau pour suivre les sommets visités
    bool *visited = (bool *)malloc(graph->V * sizeof(bool));
    for (int i = 0; i < graph->V; i++)
    {
        visited[i] = false;
    }

    printf("Parcours en profondeur (DFS) à partir de %s :\n", graph->cityNames[startVertex]);
    dfsUtil(graph, startVertex, visited);

    printf("\n");

    // Libérer la mémoire
    free(visited);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> BFS ALGORITHM
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Fonction pour effectuer un parcours en largeur (BFS) à partir d'un sommet donné
void bfs(Graph *graph, int startVertex)
{
    // Créer un tableau pour suivre les sommets visités
    bool *visited = (bool *)malloc(graph->V * sizeof(bool));
    for (int i = 0; i < graph->V; i++)
        visited[i] = false;

    // Créer une file pour le BFS
    int *queue = (int *)malloc(graph->V * sizeof(int));
    int front = 0, rear = 0;

    // Marquer le sommet de départ comme visité et l'ajouter à la file
    visited[startVertex] = true;
    queue[rear++] = startVertex;

    printf("Parcours en largeur (BFS) à partir de %s :\n", graph->cityNames[startVertex]);

    while (front < rear)
    {
        // Extraire un sommet de la file
        int currentVertex = queue[front++];
        printf("%s ", graph->cityNames[currentVertex]);

        // Parcourir tous les voisins du sommet actuel
        AdjListNode *node = graph->array[currentVertex].head;
        while (node)
        {
            if (!visited[node->dest])
            {
                visited[node->dest] = true;
                queue[rear++] = node->dest;
            }
            node = node->next;
        }
    }

    printf("\n");

    // Libérer la mémoire
    free(visited);
    free(queue);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> FLOYD-WARSHALL ALGORITHM
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Fonction pour trouver le chemin le plus court entre tous les paires de sommets
void floydWarshall(Graph *graph, float dist[][graph->V])
{
    int V = graph->V;

    // Initialisation
    for (int i = 0; i < V; i++)
    {
        for (int j = 0; j < V; j++)
        {
            dist[i][j] = (i == j) ? 0 : INF;
        }

        AdjListNode *pCrawl = graph->array[i].head;
        while (pCrawl)
        {
            dist[i][pCrawl->dest] = pCrawl->attr.distance; // ou baseTime / cost selon le critère
            pCrawl = pCrawl->next;
        }
    }

    // Algorithme de Floyd-Warshall
    for (int k = 0; k < V; k++)
    {
        for (int i = 0; i < V; i++)
        {
            for (int j = 0; j < V; j++)
            {
                if (dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
            }
        }
    }
}

// Fonction pour afficher les distances
void printFloydWarshall(Graph *graph, float dist[][graph->V])
{
    printf("\n===== Plus courts chemins entre toutes les paires de villes (en km) - FLOYD WARSHALL =====\n");

    for (int i = 0; i < graph->V; i++)
    {
        for (int j = 0; j < graph->V; j++)
        {
            printf("De %-15s à %-15s : ", graph->cityNames[i], graph->cityNames[j]);
            if (dist[i][j] == INF)
                printf("Aucun chemin\n");
            else
                printf("%.2f km\n", dist[i][j]);
        }
        printf("\n");
    }
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> BELLMAN-FORD ALGORITHM
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Fonction pour trouver le chemin le plus court à partir d'une source
void bellmanFord(Graph *graph, int src, float *dist, int *pred, float maxTime)
{
    int V = graph->V;
    float time[V]; // Array to track cumulative time

    // Initialisation
    for (int i = 0; i < V; i++)
    {
        dist[i] = INF;
        time[i] = INF;
        pred[i] = -1;
    }
    dist[src] = 0;
    time[src] = 0;

    // Relaxation des arêtes V-1 fois
    for (int i = 1; i <= V - 1; i++)
    {
        for (int u = 0; u < V; u++)
        {
            AdjListNode *node = graph->array[u].head;
            while (node)
            {
                int v = node->dest;

                // float weight = node->attr.cost + 10 * node->attr.toll;
                float weight = node->attr.cost;
                float edgeTime = node->attr.baseTime;

                // Appliquer une contrainte sur le temps
                if (time[u] + edgeTime <= maxTime && dist[u] + weight < dist[v])
                {
                    dist[v] = dist[u] + weight;
                    time[v] = time[u] + edgeTime; // Update cumulative time
                    pred[v] = u;
                }

                node = node->next;
            }
        }
    }

    // Vérification des cycles négatifs (facultatif ici)
    for (int u = 0; u < V; u++)
    {
        AdjListNode *node = graph->array[u].head;
        while (node)
        {
            int v = node->dest;
            // float weight = node->attr.cost + 10 * node->attr.toll;
            float weight = node->attr.cost;
            float edgeTime = node->attr.baseTime;

            if (time[u] + edgeTime <= maxTime && dist[u] + weight < dist[v])
            {
                printf("Attention : présence d’un cycle de poids négatif.\n");
                return;
            }

            node = node->next;
        }
    }
}

// Fonction pour afficher le chemin le plus court
void printBellmanFord(Graph *graph, int src, float *dist, int *pred)
{
    printf("\n===== Chemins optimaux depuis %s (BELLMAN-FORD) =====\n", graph->cityNames[src]);

    for (int i = 0; i < graph->V; i++)
    {
        printf("Vers %-15s : ", graph->cityNames[i]);

        if (dist[i] == INF)
        {
            printf("Aucun chemin respectant les contraintes.\n");
        }
        else
        {
            float totalTime = 0;
            float totalCost = 0;
            int path[graph->V];
            int count = 0;

            for (int v = i; v != -1; v = pred[v])
                path[count++] = v;

            for (int j = count - 1; j > 0; j--)
            {
                AdjListNode *node = graph->array[path[j]].head;
                while (node && node->dest != path[j - 1])
                    node = node->next;

                if (node)
                {
                    totalTime += node->attr.baseTime;
                    totalCost += node->attr.cost; // instead of node->attr.cost + 10 * node->attr.toll
                }
            }

            printf("Temps = %.2f, Cout = %.2f XOF, Chemin = ", totalTime, totalCost);
            for (int j = count - 1; j >= 0; j--)
            {
                printf("%s", graph->cityNames[path[j]]);
                if (j > 0)
                    printf(" -> ");
            }
            printf("\n");
        }
    }
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>> GLOUTONNE ALGORITHM
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// ---------- AFFECTATION GLOUTONNE ----------
void affecterColis(Vehicule *vehicules, int nbVehicules, Colis *colis, int nbColis, Carte *carte, Graph *graph)
{
    printf("\n===== Affectation des colis aux véhicules (GLOUTONNE) =====\n");
    for (int i = 0; i < nbColis; i++)
    {
        float minDistance = FLT_MAX;
        int bestVehicule = -1;

        // Trouver le meilleur véhicule pour le colis
        for (int j = 0; j < nbVehicules; j++)
        {
            if (vehicules[j].capaciteRestante >= colis[i].poids)
            {
                float d = carte->distances[vehicules[j].villeActuelle][colis[i].villeDest];
                if (d < minDistance)
                {
                    minDistance = d;
                    bestVehicule = j;
                }
            }
        }

        // Affecter le colis au véhicule trouvé
        if (bestVehicule != -1)
        {
            vehicules[bestVehicule].capaciteRestante -= colis[i].poids;
            vehicules[bestVehicule].tournee[vehicules[bestVehicule].nbLivraisons++] = colis[i].villeDest;
            printf("Colis %d affecté au véhicule %d (destination : %s, distance : %.1f km)\n",
                   colis[i].id, bestVehicule, graph->cityNames[colis[i].villeDest], minDistance);
        }
        else
        {
            printf("Colis %d non assigné : aucun véhicule disponible avec la capacité suffisante\n", colis[i].id);
        }
    }
}

// ---------- AFFICHAGE TOURNEE ----------
void afficherTournees(Vehicule *vehicules, int nbVehicules, Graph *graph)
{
    printf("\n===== Tournées des véhicules (GLOUTONNE) =====\n");
    for (int i = 0; i < nbVehicules; i++)
    {
        printf("Tournée du véhicule %d : ", vehicules[i].id);
        for (int j = 0; j < vehicules[i].nbLivraisons; j++)
        {
            printf("-> %s ", graph->cityNames[vehicules[i].tournee[j]]);
        }
        printf("\n");
    }
}