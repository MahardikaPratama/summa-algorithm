Berikut adalah `README.md` dalam format **Markdown** yang menarik dan siap Anda copy-paste ke GitHub:

```markdown
# 🚀 Parallel Matrix Multiplication with MPI & Docker

> Perkalian Matriks Secara Paralel dengan Algoritma SUMMA menggunakan MPI dan Docker

## 📌 Informasi Proyek

- 👨‍💻 **Nama**: Mahardika Pratama  
- 🆔 **NIM**: 221524044  
- 🏫 **Kelas**: 3B – D4 Teknik Informatika  
- 📘 **Topik**: Perkalian Matriks Multi-Node dengan MPI (MPICH)  
- ⚙️ **Teknologi**: MPI, MPICH, Docker, Docker Compose, SSH

---

## 💡 Deskripsi Singkat

Proyek ini menerapkan algoritma **SUMMA (Scalable Universal Matrix Multiplication Algorithm)** untuk melakukan perkalian matriks besar secara paralel. Distribusi kerja dilakukan melalui **MPI** dan diujicoba dalam lingkungan multi-node menggunakan **Docker**.

---

## 🧠 Tentang Algoritma SUMMA

- 📊 Dirancang untuk sistem paralel.
- 📦 Membagi matriks A dan B ke blok-blok kecil dan mendistribusikannya.
- 🔄 Proses berjalan dalam grid 2D, contoh: `2x2`.
- 📡 Komunikasi efisien menggunakan `broadcast` antar proses baris & kolom.

---

## 📁 Struktur File

```

├── p13\_044.c                 # Source code program MPI
├── Dockerfile                # Image untuk node (MPICH + dependensi)
├── docker-compose.yaml       # Setup multi-container (master & workers)
├── matriksA\_*.csv            # Input matriks A (ukuran 2x2 hingga 1000x1000)
├── matriksB\_*.csv            # Input matriks B
└── README.md                 # Dokumentasi proyek ini

````

---

## 🛠️ Langkah Menjalankan Proyek

### 1. Build dan Jalankan Docker
```bash
docker-compose up --build -d
````

### 2. Masuk ke Container Master

```bash
docker exec -it master bash
```

### 3. Generate SSH Key

```bash
ssh-keygen -t rsa -N "" -f /root/.ssh/id_rsa
```

### 4. Salin Key ke Worker Node

```bash
sshpass -p root ssh-copy-id -o StrictHostKeyChecking=no root@172.28.0.3
sshpass -p root ssh-copy-id -o StrictHostKeyChecking=no root@172.28.0.4
sshpass -p root ssh-copy-id -o StrictHostKeyChecking=no root@172.28.0.5
```

> **Note:** Install `sshpass` jika belum ada:

```bash
apt-get install -y sshpass
```

### 5. Buat `hostfile`

```bash
echo "172.28.0.2" > hostfile
echo "172.28.0.3" >> hostfile
echo "172.28.0.4" >> hostfile
echo "172.28.0.5" >> hostfile
```

---

## ▶️ Menjalankan Program MPI

### ✅ Matriks 2x2 (Mudah)

```bash
mpirun -f hostfile -np 4 /root/p13_044 /root/matriksA_2.csv /root/matriksB_2.csv
```

### ✅ Matriks 100x100 (Menengah)

```bash
mpirun -f hostfile -np 4 /root/p13_044 /root/matriksA_100.csv /root/matriksB_100.csv
```

### ✅ Matriks 500x500 (Menengah)

```bash
mpirun -f hostfile -np 4 /root/p13_044 /root/matriksA_500.csv /root/matriksB_500.csv
```

### ✅ Matriks 1000x1000 (Sulit)

```bash
mpirun -f hostfile -np 4 /root/p13_044 /root/matriksA_1000.csv /root/matriksB_1000.csv
```

---

## 📊 Hasil Pengujian

| Ukuran Matriks | Operasi/Proses | Total Operasi | Waktu (detik) | Status     |
| -------------- | -------------- | ------------- | ------------- | ---------- |
| 2x2            | 2              | 8             | 0.001179      | ✅ Berhasil |
| 100x100        | 250,000        | 1,000,000     | 0.181680      | ✅ Berhasil |
| 500x500        | 31,250,000     | 125,000,000   | 0.766666      | ✅ Berhasil |
| 1000x1000      | 250,000,000    | 1,000,000,000 | 4.765031      | ✅ Berhasil |

---

## 📘 Lesson Learned

* 🔁 Algoritma paralel seperti **SUMMA** sangat efektif untuk proses matriks besar.
* 📡 Komunikasi antar proses menggunakan `broadcast` dapat mengurangi beban sinkronisasi.
* 🧠 Manajemen memori (malloc & free) harus tepat, terutama untuk ukuran besar.
* 🔧 Program berjalan stabil hingga ukuran 1000x1000 tanpa error/crash.

---

## 📚 Referensi

Van De Geijn, R.A. and Watts, J. (1997).
**SUMMA: Scalable Universal Matrix Multiplication Algorithm**
Concurrency Practice and Experience, 9(4), pp.255–274.
[DOI Link](https://doi.org/10.1002/%28SICI%291096-9128%28199704%299:4<255::AID-CPE250>3.0.CO;2-2)

---

## ✅ Status

> 💯 Proyek ini berhasil dijalankan pada lingkungan multi-container Docker dengan hasil akurat dan performa efisien.

---

**Jangan lupa 🌟 star repo ini jika bermanfaat!**

```

