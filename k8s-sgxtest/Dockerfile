# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
FROM ubuntu:16.04
RUN apt update; apt -y install wget gnupg python apt-transport-https libssl1.0.0
RUN echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu xenial main' | tee /etc/apt/sources.list.d/intel-sgx.list && wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add -
RUN echo 'deb [arch=amd64] https://packages.microsoft.com/ubuntu/16.04/prod xenial main' | tee /etc/apt/sources.list.d/msprod.list && wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
RUN mkdir -p /etc/init; echo "#!/bin/bash" > /sbin/initctl; chmod +x /sbin/initctl
RUN apt update && apt -y install libsgx-enclave-common libsgx-dcap-ql az-dcap-client
COPY ./sgxtest/host/sgxtesthost ./sgxtest/enclave/sgxtestenc.signed /usr/local/sgx-k8s-test/
WORKDIR /usr/local/sgx-k8s-test
CMD ["./sgxtesthost", "sgxtestenc.signed"]
