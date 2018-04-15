FROM debian:stretch

LABEL maintainer="mgeitz" \
      version="0.1.6" \
      description="This image is used to start the albumfs executable"

RUN groupadd -g 999 albumfs && \
    useradd -r -u 999 -g albumfs albumfs

RUN apt-get update -y && apt-get install -y \
    libfuse-dev \
    libpng-dev \
    libssl-dev \
    pkg-config \
    build-essential

WORKDIR /bin/albumfs

COPY . .

RUN make all && \
    make install && \
    chown -R albumfs:albumfs ./

USER albumfs

CMD /bin/bash
