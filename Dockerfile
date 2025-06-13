FROM ubuntu:22.04

# Noninteractive frontend untuk apt
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    mpich \
    openssh-server \
    vim \
    iputils-ping \
    net-tools

# Setup SSH
RUN mkdir /var/run/sshd
RUN echo "root:root" | chpasswd
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

# Copy file matriks
COPY matriksA_2.csv /root/matriksA_2.csv
COPY matriksB_2.csv /root/matriksB_2.csv
COPY matriksA_100.csv /root/matriksA_100.csv
COPY matriksB_100.csv /root/matriksB_100.csv
COPY matriksA_500.csv /root/matriksA_500.csv
COPY matriksB_500.csv /root/matriksB_500.csv
COPY matriksA_1000.csv /root/matriksA_1000.csv
COPY matriksB_1000.csv /root/matriksB_1000.csv

# Copy program dan compile
COPY p13_044.c /root/p13_044.c
RUN mpicc /root/p13_044.c -o /root/p13_044 -lm

# Setup authorized_keys (akan diisi nanti dari docker-compose)
RUN mkdir -p /root/.ssh && chmod 700 /root/.ssh

# Start ssh + bash
CMD service ssh start && bash
