#include "terasic_includes.h"
#include "stdint.h"
void IOWR(void *base_address, int NumOfReg, uint32_t data){
	base_address += NumOfReg*4;
	*(uint32_t *)base_address = data;
}

uint32_t IORD(void *base_address, int NumOfReg){
	base_address += NumOfReg*4;
	return *(uint32_t *)base_address;
}

uint8_t IO_8_read(void *base_address, int NumOfReg){
	uint8_t data;
	base_address += NumOfReg*4;
	data = *(uint8_t *)base_address;
	return data;
}

uint16_t IO_16_read(void *base_address, int NumOfReg){
	uint16_t data;
	base_address += NumOfReg*4;
	data = *(uint16_t *)base_address;
	return data;
}

uint32_t IO_32_read(void *base_address, int NumOfReg){
	uint32_t data;
	base_address += NumOfReg*4;
	data = *(uint32_t *)base_address;
	return data;
}

