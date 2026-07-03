#!/bin/bash

log() { echo "[$(date '+%H:%M:%S')] $*"; }

start_qemu() {
    pkill -f qemu-system-i386 2>/dev/null || true
    sleep 0.3
    qemu-system-i386 \
        -drive format=raw,file=build/os.img,snapshot=on \
        -display vnc=:0 \
        -monitor none &
    log "QEMU iniciado"
}

log "Build inicial..."
make all || { log "Build falhou"; exit 1; }

start_qemu

log "Iniciando noVNC na porta 6080..."
websockify --web=/usr/share/novnc 6080 localhost:5900 &

log "Acesse: http://localhost:6080/vnc.html"
log "Aguardando alterações em *.c, *.asm, *.ld ..."

inotifywait -m -e close_write -r /os --include='.*\.(c|asm|ld)$' |
while read -r dir event file; do
    log "Mudança: $file — rebuilding..."
    make all && start_qemu && log "OK" || log "Build falhou, QEMU mantido"
done
