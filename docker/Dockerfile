FROM ubuntu:latest
ARG VERSION
ARG ARCH

RUN apt update \
    && apt upgrade -y \
    && apt install -y curl

WORKDIR /opt

ADD https://github.com/heiher/natmap/releases/download/${VERSION}/natmap-linux-${ARCH} /opt/natmap
RUN chmod +x /opt/natmap

ENTRYPOINT ["/opt/natmap"]
