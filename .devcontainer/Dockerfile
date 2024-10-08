# Verwende ein Basis-Debian-Image
FROM debian:bullseye

# Installiere grundlegende Pakete
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    git \
    python3 \
    python3-pip \
    sudo \
    && apt-get clean

# Erstelle den 'vscode' Benutzer
RUN useradd -m vscode \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Setze den Benutzer auf 'vscode'
USER vscode

# Setze das Arbeitsverzeichnis
WORKDIR /workspace
