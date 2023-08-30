

######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
else
        SGX_COMMON_CFLAGS += -O2
endif



ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

# --------------------- SGX SDK Settings End ---------------------


##--------------- Remote Attestation related Library ---------------------
# for enclave
KEY_EXCHANGE_LIB_NAME := sgx_tkey_exchange
# for APP
U_KEY_EXCHANGE_LIB_NAME := sgx_ukey_exchange
#----------------------------------------------------------

# --------------------- COMMON  ---------------------
COMMON_CPP_FILES =   service-provider/common.cpp  service-provider/msgio.cpp 

COMMON_C_FILES = service-provider/byteorder.c  service-provider/crypto.c  service-provider/hexutil.c   service-provider/fileio.c  service-provider/base64.c  service-provider/logfile.c

# GLIB_INCLUDE := -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
# OPENSSL_INCLUDE := -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto

#----------------------------------------------------------------------------------
######## Service Provider Settings ########
#---------------------------------------------------------------------------------
SP_CPP_FILES := service-provider/sp.cpp service-provider/agent_wget.cpp service-provider/iasrequest.cpp $(COMMON_CPP_FILES)

SP_C_FILES := service-provider/enclave_verify.c $(COMMON_C_FILES)

# sp_LDFLAGS= $(AM_LDFLAGS) @OPENSSL_LDFLAGS@ @CURL_LDFLAGS@
SP_Include_Paths :=  -IInclude -IApp -Iservice-provider -I$(SGX_SDK)/include

SP_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(SP_Include_Paths) -I/opt/intel/sgxsdk/include
SP_CPP_Flags := $(SP_C_Flags) -std=c++11

SGX_Capable_Lib := sgx_capable
SP_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lsgx_uae_service -lpthread -l$(SGX_Capable_Lib) -l$(KEY_EXCHANGE_LIB_NAME)  -lssl -lcrypto

SP_Cpp_Objects := $(SP_CPP_FILES:.cpp=.o) 
SP_C_Objects := $(SP_C_FILES:.c=.o)

SP_Name := sp
#---------------------Service Provider Settings END ------------------------------

#----------------------------------------------------------------------------------
######## MRSIGNER Settings ########
#---------------------------------------------------------------------------------
# MRSIGNER_CPP_FILES := service-provider/mrsigner.cpp

# no need to recompile the C files - already compiled in the SP
MRSIGNER_C_FILES := service-provider/mrsigner.c service-provider/crypto.c service-provider/hexutil.c

# MRSIGNER_CPP_Objects := $(MRSIGNER_CPP_FILES:.cpp=.o)
MRSIGNER_C_Objects := $(MRSIGNER_C_FILES:.c=.o)

MRSIGNER_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lsgx_uae_service -lpthread -l$(SGX_Capable_Lib) -l$(KEY_EXCHANGE_LIB_NAME)  -lssl -lcrypto

MRSIGNER_NAME := mrsigner

#-------------------------MRSIGNER Settings END ----------------------------------

#----------------------------------------------------------------------------------
######## App Settings ########
#----------------------------------------------------------------------------------
App_Cpp_Files := App/App.cpp
App_Cpp_Files += $(COMMON_CPP_FILES)

#client_SOURCES = client.cpp service-provider/sgx_detect_linux.c subdir-objects service-provider/sgx_stub.c subdir-objects service-provider/quote_size.c subdir-objects $(common) subdir-objects
App_C_Files := service-provider/sgx_detect_linux.c service-provider/sgx_stub.c service-provider/quote_size.c $(COMMON_C_FILES)

App_Include_Paths := -IInclude -IApp -I$(SGX_SDK)/include 

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11
# App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread 
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread -l$(U_KEY_EXCHANGE_LIB_NAME) -lcrypto \
	-l:libsgx_capable.a -lpthread -ldl -L/usr/lib/x86_64-linux-gnu -lglib-2.0


ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)
App_C_Objects := $(App_C_Files:.c=.o)

App_Name := app

# --------------------- App Settings End ---------------------

#----------------------------------------------------------------------------------
######## Enclave Settings ########
#----------------------------------------------------------------------------------
ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif
Crypto_Library_Name := sgx_tcrypto

Enclave_Cpp_Files := Enclave/Enclave.cpp
Enclave_C_Files := Enclave/Enclave.c
Enclave_Include_Paths := -IInclude -IEnclave -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/libcxx

CC_BELOW_4_9 := $(shell expr "`$(CC) -dumpversion`" \< "4.9")
ifeq ($(CC_BELOW_4_9), 1)
	Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections -fstack-protector
else
	Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections -fstack-protector-strong
endif

Enclave_C_Flags += $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++11 -nostdinc++

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) \
	-l$(KEY_EXCHANGE_LIB_NAME) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=Enclave/Enclave.lds

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)
Enclave_C_Objects := $(Enclave_C_Files:.c=.o)

Enclave_Name := enclave.so
Signed_Enclave_Name := enclave.signed.so
Enclave_Config_File := Enclave/Enclave.config.xml
Enclave_Test_Key := Enclave/Enclave_private_test.pem

# --------------------- Enclave Settings End ---------------------



######## Common Settings ########

ifeq ($(SGX_MODE), HW)
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = HW_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_PRERELEASE
else
	Build_Mode = HW_RELEASE
