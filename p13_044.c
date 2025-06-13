#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// Fungsi ini digunakan untuk mengalokasikan memori untuk matriks 2D
double** allocMatrix(int rows, int cols) {
    double **matrix = malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = malloc(cols * sizeof(double));
        memset(matrix[i], 0, cols * sizeof(double));
    }
    return matrix;
}

// Fungsi ini digunakan untuk membebaskan memori yang dialokasikan untuk matriks 2D
void freeMatrix(double **matrix, int rows) {
    for (int i = 0; i < rows; i++) free(matrix[i]);
    free(matrix);
}

// Fungsi untuk membaca matriks dari file CSV
void bacaMatrix(const char *filename, double ***matrix, int *rows, int *cols) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Gagal membuka file");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int r = 0, c = 0;

    // Hitung jumlah baris dan kolom
    while ((read = getline(&line, &len, file)) != -1) {
        int temp_cols = 0;
        char *token = strtok(line, ",");
        while (token) {
            temp_cols++;
            token = strtok(NULL, ",");
        }
        if (r == 0) {
            c = temp_cols; // Tetapkan jumlah kolom dari baris pertama
        } else if (temp_cols != c) {
            fprintf(stderr, "Jumlah kolom tidak konsisten di baris %d\n", r + 1);
            free(line);
            fclose(file);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        r++;
    }
    *rows = r;
    *cols = c;

    // Alokasi matriks
    *matrix = allocMatrix(*rows, *cols);

    // Kembali ke awal file untuk membaca data
    rewind(file);

    r = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        int col = 0;
        char *token = strtok(line, ",");
        while (token) {
            (*matrix)[r][col++] = atof(token);
            token = strtok(NULL, ",");
        }
        r++;
    }

    free(line);
    fclose(file);
}

// Fungsi untuk menyimpan hasil matriks ke file CSV
void simpanHasil(const char *filename, double **matrix, int rows, int cols) {
    FILE *file = fopen(filename, "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%.2f%c", matrix[i][j], (j == cols - 1) ? '\n' : ',');
        }
    }
    fclose(file);
}

// Fungsi untuk membuat grid 2D berdasarkan jumlah proses
void createGrid(int size, int *grid_rows, int *grid_cols) {
    *grid_rows = (int)sqrt(size);
    while (size % (*grid_rows) != 0) (*grid_rows)--;
    *grid_cols = size / (*grid_rows);
}

// Fungsi untuk menghitung ukuran blok
void calculateBlockSize(int matrix_size, int grid_size, int rank_in_dim, int *block_size, int *start_idx) {
    int base_size = matrix_size / grid_size;
    int remainder = matrix_size % grid_size;
    
    if (rank_in_dim < remainder) {
        *block_size = base_size + 1;
        *start_idx = rank_in_dim * (*block_size);
    } else {
        *block_size = base_size;
        *start_idx = remainder * (base_size + 1) + (rank_in_dim - remainder) * base_size;
    }
}

