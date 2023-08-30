import ctypes

# Load the shared library
app_lib = ctypes.CDLL('./app.so')

# Specify the argument and return types for the add_nums function
app_lib.add_nums.argtypes = [ctypes.c_int, ctypes.c_int]
app_lib.add_nums.restype = ctypes.c_int

# Call the add_nums function
result = app_lib.add_nums(5, 7)
print("Result of add_nums function:", result)


# Specify the return type for the attestation_with_custom_config function
app_lib.attestation_with_custom_config.restype = ctypes.c_int

# Call the attestation_with_custom_config function
result_attestation = app_lib.attestation_with_custom_config()
print("Result of attestation_with_custom_config function:", result_attestation)