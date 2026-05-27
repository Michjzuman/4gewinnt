#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main2() {
    int *numbers = NULL;
    int length = 0;
    int input;

    printf("%p\n", numbers);

    while (1) {
        printf("Zahl eingeben: ");
        scanf("%d", &input);

        // Six Seven hahaha
        if (input == 67) {
            break;
        }

        // Array vergrössern
        numbers = realloc(numbers, (length + 1) * sizeof(int));

        // Neue Zahl speichern
        numbers[length] = input;

        length++;
    }

    printf("\nGespeicherte Zahlen:\n");

    for (int i = 0; i < length; i++) {
        printf("%d\n", numbers[i]);
    }

    free(numbers);

    return 0;
}

typedef struct {
   int a;
   bool b;
   char c;
} A;

int main() {


    printf("%zu\n", sizeof(A));

}
