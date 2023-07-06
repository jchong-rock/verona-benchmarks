FROM ubuntu:22.04
ARG USERNAME=verona

# Install dependencies
RUN apt update \
    && apt upgrade -y \
    && DEBIAN_FRONTEND=noninteractive apt install -y cmake ninja-build llvm-12 clang-12 lld-12 git lsb-release curl sudo pip cloc

# Install Python dependencies
RUN pip install psutil plotly pandas kaleido

# Create a non-root user
RUN useradd -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
USER $USERNAME

# Build the artifact directory
RUN sudo mkdir artifact \
    && sudo chown $USERNAME artifact
WORKDIR artifact

# Build Verona Savina benchmarks
RUN git clone https://github.com/ic-slurp/verona-benchmarks.git \
    && cd verona-benchmarks \
    && git checkout artifact \
    && mkdir build \
    && cd build \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12 .. \
    && ninja

# Build Verona Runtime benchmark
RUN git clone https://github.com/Microsoft/verona-rt.git \
    && cd verona-rt \
    && mkdir build \
    && cd build \
    && cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12 .. \
    && ninja test/perf-con-dining_phil

# Install PonyUp
RUN export SHELL=/bin/bash \
    && sh -c "$(curl --proto '=https' --tlsv1.2 -sSf https://raw.githubusercontent.com/ponylang/ponyup/latest-release/ponyup-init.sh)"

# Install Pony
RUN $HOME/.local/share/ponyup/bin/ponyup update ponyc release-0.53.0

# Build Pony Savina
RUN export CC=/usr/bin/clang-12 \
    && git clone https://github.com/sblessing/pony-savina.git \
    && cd pony-savina \
    && git checkout boc \
    && cd savina-pony \
    && $HOME/.local/share/ponyup/bin/ponyc

COPY README.md README.md

ENV SHELL /bin/bash
CMD /bin/bash
