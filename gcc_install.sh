#!/bin/bash

# Verifica si el script está siendo ejecutado como root
if [ "$EUID" -ne 0 ]; then
  echo "Este script necesita permisos de root, reiniciando con sudo..."
  exec sudo "$0" "$@"  # Relanza el script con sudo
  exit 1
fi

# Instala gcc y make
echo "Instalando gcc y make..."
dnf install -y gcc make

# Verifica si la instalación fue exitosa
if command -v gcc >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
  echo "gcc y make se han instalado correctamente."
else
  echo "Hubo un problema al instalar gcc y make."
  exit 1
fi

# Usa el directorio actual
PROYECTO_DIR=$(pwd)

# Verifica si el directorio del proyecto contiene un Makefile
if [ -f "$PROYECTO_DIR/Makefile" ]; then
  echo "Ejecutando make en el directorio $PROYECTO_DIR..."
  make
else
  echo "No se encontró un Makefile en el directorio actual ($PROYECTO_DIR). No se puede ejecutar make."
  exit 1
fi
