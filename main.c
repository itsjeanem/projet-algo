#include <stdio.h>
#include <stdlib.h>

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
typedef struct Graph
{
    int V;          // nombre de sommets
    AdjList *array; // tableau des listes d’adjacence
                    // Informations supplementaires sur les sommets
} Graph;

// Fonction pour créer un graphe avec V sommets
Graph *createGraph(int V)
{
    Graph *graph = (Graph *)malloc(sizeof(Graph));
    graph->V = V;
    graph->array = (AdjList *)malloc(V * sizeof(AdjList));

    for (int i = 0; i < V; i++)
        graph->array[i].head = NULL; // Initialisation des listes vides

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

// Fonction pour afficher le graphe
void printGraph(Graph *graph)
{
    for (int i = 0; i < graph->V; i++)
    {
        AdjListNode *pCrawl = graph->array[i].head;
        printf("Sommet %d:", i);
        while (pCrawl)
        {
            printf(" -> %d (dist: %.2f km, cout: %.2f)",
                   pCrawl->dest, pCrawl->attr.distance, pCrawl->attr.cost);
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
    }
    free(graph->array); // Libère le tableau de listes
    free(graph);        // Libère la structure du graphe
}

// Programme principal
int main()
{

    // ajouterArete(graph, 0, 1, 10.5, 15.0, 3.0, 1, 0.9, 0);
    // ajouterArete(graph, 0, 4, 20.0, 25.0, 5.0, 2, 0.8, 1);
    // ajouterArete(graph, 1, 2, 12.0, 18.0, 2.5, 1, 0.95, 0);
    // ajouterArete(graph, 1, 3, 8.0, 10.0, 1.5, 2, 0.85, 0);
    // ajouterArete(graph, 3, 4, 15.0, 20.0, 4.0, 3, 0.7, 1);

    int V = 5; // Nombre de sommets
    Graph *graph = createGraph(V);

    // Création d'arêtes avec des attributs
    EdgeAttr attr1 = {10.5, 15.0, 5.0, 0, 0.9, 0};
    EdgeAttr attr2 = {7.2, 10.0, 3.5, 1, 0.8, 1};
    // EdgeAttr attr3 = {12.0, 18.0, 2.5, 0, 0.95, 0};
    // EdgeAttr attr4 = {8.0, 10.0, 1.5, 1, 0.85, 0};
    // EdgeAttr attr5 = {15.0, 20.0, 4.0, 2, 0.7, 1};
    // EdgeAttr attr6 = {20.0, 25.0, 5.0, 1, 0.8, 1};
    // EdgeAttr attr7 = {5.0, 7.0, 1.0, 0, 0.95, 0};

    addEdge(graph, 0, 1, attr1);
    addEdge(graph, 0, 2, attr2);
    addEdge(graph, 1, 3, attr1);
    addEdge(graph, 2, 4, attr2);
    addEdge(graph, 3, 4, attr1);

    // Affichage du graphe
    printGraph(graph);
    freeGraph(graph); // Libération de la mémoire

    return 0;
}
