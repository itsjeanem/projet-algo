#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define INF 1e9          // Représente l'infini
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

Graph *createGraph(int V)
{
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->V = V;
    graph->array = (AdjList *)malloc(V * sizeof(AdjList));
    graph->cityNames = (char **)malloc(V * sizeof(char *)); // Allouer un tableau de chaînes

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
    AdjListNode *newNode = (AdjListNode *)malloc(sizeof(AdjListNode));
    newNode->dest = dest;
    newNode->attr = attr;
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;
}

// Fonction pour lire un fichier et retourner son contenu
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

// Fonction pour charger un graphe depuis un fichier JSON
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
    Graph *graph = createGraph(V);

    // Lire les noms des villes
    cJSON *nodes = cJSON_GetObjectItem(json, "nodes");
    if (nodes)
    {
        cJSON *node;
        cJSON_ArrayForEach(node, nodes)
        {
            int index = atoi(node->string);                      // Convertir la clé en entier
            graph->cityNames[index] = strdup(node->valuestring); // Copier le nom
        }
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

        addEdge(graph, src, dest, attr);
    }

    cJSON_Delete(json);
    free(jsonData);
    return graph;
}

// Modifier printGraph pour afficher les noms des villes
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

// >>>>>>>>>> Floyd-Warshall <<<<<<<<<<<
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
void printAllPairsShortestPaths(Graph *graph, float dist[][graph->V])
{
    printf("\n===== Plus courts chemins entre toutes les paires de villes (en km) =====\n");

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

// >>>>>>>>>> Bellman-Ford <<<<<<<<<<<
// Fonction pour trouver le chemin le plus court à partir d'une source
void bellmanFord(Graph *graph, int src, float *dist, int *pred, float maxTime)
{
    int V = graph->V;

    // Initialisation
    for (int i = 0; i < V; i++)
    {
        dist[i] = INF;
        pred[i] = -1;
    }
    dist[src] = 0;

    // Relaxation des arêtes V-1 fois
    for (int i = 1; i <= V - 1; i++)
    {
        for (int u = 0; u < V; u++)
        {
            AdjListNode *node = graph->array[u].head;
            while (node)
            {
                int v = node->dest;

                // float weight = node->attr.distance; // ou node->attr.cost / baseTime
                float weight = node->attr.cost + 10 * node->attr.toll;
                float time = node->attr.baseTime;

                // Appliquer une contrainte sur le temps
                if (time <= maxTime && dist[u] + weight < dist[v])
                {
                    dist[v] = dist[u] + weight;
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
            // float weight = node->attr.distance;
            float weight = node->attr.cost + 10 * node->attr.toll;

            if (dist[u] + weight < dist[v])
            {
                printf("Attention : présence d’un cycle de poids négatif.\n");
                return;
            }

            node = node->next;
        }
    }
}

// Fonction pour afficher le chemin le plus court
void printBellmanResults(Graph *graph, int src, float *dist, int *pred)
{
    printf("\n===== Chemins optimaux depuis %s (avec contraintes) =====\n", graph->cityNames[src]);

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
                    totalCost += node->attr.cost + 10 * node->attr.toll;
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

// Programme principal
int main(int argc, char *argv[])
{

    Graph *graph = loadGraphFromJSON("graph.json");
    if (!graph)
    {
        printf("Erreur lors du chargement du graphe.\n");
        return 1;
    }

    printGraph(graph);

    // >>>>>>>>> Floyd-Warshall <<<<<<<<<<<
    // float dist[graph->V][graph->V];
    // floydWarshall(graph, dist);
    // printAllPairsShortestPaths(graph, dist);

    // >>>>>>>>>> Bellman-Ford <<<<<<<<<<<
    int src = 0;         // Abidjan
    float maxTime = 400; // En minutes, par exemple

    float dist[graph->V];
    int pred[graph->V];

    bellmanFord(graph, src, dist, pred, maxTime);
    printBellmanResults(graph, src, dist, pred);

    freeGraph(graph);

    return 0;
}
