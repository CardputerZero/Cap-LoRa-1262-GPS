FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        binutils-aarch64-linux-gnu \
        ca-certificates \
        cmake \
        curl \
        dpkg-dev \
        g++-aarch64-linux-gnu \
        gcc-aarch64-linux-gnu \
        git \
        make \
        ninja-build \
        passwd \
        pkg-config \
        python3 \
        sudo \
    && rm -rf /var/lib/apt/lists/*

ARG HOST_UID=1000
ARG HOST_GID=1000

RUN if ! getent group "${HOST_GID}" >/dev/null; then \
        groupadd --gid "${HOST_GID}" cap-gps-builder; \
    fi \
    && if ! getent passwd "${HOST_UID}" >/dev/null; then \
        useradd --uid "${HOST_UID}" --gid "${HOST_GID}" --no-create-home \
            --shell /usr/sbin/nologin cap-gps-builder; \
    fi

ENV CMAKE_GENERATOR=Ninja
WORKDIR /workspace
ENTRYPOINT ["/workspace/packaging/docker/build_package.sh"]
