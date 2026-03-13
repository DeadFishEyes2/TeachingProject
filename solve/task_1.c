#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct Node {
    double pret;
    double randament; // first node has no meaningful value here
    struct Node *next;
} Node;

// Create a new node
Node *create_node(double pret, double randament) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->pret = pret;
    node->randament = randament;
    node->next = NULL;
    return node;
}

// Append to end of list
void append(Node **head, double pret, double randament) {
    Node *new_node = create_node(pret, randament);
    if (*head == NULL) {
        *head = new_node;
        return;
    }
    Node *curr = *head;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = new_node;
}

// Free the list
void free_list(Node *head) {
    while (head) {
        Node *tmp = head;
        head = head->next;
        free(tmp);
    }
}

// Truncate to 3 decimal places (no rounding, just truncation)
double truncate3(double x) {
    // Handle negative numbers correctly
    if (x < 0)
        return -floor(-x * 1000.0) / 1000.0;
    return floor(x * 1000.0) / 1000.0;
}

int main() {
    int N;
    scanf("%d", &N);

    double *prices = (double *)malloc(N * sizeof(double));
    for (int i = 0; i < N; i++)
        scanf("%lf", &prices[i]);

    // Build linked list
    // First node has no randament
    Node *head = NULL;
    append(&head, prices[0], 0.0);

    for (int i = 1; i < N; i++) {
        double r = (prices[i] - prices[i - 1]) / prices[i - 1];
        append(&head, prices[i], r);
    }

    free(prices);

    // Calculate mean return (mu) - skip first node
    double sum = 0.0;
    int count = 0;
    Node *curr = head->next; // skip first node
    while (curr) {
        sum += curr->randament;
        count++;
        curr = curr->next;
    }
    double mu = (count > 0) ? sum / count : 0.0;

    // Calculate standard deviation (sigma)
    double sum_sq = 0.0;
    curr = head->next;
    while (curr) {
        double diff = curr->randament - mu;
        sum_sq += diff * diff;
        curr = curr->next;
    }
    double sigma = (count > 0) ? sqrt(sum_sq / count) : 0.0;

    // Sharpe ratio (Rf = 0)
    double sharpe = (sigma != 0.0) ? (mu / sigma) : 0.0;

    // Output with truncation to 3 decimals
    printf("%.3f\n", truncate3(mu));
    printf("%.3f\n", truncate3(sigma));
    printf("%.3f\n", truncate3(sharpe));

    free_list(head);
    return 0;
}