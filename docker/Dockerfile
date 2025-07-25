FROM ubuntu:latest
ARG VERSION
ARG ARCH

LABEL org.opencontainers.image.version=${VERSION}
LABEL org.opencontainers.image.architecture=${ARCH}

RUN apt update \
    && apt upgrade -y \
    && apt install -y curl

WORKDIR /opt

ADD https://github.com/heiher/natmap/releases/download/${VERSION}/natmap-linux-${ARCH} /opt/natmap
RUN chmod +x /opt/natmap

ENTRYPOINT ["/opt/natmap"]