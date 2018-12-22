FROM debian:stretch as builder

RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-7 main" > /etc/apt/sources.list.d/docker.list \
&& apt-get update -y && apt-get install -y --no-install-recommends --allow-unauthenticated \
    libllvm7 llvm-7 llvm-7-dev llvm-7-runtime \
    clang-7 clang-format-7  \
    lld-7 \
    libc++-7-dev libc++abi-7-dev \
    cmake make zlib1g-dev libedit-dev ca-certificates git \
&& apt-get clean \
&& rm -rf /var/lib/apt/lists/* \
&& update-alternatives --install /usr/bin/cc cc /usr/bin/clang-7 100 \
&& update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-7 100 \
&& update-alternatives --install /usr/bin/ld ld /usr/bin/lld-7 100 \
&& ldconfig

ADD lib /opt/tyr/lib
ADD cmake /opt/tyr/cmake
ADD CMakeLists.txt /opt/tyr/CMakeLists.txt

WORKDIR /opt/tyr
RUN mkdir -p build && cd build \
&& cmake .. -DCMAKE_BUILD_TYPE=Release \
&& export LD_LIBRARY_PATH="/usr/lib/llvm-7/lib:${LD_LIBRARY_PATH}" \
&& make check install

FROM debian:stretch as test

RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-7 main" > /etc/apt/sources.list.d/docker.list \
&& apt-get update -y && apt-get install -y --no-install-recommends --allow-unauthenticated \
    make clang-7 lld-7 python3 python3-pip python3-setuptools \
&& apt-get clean \
&& rm -rf /var/lib/apt/lists/* \
&& pip3 install pip && pip3 install cffi numpy \
&& update-alternatives --install /usr/bin/cc cc /usr/bin/clang-7 100 \
&& update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-7 100 \
&& update-alternatives --install /usr/bin/ld ld /usr/bin/lld-7 100 \
&& ln -s /usr/bin/clang-7 /usr/bin/clang \
&& ln -s /usr/bin/clang++-7 /usr/bin/clang++

COPY --from=builder /usr/local/lib/tyr /usr/local/lib/tyr
COPY --from=builder /usr/local/include/tyr /usr/local/include/tyr
COPY --from=builder /usr/local/bin/tyr /usr/local/bin/tyr

ADD test /opt/test
WORKDIR /opt/test
RUN make test_c test_python test_x_lang

FROM debian:stretch as rt

RUN apt-get update -y && apt-get install -y --no-install-recommends \
    libffi6 libedit2 \
&& apt-get clean \
&& rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/tyr /usr/local/lib/tyr
COPY --from=builder /usr/local/include/tyr /usr/local/include/tyr
COPY --from=builder /usr/local/bin/tyr /usr/local/bin/tyr
COPY --from=builder /usr/lib/llvm-7/lib/libLLVM-7.so.1 /usr/lib/libLLVM-7.so.1

CMD ["bash"]
