#!/usr/bin/env bash
# Compila TCMon01 e TCMon02 para o Arduino Nano (bootloader antigo) e gera
# os .hex em hex/. Se o repositório tcmonrust estiver ao lado deste, copia
# os .hex para tcmonrust/firmware/, onde são embutidos no executável.
set -euo pipefail
cd "$(dirname "$0")"

FQBN="arduino:avr:nano:cpu=atmega328old"
mkdir -p hex
for v in TCMon01 TCMon02; do
    arduino-cli compile --fqbn "$FQBN" --output-dir "build/$v" "$v"
    cp "build/$v/$v.ino.hex" "hex/$v.hex"
done

if [ -d ../tcmonrust/firmware ]; then
    cp hex/TCMon01.hex hex/TCMon02.hex ../tcmonrust/firmware/
    echo "Copiado para ../tcmonrust/firmware/"
fi
