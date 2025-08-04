FROM ubuntu:latest
ARG NATMAP_ARCH

LABEL org.opencontainers.image.source="https://github.com/heiher/natmap"

RUN apt update \
    && apt upgrade -y \
    && apt install -y curl

WORKDIR /opt
COPY ./tmp/artifact/natmap-linux-${NATMAP_ARCH}/natmap-linux-${NATMAP_ARCH} /opt/natmap
RUN chmod +x /opt/natmap

ENTRYPOINT ["/opt/natmap"]