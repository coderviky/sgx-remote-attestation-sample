# Intel SGX Remote Attestation End-to-End Sample

This is forked from [Intel's remote attestaion sample code](https://github.com/intel/sgx-ra-sample) (sgx-ra-sample repo)

For more information go to [sgx-ra-sample](https://github.com/intel/sgx-ra-sample) repo.

This sample demonstrates how to use Intel(R) Software Guard Extensions (Intel(R) SGX) remote attestation in an end-to-end scenario. 

### Prerequisites

-   Enable Intel SGX in BIOS/UEFI settings
-   Install SGX Linux Drivers ([Guide](https://docs.scrt.network/secret-network-documentation/infrastructure/node-runners/node-setup/install-sgx)) and [Intel SGX Linux SDK](https://github.com/intel/linux-sgx) & [Intel SGX Linux PSW](https://github.com/intel/linux-sgx) manually or using install-sgx.sh

    ```sh
    chmod +x ./install-sgx.sh

    # flags : Deps=true SDK=true PSW=true Driver=false
    sudo ./install-sgx.sh true true true false
    ```

### Setup
- generate key and save private key in private_key.pem and create pubkey.h for enclave

    ```bash
    make keygen
    ```

- download IAS certificate pem file

    ```bash
    wget https://certificates.trustedservices.intel.com/Intel_SGX_Attestation_RootCA.pem
    ```

- go to https://api.portal.trustedservices.intel.com/EPID-attestation & login
    -  Development Access -> Subscribe (linkable) here linkable imp!
    - go to https://api.portal.trustedservices.intel.com/developer   (for subscription data)
        - "service-provider/settings" file : 
    copy SPID, Primary Key, Secondary key and 
    set LINKABLE=1 

            ```bash
            SPID=your_spid

            PRIMARY_KEY=your_primary_key

            SECONDARY_KEY=your_secondary_key

            LINKABLE=1

            IAS_REPORT_SIGNING_CA_FILE=/path/Intel_SGX_Attestation_RootCA.pem

            ```


### Build and Run Service Provider & App (client)
 generate app binary file and run it

```bash
# generate files
  make

# run servie provider (server)
  ./run-server  --service-key-file=service-provider/sp_private_key.pem
  
# run app (client)
  ./app

# clean generated files
  make clean
```
