FROM alpine:3.19 AS builder

RUN apk add --no-cache build-base cmake ninja linux-headers

WORKDIR /src
COPY . .

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --target turbokv_server

FROM alpine:3.19
RUN apk add --no-cache libstdc++

WORKDIR /app
COPY --from=builder /src/build/turbokv_server /app/turbokv_server

VOLUME ["/app/data"]
WORKDIR /app/data

EXPOSE 6389
ENTRYPOINT ["/app/turbokv_server", "--port", "6389", "--wal", "turbokv.wal"]