endif
else
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = SIM_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = SIM_PRERELEASE
else
	Build_Mode = SIM_RELEASE
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: .config_$(Build_Mode)_$(SGX_ARCH) $(SP_Name) policy $(App_Name) $(Enclave_Name) 
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Enclave_Name) first with your signing key before you run the $(App_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Enclave_Name) -out <$(Signed_Enclave_Name)> -config $(Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
all: .config_$(Build_Mode)_$(SGX_ARCH) $(SP_Name) policy $(App_Name) $(Signed_Enclave_Name)
ifeq ($(Build_Mode), HW_DEBUG)
	@echo "The project has been built in debug hardware mode."
else ifeq ($(Build_Mode), SIM_DEBUG)
	@echo "The project has been built in debug simulation mode."
else ifeq ($(Build_Mode), HW_PRERELEASE)
	@echo "The project has been built in pre-release hardware mode."
else ifeq ($(Build_Mode), SIM_PRERELEASE)
	@echo "The project has been built in pre-release simulation mode."
else
	@echo "The project has been built in release simulation mode."
endif
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(App_Name)
	@echo "RUN  =>  $(App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

#----------------------------------------------------------------------------------
######## Service Provider Objects ########
#----------------------------------------------------------------------------------
service-provider/%.o: service-provider/%.cpp
	@$(CXX) $(SP_CPP_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

service-provider/%.o: service-provider/%.c
	@$(CC) $(SP_C_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(SP_Name): $(SP_Cpp_Objects) $(SP_C_Objects)
	@$(CXX) $^ -o $@ $(SP_Link_Flags)
	@echo "LINK =>  $@"

#-------------------------- Service Provider Objects End --------------------------

#----------------------------------------------------------------------------------
######## MRSIGNER Objects ########
#----------------------------------------------------------------------------------
service-provider/mrsigner.o: service-provider/mrsigner.c
	@$(CXX) $(SP_CPP_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(MRSIGNER_NAME): $(MRSIGNER_C_Objects)
	@$(CXX) $^ -o $@ $(MRSIGNER_Link_Flags)
	@echo "LINK =>  $@"

#-------------------------- MRSIGNER Objects End --------------------------



#----------------------------------------------------------------------------------
######## App Objects ########
#----------------------------------------------------------------------------------
App/Enclave_u.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd App && $(SGX_EDGER8R) --untrusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

# /opt/intel/sgxsdk/bin/x64/sgx_edger8r --untrusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path /opt/intel/sgxsdk/include

App/Enclave_u.o: App/Enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

App/%.o: App/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name): App/Enclave_u.o $(App_Cpp_Objects) $(App_C_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

.config_$(Build_Mode)_$(SGX_ARCH):
	@rm -f .config_* $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Cpp_Objects) App/Enclave_u.* $(Enclave_Cpp_Objects) Enclave/Enclave_t.*
	@touch .config_$(Build_Mode)_$(SGX_ARCH)

#-------------------------- App Objects End --------------------------

#----------------------------------------------------------------------------------
######## Enclave Objects ########
#----------------------------------------------------------------------------------
Enclave/Enclave_t.c: $(SGX_EDGER8R) Enclave/Enclave.edl
	@cd Enclave && $(SGX_EDGER8R) --trusted ../Enclave/Enclave.edl --search-path ../Enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

Enclave/Enclave_t.o: Enclave/Enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

# Enclave/%.o: Enclave/%.cpp
# 	@$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
# 	@echo "CXX  <=  $<"

# $(Enclave_Name): Enclave/Enclave_t.o $(Enclave_Cpp_Objects)
# 	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
# 	@echo "LINK =>  $@"

# ------ for Enclave.c file ------
Enclave/%.o: Enclave/%.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC <=  $<"

$(Enclave_Name): Enclave/Enclave_t.o $(Enclave_C_Objects)
	@$(CC) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"
#--------------------------------

$(Signed_Enclave_Name): $(Enclave_Name)
ifeq ($(wildcard $(Enclave_Test_Key)),)
	@echo "There is no enclave test key<Enclave_private_test.pem>."
	@echo "The project will generate a key<Enclave_private_test.pem> for test."
	@openssl genrsa -out $(Enclave_Test_Key) -3 3072
endif
	@$(SGX_ENCLAVE_SIGNER) sign -key $(Enclave_Test_Key) -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

#-------------------------- Enclave Objects End --------------------------

#----------------------------------------------------------------------------------
#----------- Policy Objects -----------
policy: $(MRSIGNER_NAME) policy.in $(Signed_Enclave_Name)
	$(SGX_ENCLAVE_SIGNER) dump -cssfile enclave_sigstruct_raw -dumpfile /dev/null -enclave $(Signed_Enclave_Name)
	sed -e "s^@MRSIGNER@^`./mrsigner.sh enclave_sigstruct_raw`^" policy.in > policy
	rm -f enclave_sigstruct_raw 

#--------------------------------------

.PHONY: clean

clean:
	@rm -f .config_* $(App_Name) $(App_Name).so $(App_C_Objects) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Cpp_Objects) App/Enclave_u.* $(Enclave_Cpp_Objects) $(Enclave_C_Objects) Enclave/Enclave_t.* $(Enclave_Test_Key) $(SP_Name) $(SP_Cpp_Objects) $(SP_C_Objects) $(MRSIGNER_NAME) $(MRSIGNER_C_Objects) policy sp-keygen


.PHONY: so


so:
	g++ -shared -o $(App_Name).so App/Enclave_u.o $(App_Cpp_Objects) $(App_C_Objects) $(App_Link_Flags)


keygen:
	gcc -o sp-keygen service-provider/evp-key-to-file-struct.c -lssl -lcrypto && ./sp-keygen