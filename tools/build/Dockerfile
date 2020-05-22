FROM ubuntu:20.04

RUN apt-get update -y                                                               && \
    apt-get upgrade -y                                                              && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y                                  \
      python3-pip                                                                      \
      vim                                                                              \
      cmake                                                                            \
      gcc-9

RUN pip3 install conan==1.25.1                                                      && \
    conan profile new default --detect                                              && \
    conan profile update settings.compiler.libcxx=libstdc++11 default               && \
    conan profile update settings.compiler.cppstd=20 default

WORKDIR /develop/src

EXPOSE 8080

ENV PATH=$PATH:/develop/src/tools/build
