#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // Inclure pour strdup
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
                       // Autres attributs pertinents
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

// Structure pour le graphe
// typedef struct Graph
// {
//     int V;          // nombre de sommets
//     AdjList *array; // tableau des listes d’adjacence
//                     // Informations supplementaires sur les sommets
// } Graph;
typedef struct Graph
{
    int V;
    AdjList *array;
    char **cityNames; // Tableau des noms des villes
} Graph;

// Fonction pour créer un graphe avec V sommets
// Graph *createGraph(int V)
// {
//     Graph *graph = (Graph *)malloc(sizeof(Graph));
//     graph->V = V;
//     graph->array = (AdjList *)malloc(V * sizeof(AdjList));

//     for (int i = 0; i < V; i++)
//         graph->array[i].head = NULL; // Initialisation des listes vides

//     return graph;
// }
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

// Fonction pour afficher le graphe
// void printGraph(Graph *graph)
// {
//     for (int i = 0; i < graph->V; i++)
//     {
//         AdjListNode *pCrawl = graph->array[i].head;
//         printf("Sommet %d:", i);
//         while (pCrawl)
//         {
//             printf(" -> %d (dist: %.2f km, cout: %.2f)",
//                    pCrawl->dest, pCrawl->attr.distance, pCrawl->attr.cost);
//             pCrawl = pCrawl->next;
//         }
//         printf("\n");
//     }
// }
// Modifier printGraph pour afficher les noms des villes
void printGraph(Graph *graph)
{
    for (int i = 0; i < graph->V; i++)
    {
        printf("Ville %s:", graph->cityNames[i] ? graph->cityNames[i] : "Inconnue");

        AdjListNode *pCrawl = graph->array[i].head;
        while (pCrawl)
        {
            printf(" -> %s (dist: %.2f km, coût: %.2f)",
                   graph->cityNames[pCrawl->dest] ? graph->cityNames[pCrawl->dest] : "Inconnue",
                   pCrawl->attr.distance, pCrawl->attr.cost);
            pCrawl = pCrawl->next;
        }
        printf("\n");
    }
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

// Programme principal
int main()
{

    /*
   int V = 5; // Nombre de sommets
  Graph *graph = createGraph(V);

  // Création d'arêtes avec des attributs
  EdgeAttr attr1 = {10.5, 15.0, 5.0, 0, 0.9, 0};
  EdgeAttr attr2 = {7.2, 10.0, 3.5, 1, 0.8, 1};

  addEdge(graph, 0, 1, attr1);
  addEdge(graph, 0, 2, attr2);
  addEdge(graph, 1, 3, attr1);
  addEdge(graph, 2, 4, attr2);
  addEdge(graph, 3, 4, attr1);

  // Affichage du graphe
  printGraph(graph);
  freeGraph(graph); // Libération de la mémoire
  */

    Graph *graph = loadGraphFromJSON("graph.json");
    if (!graph)
    {
        printf("Erreur lors du chargement du graphe.\n");
        return 1;
    }

    printf("Graphe chargé depuis JSON :\n");
    printGraph(graph);

    freeGraph(graph);

    return 0;
}
