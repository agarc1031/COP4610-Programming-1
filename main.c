#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 10

int input_array[SIZE] = {7, 12, 19, 3, 18, 4, 2, 6, 15, 8};
int sorted_array[SIZE];

typedef struct {
    int start;
    int end;
} ThreadArgs;

void bubble_sort(int* arr, int start, int end) {
    for (int i = start; i < end - 1; i++) {
        for (int j = start; j < end - (i - start) - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void* sort_thread(void* args) {
    ThreadArgs* t = (ThreadArgs*)args;
    bubble_sort(input_array, t->start, t->end);
    pthread_exit(NULL);
}

void* merge_thread(void* args) {
    int mid = SIZE / 2;
    int i = 0, j = mid, k = 0;

    while (i < mid && j < SIZE) {
        if (input_array[i] < input_array[j]) {
            sorted_array[k++] = input_array[i++];
        } else {
            sorted_array[k++] = input_array[j++];
        }
    }
    while (i < mid) {
        sorted_array[k++] = input_array[i++];
    }
    while (j < SIZE) {
        sorted_array[k++] = input_array[j++];
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t sort_thread1, sort_thread2, mergeThread;
    ThreadArgs t1 = {0, SIZE / 2};
    ThreadArgs t2 = {SIZE / 2, SIZE};

    pthread_create(&sort_thread1, NULL, sort_thread, &t1);
    pthread_create(&sort_thread2, NULL, sort_thread, &t2);

    pthread_join(sort_thread1, NULL);
    pthread_join(sort_thread2, NULL);

    pthread_create(&mergeThread, NULL, merge_thread, NULL);
    pthread_join(mergeThread, NULL);

    printf("Sorted array: ");
    for (int i = 0; i < SIZE; i++) {
        printf("%d ", sorted_array[i]);
    }
    printf("\n");

    return 0;
}
