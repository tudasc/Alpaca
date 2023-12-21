FROM ubuntu:latest
WORKDIR /opt

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && apt-get upgrade -y && apt-get install -y gcc g++ gdb cmake cmake-curses-gui python3 apt-utils wget gnupg git autoconf automake libtool zlib1g-dev zlib1g vim unzip python3-pip python3-pytest python3-pytest-cov openmpi-bin openmpi-common bison flex python2

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| apt-key add - && apt-get install -y libllvm-12-ocaml-dev libllvm12 llvm-12 llvm-12-dev llvm-12-doc llvm-12-examples llvm-12-runtime clang-12 clang-tools-12 clang-12-doc libclang-common-12-dev libclang-12-dev libclang1-12 clang-format-12 python3-clang-12 clangd-12 clang-tidy-12

RUN ln -s /usr/bin/clang-12 /usr/bin/clang && ln -s /usr/bin/clang++-12 /usr/bin/clang++ && ln -s /usr/lib/llvm-12/build/utils/lit/lit.py /usr/bin/lit && ln -s /usr/bin/opt-12 /usr/bin/opt && ln -s /usr/bin/FileCheck-12 /usr/bin/FileCheck

RUN apt-get install -y bear
RUN apt-get install -y cmake

COPY . /app
WORKDIR /app
RUN mkdir build
WORKDIR /app/build
RUN cmake ..
RUN make
# put the executable into the PATH
ENV PATH="/app/build:${PATH}"

# download the git project to analyse
WORKDIR /app
# RUN wget and instantly unpack the tar.gz as a folder named "version1"
RUN wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.5.tar.gz && tar -xzf openmpi-4.1.5.tar.gz && mv openmpi-4.1.5 version1
RUN rm openmpi-4.1.5.tar.gz
# RUN wget and instantly unpack the tar.gz as a folder named "version2"
RUN wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.5.tar.gz && tar -xzf openmpi-4.1.5.tar.gz && mv openmpi-4.1.5 version2
# delete the tar.gz file
RUN rm openmpi-4.1.5.tar.gz

WORKDIR /app/version1
RUN ./configure
# builds the compilation database
RUN bear -- make

# build version2
WORKDIR /app/version2
RUN ./configure
# builds the compilation database
RUN bear -- make