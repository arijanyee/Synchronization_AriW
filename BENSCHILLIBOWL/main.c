
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "BENSCHILLIBOWL.h"

// Restaurant configuration
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS (NUM_CUSTOMERS * ORDERS_PER_CUSTOMER)

// Global restaurant variable
BENSCHILLIBOWL *bcb;

/**
 * Customer thread:
 * 1. Allocate an order.
 * 2. Pick a random menu item.
 * 3. Populate the order.
 * 4. Add the order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {

    int customer_id = (int)(long)tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {

        Order* order = malloc(sizeof(Order));

        if (order == NULL) {
            perror("Failed to allocate order");
            return NULL;
        }

        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->order_number = -1;
        order->next = NULL;

        AddOrder(bcb, order);
    }

    return NULL;
}

/**
 * Cook thread:
 * 1. Retrieve an order.
 * 2. Fulfill the order.
 * 3. Free the order.
 * 4. Continue until no more orders exist.
 */
void* BENSCHILLIBOWLCook(void* tid) {

    int cook_id = (int)(long)tid;
    int orders_fulfilled = 0;

    Order* order;

    while ((order = GetOrder(bcb)) != NULL) {

        /* Fulfill the order */
        if (order->menu_item != NULL) {

            orders_fulfilled++;

            /* Record the completed order */
            pthread_mutex_lock(&bcb->mutex);
            bcb->orders_handled++;
            pthread_mutex_unlock(&bcb->mutex);
        }

        /* Release the order's allocated memory */
        free(order);
    }

    printf("Cook #%d fulfilled %d orders\n",
           cook_id, orders_fulfilled);

    return NULL;
}

/**
 * Main function:
 * 1. Open the restaurant.
 * 2. Create customer and cook threads.
 * 3. Wait for all threads to finish.
 * 4. Close the restaurant.
 */
int main() {

    srand(time(NULL));

    /* Open restaurant */
    bcb = OpenRestaurant(
        BENSCHILLIBOWL_SIZE,
        EXPECTED_NUM_ORDERS
    );

    if (bcb == NULL) {
        fprintf(stderr, "Failed to open restaurant\n");
        return 1;
    }

    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    /* Create cook threads */
    for (long i = 0; i < NUM_COOKS; i++) {

        if (pthread_create(
                &cooks[i],
                NULL,
                BENSCHILLIBOWLCook,
                (void*)i) != 0) {

            perror("Failed to create cook thread");
            return 1;
        }
    }

    /* Create customer threads */
    for (long i = 0; i < NUM_CUSTOMERS; i++) {

        if (pthread_create(
                &customers[i],
                NULL,
                BENSCHILLIBOWLCustomer,
                (void*)i) != 0) {

            perror("Failed to create customer thread");
            return 1;
        }
    }

    /* Wait for all customers to finish */
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    /* Wait for all cooks to finish */
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cooks[i], NULL);
    }

    /* Close restaurant */
    CloseRestaurant(bcb);

    return 0;
}
