# syntel-os

SO bare-metal x86 minimalista, escrito do zero para fins educacionais.

## Estrutura

```
boot.asm      — bootloader 16-bit (carrega o kernel, entra em protected mode)
kernel.c      — entrada do kernel (32-bit)
screen.c      — driver VGA text mode (escreve em 0xb8000)
linker.ld     — script de linkagem (endereço base 0x1000)
Makefile      — build e execução local
Dockerfile    — ambiente de build + QEMU + noVNC
entrypoint.sh — hot reload dentro do container
```

## Pré-requisitos (local)

- `nasm`
- `gcc` com suporte a `-m32` (`gcc-multilib`)
- `binutils` (`ld`, `objcopy`)
- `qemu-system-x86`
- `make`

## Rodando localmente

**Build:**
```bash
make all
```

**Executar no QEMU** (terminal, modo curses):
```bash
make run
```

**Limpar artefatos:**
```bash
make clean
```

## Rodando com Docker (recomendado)

Não requer nenhuma dependência instalada além de Docker.

```bash
docker compose up
```

Abra `http://localhost:6080/vnc.html` no browser e clique em **Connect**.

O OS vai aparecer rodando no QEMU direto no browser.

### Hot reload

Com o container rodando, qualquer alteração salva em `.c`, `.asm` ou `.ld` dispara rebuild automático e reinicia o QEMU. Sem precisar rodar nenhum comando.

## Como funciona

1. O BIOS carrega o bootloader (`boot.asm`) no endereço `0x7c00`
2. O bootloader imprime uma mensagem via BIOS, lê o kernel do disco e configura a GDT
3. O processador entra em **protected mode 32-bit**
4. O kernel (`kernel_main`) é executado a partir do endereço `0x1000`
5. O driver VGA escreve diretamente na memória de vídeo (`0xb8000`)
