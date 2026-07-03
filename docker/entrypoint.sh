#!/bin/bash

log() { echo "[$(date '+%H:%M:%S')] $*"; }

start_qemu() {
    pkill -f qemu-system-i386 2>/dev/null || true
    sleep 0.3
    qemu-system-i386 \
        -drive format=raw,file=build/os.img,snapshot=on \
        -vga std \
        -display vnc=:0 \
        -monitor none \
        -no-reboot &
    log "QEMU iniciado"
}

snapshot() {
    find /os/src -maxdepth 1 -type f | sort | xargs stat -c "%n:%Y:%s" 2>/dev/null
    stat -c "%n:%Y:%s" /os/Makefile 2>/dev/null
}

log "Build inicial..."
make -C /os all || { log "Build falhou"; exit 1; }
start_qemu

log "Iniciando noVNC na porta 6080..."
websockify --web=/usr/share/novnc 6080 localhost:5900 &

log "Acesse: http://localhost:6080/vnc.html"
log "Aguardando alteracoes (polling 1s)..."

LAST=$(snapshot)

while true; do
    sleep 1
    CURRENT=$(snapshot)
    if [ "$CURRENT" != "$LAST" ]; then
        log "Alteracao detectada — rebuilding..."
        make -C /os all && start_qemu && log "OK" || log "Build falhou, QEMU mantido"
        LAST=$(snapshot)
    fi
done