// Fungsi untuk melakukan perkalian matriks lokal
void localMatrixMultiply(double **A, double **B, double **C, int rows_A, int cols_A, int cols_B) {
    for (int i = 0; i < rows_A; i++) {
        for (int j = 0; j < cols_B; j++) {
            for (int k = 0; k < cols_A; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main(int argc, char *argv[]) {

    // Inisialisasi MPI
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char hostname[256];
    gethostname(hostname, 256);
    printf("[%s] Proses MPI [%d] berjalan pada processor: %s\n", hostname, rank, hostname);

    double **A_full = NULL, **B_full = NULL, **C_full = NULL;
    int rowsA, colsA, rowsB, colsB;
    int taskCount = 0;

    // Baca matriks di proses 0
    if (rank == 0) {
        // Periksa argumen input
        if (argc != 3) {
            fprintf(stderr, "Penggunaan: %s matrixA.csv matrixB.csv\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        bacaMatrix(argv[1], &A_full, &rowsA, &colsA);
        bacaMatrix(argv[2], &B_full, &rowsB, &colsB);
        //  Periksa dimensi matriks
        if (colsA != rowsB) {
            fprintf(stderr, "Dimensi matriks tidak cocok untuk perkalian!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    // Broadcast dimensi matriks ke semua proses
    MPI_Bcast(&rowsA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&colsA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&rowsB, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&colsB, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Setup grid 2D
    int grid_rows, grid_cols;
    createGrid(size, &grid_rows, &grid_cols);

    int my_row = rank / grid_cols;
    int my_col = rank % grid_cols;

    // Buat komunikasi baris dan kolom
    MPI_Comm row_comm, col_comm;
    MPI_Comm_split(MPI_COMM_WORLD, my_row, my_col, &row_comm);
    MPI_Comm_split(MPI_COMM_WORLD, my_col, my_row, &col_comm);

    // Hitung ukuran blok lokal
    int local_rows_A, local_cols_A, local_rows_B, local_cols_B;
    int start_row_A, start_col_A, start_row_B, start_col_B;
    
    // Hitung ukuran blok untuk matriks A dan B
    calculateBlockSize(rowsA, grid_rows, my_row, &local_rows_A, &start_row_A);
    calculateBlockSize(colsA, grid_cols, my_col, &local_cols_A, &start_col_A);
    calculateBlockSize(rowsB, grid_rows, my_row, &local_rows_B, &start_row_B);
    calculateBlockSize(colsB, grid_cols, my_col, &local_cols_B, &start_col_B);

    // Alokasi blok lokal
    double **A_local = allocMatrix(local_rows_A, local_cols_A);
    double **B_local = allocMatrix(local_rows_B, local_cols_B);
    double **C_local = allocMatrix(local_rows_A, local_cols_B);

    // Distribusi matriks A dan B dari proses 0
    if (rank == 0) {
        // Kirim blok A
        for (int p = 0; p < size; p++) {
            // Hitung baris dan kolom proses p
            int p_row = p / grid_cols;
            int p_col = p % grid_cols;
            int p_local_rows_A, p_local_cols_A, p_start_row_A, p_start_col_A;
            
            // Hitung ukuran blok untuk proses p
            calculateBlockSize(rowsA, grid_rows, p_row, &p_local_rows_A, &p_start_row_A);
            calculateBlockSize(colsA, grid_cols, p_col, &p_local_cols_A, &p_start_col_A);
            
            // Jika proses p adalah proses 0, copy blok A ke lokal
            if (p == 0) {
                for (int i = 0; i < local_rows_A; i++) {
                    for (int j = 0; j < local_cols_A; j++) {
                        A_local[i][j] = A_full[start_row_A + i][start_col_A + j];
                    }
                }
            // Jika bukan proses 0, kirim blok A ke proses p
            } else {
                for (int i = 0; i < p_local_rows_A; i++) {
                    MPI_Send(A_full[p_start_row_A + i] + p_start_col_A, p_local_cols_A, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                }
            }
        }
        
        // Kirim blok B
        for (int p = 0; p < size; p++) {
            // Hitung baris dan kolom proses p
            int p_row = p / grid_cols;
            int p_col = p % grid_cols;
            int p_local_rows_B, p_local_cols_B, p_start_row_B, p_start_col_B;
            
            // Hitung ukuran blok untuk proses p
            calculateBlockSize(rowsB, grid_rows, p_row, &p_local_rows_B, &p_start_row_B);
            calculateBlockSize(colsB, grid_cols, p_col, &p_local_cols_B, &p_start_col_B);
            
            // Jika proses p adalah proses 0, copy blok B ke lokal
            if (p == 0) {
                for (int i = 0; i < local_rows_B; i++) {
                    for (int j = 0; j < local_cols_B; j++) {
                        B_local[i][j] = B_full[start_row_B + i][start_col_B + j];
                    }
                }
            // Jika bukan proses 0, kirim blok B ke proses p
            } else {
                for (int i = 0; i < p_local_rows_B; i++) {
                    MPI_Send(B_full[p_start_row_B + i] + p_start_col_B, p_local_cols_B, MPI_DOUBLE, p, 1, MPI_COMM_WORLD);
                }
            }
        }
    // Jika bukan proses 0, terima blok A dan B
    } else {
        // Terima blok A
        for (int i = 0; i < local_rows_A; i++) {
            MPI_Recv(A_local[i], local_cols_A, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // Terima blok B
        for (int i = 0; i < local_rows_B; i++) {
            MPI_Recv(B_local[i], local_cols_B, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    // MPI_Barrier untuk sinkronisasi sebelum memulai perkalian
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
        printf("\n[%s] === MEMULAI PERKALIAN MATRIKS ===\n", hostname);

    double start = MPI_Wtime();

    // SUMMA Algorithm
    for (int k = 0; k < grid_cols; k++) {
        // Alokasi blok A dan B untuk proses k
        double **A_kj = allocMatrix(local_rows_A, local_cols_A);
        double **B_ik = allocMatrix(local_rows_B, local_cols_B);
        
        // Broadcast blok A sepanjang baris
        if (my_col == k) {
            for (int i = 0; i < local_rows_A; i++) {
                for (int j = 0; j < local_cols_A; j++) {
                    A_kj[i][j] = A_local[i][j];
                }
            }
        }
        // Broadcast blok A ke semua proses di baris
        for (int i = 0; i < local_rows_A; i++) {
            MPI_Bcast(A_kj[i], local_cols_A, MPI_DOUBLE, k, row_comm);
        }
        
        // Broadcast blok B sepanjang kolom
        if (my_row == k) {
            for (int i = 0; i < local_rows_B; i++) {
                for (int j = 0; j < local_cols_B; j++) {
                    B_ik[i][j] = B_local[i][j];
                }
            }
        }
        // Broadcast blok B ke semua proses di kolom
        for (int i = 0; i < local_rows_B; i++) {
            MPI_Bcast(B_ik[i], local_cols_B, MPI_DOUBLE, k, col_comm);
        }
        
        // Perkalian matriks lokal
        localMatrixMultiply(A_kj, B_ik, C_local, local_rows_A, local_cols_A, local_cols_B);
        taskCount += local_rows_A * local_cols_A * local_cols_B;
        
        // Bersihkan blok A dan B
        freeMatrix(A_kj, local_rows_A);
        freeMatrix(B_ik, local_rows_B);
    }

    // Akhir dari SUMMA Algorithm
    // Sinkronisasi sebelum mencetak hasil
    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();
    double exec_time = end - start;

    printf("[%s] Proses MPI [%d] selesai. Operasi perkalian: %d, Waktu eksekusi: %.6f detik\n",
           hostname, rank, taskCount, exec_time);

    // Kumpulkan hasil ke proses 0
    if (rank == 0) {
        // Alokasi matriks hasil
        C_full = allocMatrix(rowsA, colsB);
        
        // Copy hasil lokal proses 0
        for (int i = 0; i < local_rows_A; i++) {
            for (int j = 0; j < local_cols_B; j++) {
                C_full[start_row_A + i][start_col_B + j] = C_local[i][j];
            }
        }
        
        // Terima hasil dari proses lain
        for (int p = 1; p < size; p++) {
            // Hitung baris dan kolom proses p
            int p_row = p / grid_cols;
            int p_col = p % grid_cols;
            int p_local_rows_A, p_local_cols_B, p_start_row_A, p_start_col_B;
            
            // Hitung ukuran blok untuk proses p
            calculateBlockSize(rowsA, grid_rows, p_row, &p_local_rows_A, &p_start_row_A);
            calculateBlockSize(colsB, grid_cols, p_col, &p_local_cols_B, &p_start_col_B);
            
            // Terima hasil dari proses p
            for (int i = 0; i < p_local_rows_A; i++) {
                MPI_Recv(C_full[p_start_row_A + i] + p_start_col_B, p_local_cols_B, MPI_DOUBLE, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        
        printf("\n[%s] === HASIL AKHIR MATRIKS PERKALIAN ===\n", hostname);
        simpanHasil("hasil_perkalian.csv", C_full, rowsA, colsB);
        printf("[%s] Hasil matriks disimpan ke file hasil_perkalian.csv\n", hostname);
    // Jika bukan proses 0, kirim hasil lokal ke proses 0
    } else {
        for (int i = 0; i < local_rows_A; i++) {
            MPI_Send(C_local[i], local_cols_B, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
        }
    }

    // Statistik global
    if (rank == 0) {
        int totalTask = taskCount;
        double maxTime = exec_time;

        for (int p = 1; p < size; p++) {
            int recvTasks;
            double recvTime;
            MPI_Recv(&recvTasks, 1, MPI_INT, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&recvTime, 1, MPI_DOUBLE, p, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            totalTask += recvTasks;
            if (recvTime > maxTime) maxTime = recvTime;
        }

        printf("\n[%s] === STATISTIK GLOBAL ===\n", hostname);
        printf("[%s] Total proses yang digunakan: %d\n", hostname, size);
        printf("[%s] Total operasi perkalian seluruh proses: %d\n", hostname, totalTask);
        printf("[%s] Waktu eksekusi maksimum antar proses: %.6f detik\n", hostname, maxTime);
    } else {
        MPI_Send(&taskCount, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
        MPI_Send(&exec_time, 1, MPI_DOUBLE, 0, 4, MPI_COMM_WORLD);
    }

    // Bersihkan memori
    freeMatrix(A_local, local_rows_A);
    freeMatrix(B_local, local_rows_B);
    freeMatrix(C_local, local_rows_A);
    
    if (rank == 0) {
        freeMatrix(A_full, rowsA);
        freeMatrix(B_full, rowsB);
        freeMatrix(C_full, rowsA);
    }

    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&col_comm);
    MPI_Finalize();
    return 0;
}