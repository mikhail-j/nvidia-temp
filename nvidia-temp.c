/* Functions for NVIDIA device temperatures.
 *
 * Copyright (C) 2021 Blackbody Research LLC
 *        Author: Qijia (Michael) Jin
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * NOTICE TO USER:
 *
 * BLACKBODY RESEARCH MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS
 * SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  BLACKBODY RESEARCH DISCLAIMS ALL WARRANTIES 
 * WITH  REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL BLACKBODY RESEARCH BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH
 * THE USE OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */
#include <stdio.h>
#include <nvml.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#include "nvidia-temp.h"

static const struct option long_options[] = {
	{"verbose", 0, 0, 'V'},
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0},
};

static const char short_options[] = "hvV";

int main(int argc, char** argv) {
	bool verbose = false;
	nvmlReturn_t nvml_status;

	int option_char;
	while ((option_char = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (option_char) {
			case 'v':
				//initialize NVML library
				nvml_status = nvmlInit();
				if (nvml_status != NVML_SUCCESS) {
					printf("error: failed to initialize NVML library!\n");
					exit(1);
				}

				print_version();

				//destroy initialized NVML library
				nvmlShutdown();

				exit(0);
			case 'h':
				print_usage();
				exit(0);
			case 'V':
				verbose = true;
				break;
			default:
				fprintf(stderr, "Try 'nvidia-temp --help' for more information.\n");
				exit(127);
		}
	}

	//initialize NVML library
	nvml_status = nvmlInit();
	if (nvml_status != NVML_SUCCESS)
	{
		printf("error: failed to initialize NVML library!\n");
		return 1;
	}

	print_nvml_info(verbose);

	//destroy initialized NVML library
	nvmlShutdown();

	return 0;
}

void print_usage() {
	printf("Usage: nvidia-temp [OPTION]...\n"
			"Print NVIDIA device temperatures.\n"
			"\n"
			"  -V, --verbose  Display data with increased verbosity.\n"
			"  -h, --help     Display this help and exit.\n"
			"  -v, --version  Output version information and exit.\n"
			"\n");
	return;
}

void print_version() {
	nvmlReturn_t nvml_status;

	char* nvml_driver_version = (char *)malloc(sizeof(char) * (NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE));
	if (nvml_driver_version == NULL) {
		fprintf(stderr, "error: malloc(): out of memory!\n");
		exit(1);
	}

	nvml_status = nvmlSystemGetNVMLVersion(nvml_driver_version, NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE);
	if (nvml_status != NVML_SUCCESS)
	{
		fprintf(stderr, "error: %s\n!", nvmlErrorString(nvml_status));
		exit(1);
	}

	int cuda_driver_version;
	nvml_status = nvmlSystemGetCudaDriverVersion(&cuda_driver_version);
	if (nvml_status != NVML_SUCCESS)
	{
		fprintf(stderr, "error: %s\n!", nvmlErrorString(nvml_status));
		exit(1);
	}

	char* system_driver_version = (char *)malloc(sizeof(char) * (NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE));
	if (system_driver_version == NULL) {
		fprintf(stderr, "error: malloc(): out of memory!\n");
		exit(1);
	}

	nvml_status = nvmlSystemGetDriverVersion(system_driver_version, NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE);
	if (nvml_status != NVML_SUCCESS)
	{
		fprintf(stderr, "error: %s\n!", nvmlErrorString(nvml_status));
		exit(1);
	}

	printf("nvidia-temp version %d.%d\nNVML %s (NVIDIA %s - CUDA %d.%d)\n",
		NVIDIA_TEMP_MAJOR_VERSION,
		NVIDIA_TEMP_MINOR_VERSION,
		nvml_driver_version,
		system_driver_version,
		NVML_CUDA_DRIVER_VERSION_MAJOR(cuda_driver_version),
		NVML_CUDA_DRIVER_VERSION_MINOR(cuda_driver_version));

	free(system_driver_version);
	free(nvml_driver_version);

	return;
}

void print_nvml_info(bool verbose) {
	nvmlReturn_t nvml_status;

	unsigned int device_count;
	nvml_status = nvmlDeviceGetCount(&device_count);
	if (nvml_status != NVML_SUCCESS)
	{
		printf("error: %s\n!", nvmlErrorString(nvml_status));
		exit(1);
	}

	//allocate a new array based on the device count
	nvmlDevice_t* nvml_devices = (nvmlDevice_t *)malloc(sizeof(nvmlDevice_t) * device_count);
	if (nvml_devices == NULL) {
		fprintf(stderr, "error: malloc(): out of memory!\n");
		exit(1);
	}

	for (unsigned int i = 0; i < device_count; i++) {
		nvml_status = nvmlDeviceGetHandleByIndex(i, nvml_devices + i);
		if (nvml_status != NVML_SUCCESS) {
			printf("error: %s\n!", nvmlErrorString(nvml_status));
			exit(1);
		}
	}

	char* device_name_buffer = (char *)malloc(sizeof(char) * NVML_DEVICE_NAME_BUFFER_SIZE * device_count);
	if (nvml_devices == NULL) {
		fprintf(stderr, "error: malloc(): out of memory!\n");
		exit(1);
	}

	for (unsigned int i = 0; i < device_count; i++) {
		nvml_status = nvmlDeviceGetName(nvml_devices[i], device_name_buffer + (i * NVML_DEVICE_NAME_BUFFER_SIZE), NVML_DEVICE_NAME_BUFFER_SIZE);
		if (nvml_status != NVML_SUCCESS) {
			printf("error: %s\n!", nvmlErrorString(nvml_status));
			exit(1);
		}

		//setup query for device memory temperature
		nvmlFieldValue_t fv;
		fv.fieldId = NVML_FI_DEV_MEMORY_TEMP;

		fv.nvmlReturn = nvmlDeviceGetFieldValues(nvml_devices[i], 1, &fv);
		if (fv.nvmlReturn != NVML_SUCCESS){
			printf("error: %s\n!", nvmlErrorString(fv.nvmlReturn));
			exit(1);
		}
		
		unsigned int die;
		nvml_status = nvmlDeviceGetTemperature(nvml_devices[i], NVML_TEMPERATURE_GPU, &die);
		if (nvml_status != NVML_SUCCESS) {
			printf("error: %s\n!", nvmlErrorString(nvml_status));
			exit(1);
		}
		
		unsigned int fan_speed;
		nvml_status = nvmlDeviceGetFanSpeed(nvml_devices[i], &fan_speed);
		if (nvml_status != NVML_SUCCESS) {
			printf("error: %s\n!", nvmlErrorString(nvml_status));
			exit(1);
		}

		if (fv.valueType == NVML_VALUE_TYPE_UNSIGNED_INT) {
			if (verbose) {
				printf("GPU #%d [%s]: Fan Speed: %d%% | Die Temp: %dC | Memory Temp: %dC\n", i, device_name_buffer + (i * NVML_DEVICE_NAME_BUFFER_SIZE), fan_speed, die, fv.value.uiVal);
			}
			else {
				printf("GPU #%d [%s]: FAN: %d%% | DIE: %dC | MEMORY: %dC\n", i, device_name_buffer + (i * NVML_DEVICE_NAME_BUFFER_SIZE), fan_speed, die, fv.value.uiVal);
			}
		}
		else {
			//return type should always be unsigned int
			fprintf(stderr, "error: encountered corrupted NVML data!\n");
			exit(1);
		}
	}

	free(device_name_buffer);
	free(nvml_devices);

	return;
}
