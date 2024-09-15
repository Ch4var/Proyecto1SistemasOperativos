#!/bin/bash

# Verifica si el script está siendo ejecutado como root
if [ "$EUID" -ne 0 ]; then
  echo "Por favor, ejecuta este script como root (usa sudo)"
  exit 1
fi

# Actualiza los repositorios
echo "Actualizando los repositorios..."
dnf update -y

# Instala gcc y make
echo "Instalando gcc y make..."
dnf install -y gcc make

# Verifica si la instalación fue exitosa
if command -v gcc >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
  echo "gcc y make se han instalado correctamente."
else
  echo "Hubo un problema al instalar gcc y make."
fi
