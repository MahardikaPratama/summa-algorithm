#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generateMatrixCSV(const char *filename, int rows, int cols) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Gagal membuka file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));  // Seed random

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val = rand() % 100; // nilai antara 0 sampai 99
            fprintf(fp, "%d", val);
            if (j < cols - 1) {
                fprintf(fp, ",");
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    printf("Matriks %dx%d dengan nilai int berhasil disimpan ke %s\n", rows, cols, filename);
}

int main() {
    int rows, cols;
    char filename[256];

    printf("Masukkan jumlah baris matriks: ");
    if (scanf("%d", &rows) != 1 || rows <= 0) {
        fprintf(stderr, "Input baris tidak valid!\n");
        return 1;
    }

    printf("Masukkan jumlah kolom matriks: ");
    if (scanf("%d", &cols) != 1 || cols <= 0) {
        fprintf(stderr, "Input kolom tidak valid!\n");
        return 1;
    }

    printf("Masukkan nama file output CSV: ");
    scanf("%s", filename);

    generateMatrixCSV(filename, rows, cols);

    return 0;
}
