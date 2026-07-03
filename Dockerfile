FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    nasm \
    gcc \
    gcc-multilib \
    binutils \
    qemu-system-x86 \
    inotify-tools \
    novnc \
    websockify \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /os

COPY . .

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 6080

ENTRYPOINT ["/entrypoint.sh"]
