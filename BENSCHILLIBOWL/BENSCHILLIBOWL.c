
#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = {
    "BensChilli",
    "BensHalfSmoke",
    "BensHotDog",
    "BensChilliCheeseFries",
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};

int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int random_index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[random_index];
}

/* Allocate and initialize the restaurant */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {

    BENSCHILLIBOWL* bcb = malloc(sizeof(BENSCHILLIBOWL));

    if (bcb == NULL) {
        return NULL;
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 0;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    if (pthread_mutex_init(&bcb->mutex, NULL) != 0) {
        free(bcb);
        return NULL;
    }

    if (pthread_cond_init(&bcb->can_add_orders, NULL) != 0) {
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        return NULL;
    }

    if (pthread_cond_init(&bcb->can_get_orders, NULL) != 0) {
        pthread_cond_destroy(&bcb->can_add_orders);
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        return NULL;
    }

    printf("Restaurant is open!\n");

    return bcb;
}

/* Close the restaurant and release resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {

    pthread_mutex_lock(&bcb->mutex);

    assert(bcb->current_size == 0);
    assert(bcb->orders == NULL);
    assert(bcb->next_order_number == bcb->expected_num_orders);
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_unlock(&bcb->mutex);

    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);
    pthread_mutex_destroy(&bcb->mutex);

    free(bcb);

    printf("Restaurant is closed!\n");
}

/* Add an order to the back of the queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {

    pthread_mutex_lock(&bcb->mutex);

    /* Wait while the restaurant is full */
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    /* Assign an order number */
    order->order_number = bcb->next_order_number++;

    order->next = NULL;

    /* Add order to the back of the queue */
    AddOrderToBack(&bcb->orders, order);

    bcb->current_size++;

    int order_number = order->order_number;

    /* Notify cooks that an order is available */
    if (bcb->next_order_number == bcb->expected_num_orders) {
        pthread_cond_broadcast(&bcb->can_get_orders);
    } else {
        pthread_cond_signal(&bcb->can_get_orders);
    }

    pthread_mutex_unlock(&bcb->mutex);

    return order_number;
}

/* Remove an order from the queue */
Order* GetOrder(BENSCHILLIBOWL* bcb) {

    pthread_mutex_lock(&bcb->mutex);

    /*
     * Wait while the queue is empty and customers
     * may still submit more orders.
     */
    while (IsEmpty(bcb) &&
           bcb->next_order_number < bcb->expected_num_orders) {

        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    /* No more orders will arrive */
    if (IsEmpty(bcb)) {
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    /* Remove the first order */
    Order* order = bcb->orders;

    bcb->orders = order->next;
    order->next = NULL;

    bcb->current_size--;

    /* Notify customers that space is available */
    pthread_cond_signal(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

/* Check whether the restaurant queue is empty */
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

/* Check whether the restaurant queue is full */
bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* Add an order to the rear of the linked-list queue */
void AddOrderToBack(Order** orders, Order* order) {

    order->next = NULL;

    if (*orders == NULL) {
        *orders = order;
        return;
    }

    Order* current = *orders;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = order;
}
