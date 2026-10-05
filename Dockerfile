# astro-voyager reproducible build (bookworm, non-root).
# Runtime window needs host GPU, so image is for CI/build check + headless CLI.
# Build: docker build -t astro-voyager:0.1.0 .
# Run:   docker run --rm astro-voyager:0.1.0 --demo-geodesic
FROM debian:bookworm-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake g++ make \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)" \
    && ./build/astro-voyager --demo-geodesic

FROM debian:bookworm-slim
RUN useradd -r -s /usr/sbin/nologin app
COPY --from=build /src/build/astro-voyager /usr/local/bin/astro-voyager
COPY res/catalog.json /data/catalog.json
USER app
ENTRYPOINT ["/usr/local/bin/astro-voyager"]
CMD ["--list"]
