
FROM emscripten/emsdk:latest as base

# Setup
RUN apt-get update && apt-get install -y \
	wget \
	xz-utils \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /app/examples/web

# copy npm configs first so that it doesn't run npm install if these have not changed # TODO test
# --> if we only change some code but no node packages we can still reuse the cached packages
COPY /examples/web/package.json /examples/web/package-lock.json ./

# npm ci - clean install - reads only package-lock.json - requires a manual npm install for every update of the package.json file # TODO keep or throw out
RUN npm ci

WORKDIR /app/

# we ensure the node_packages don't get copied at this step in the .dockerignore
COPY . .

WORKDIR /app/examples/web/


FROM base as dev

EXPOSE 5173

CMD ["npm" , "run", "dev:docker"]

FROM base as builder

RUN npm run deploy




FROM nginx:alpine as production

COPY examples/web/nginx.conf /etc/nginx/nginx.conf

COPY --from=builder /app/examples/web/dist /usr/share/nginx/html

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]


FROM base as test

# Install Zig
ARG ZIG_VERSION=0.15.2 
ARG ZIG_ARCH=x86_64
ARG ZIG_OS=linux

# download, extract move zig to global path
ARG p=https://ziglang.org/download/zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz
RUN echo ${p}
RUN wget -q https://ziglang.org/download/${ZIG_VERSION}/zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz 
RUN tar -xf zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz 
RUN mv zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION} /usr/local/zig 
RUN rm zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz

# add zig to path
ENV PATH="/usr/local/zig:${PATH}"

CMD ["npm" , "test"]


# Start with an image that has Emscripten (it's the hardest to install)
FROM base AS zig-build

# Install Zig (manually downloading binary)
ARG ZIG_VERSION=0.14.1 
ARG ZIG_ARCH=x86_64
ARG ZIG_OS=linux

# download, extract move zig to global path
ARG p=https://ziglang.org/download/zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz
RUN echo ${p}
RUN wget -q https://ziglang.org/download/${ZIG_VERSION}/zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz 
RUN tar -xf zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz 
RUN mv zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION} /usr/local/zig 
RUN rm zig-${ZIG_ARCH}-${ZIG_OS}-${ZIG_VERSION}.tar.xz

ENV PATH="/usr/local/zig:${PATH}"

# Run the build (Zig -> WASM -> Vite -> Dist)
# The emscripten/emsdk image sets $EMSDK automatically!
RUN npm run zig_build


FROM nginx:alpine as zig-production

COPY examples/web/nginx.conf /etc/nginx/nginx.conf

COPY --from=zig-build /app/examples/web/dist /usr/share/nginx/html

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]


