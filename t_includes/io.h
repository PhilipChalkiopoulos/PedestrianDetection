#include "alt_types.h"  // alt_u32
#include "terasic_includes.h"


void IOWR(void *base_address, int NumOfReg, uint32_t data);

uint32_t IORD(void *base_address, int NumOfReg);

uint8_t IO_8_read(void *base_address, int NumOfReg);

uint16_t IO_16_read(void *base_address, int NumOfReg);

uint32_t IO_32_read(void *base_address, int NumOfReg);
