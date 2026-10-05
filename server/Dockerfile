FROM ubuntu:24.04 AS build

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential cmake libssl-dev libpq-dev git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DEMBERDEEP_ENABLE_POSTGRES=ON -DEMBERDEEP_BUILD_CLIENT=OFF \
    && cmake --build build --target emberdeep_api -j2

FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libssl3 libpq5 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/server/emberdeep_api /app/emberdeep_api
COPY server/migrations /app/migrations

RUN useradd --system --uid 10001 emberdeep \
    && mkdir -p /var/lib/emberdeep \
    && chown -R emberdeep:emberdeep /app /var/lib/emberdeep
USER emberdeep

ENV EMBERDEEP_HOST=0.0.0.0
ENV EMBERDEEP_PORT=8080
ENV EMBERDEEP_MIGRATIONS=/app/migrations

EXPOSE 8080
ENTRYPOINT ["/app/emberdeep_api"]
