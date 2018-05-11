#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "fbdraw.h"
#include "hps_0.h"

// located in /embedded/ip/altera/hps/altera_hps/hwlibs/include/soc_cv_av/socal
// in hps.h we can find the variables for the offset addresses of Lightweight and normal axi bus
/*
*/


extern "C" {

#include "t_includes/terasic_includes.h"

}

/*
void *h2p_lw_mipi_camera;
void *h2p_lw_mipi_contrlr;

*/

void *virtual_base;
int fd;
int i,j;
int loop_count;
int led_direction;
int led_mask;
uint8_t start_mix = 0x01;
uint8_t stop_mix = 0x00;
int reg1, reg_value1, step;
void *reg, *reg_value;
void *h2p_lw_led_addr;
void *h2p_lw_mipi_pwdn_n;
void *h2p_lw_mipi_rest_n;
void *h2p_lw_mix_addr;
void *h2p_lw_frame_reader;


#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )


#define IORD(base, index)			(*( ((uint32_t *)base)+index))
#define IOWR(base, index, data)     (*(((uint32_t *)base)+index) = data)


uint32_t calc_lw_address(uint32_t base_address){
	return ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + base_address ) & ( unsigned long)( HW_REGS_MASK ) );
	//h2p_lw_mix_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + ALT_VIP_MIX_0_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	// 			virtual base: 0x72e23000,  		 alt_lwfpga_ofst: 0xff200000,  led_pio_base: 0x10040,  hardware_regs_mask: : 0x3ffffff
	//																 	FF210040 & 3ffffff
	//																		  3210040
}

//#include <opencv2/gpu/gpu.hpp>

//#include "utility.hpp"

using namespace std;
using namespace cv;




void   *h2f_axi_master     = NULL;
size_t h2f_axi_master_span = ALT_FPGASLVS_UB_ADDR - ALT_FPGASLVS_LB_ADDR + 1;
size_t h2f_axi_master_ofst = ALT_FPGASLVS_OFST;

volatile int *sdram_data;



uint16_t mix_data;
uint32_t led_data = 0x0;

/** @function main */
int main( int argc, const char** argv)
{
	dev_fb fb;
	fb_init(&fb);
	reg1 = 0;
	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: lightweight mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	printf("the address of virtual base is: 0x%x \n", virtual_base);


	h2p_lw_led_addr		= virtual_base + calc_lw_address(LED_BASE);
	h2p_lw_mipi_pwdn_n	= virtual_base + calc_lw_address(MIPI_PWDN_N_BASE);
	h2p_lw_mipi_rest_n	= virtual_base + calc_lw_address(MIPI_RESET_N_BASE);
	h2p_lw_mipi_camera	= virtual_base + calc_lw_address(I2C_OPENCORES_CAMERA_BASE);
	h2p_lw_mipi_contrlr	= virtual_base + calc_lw_address(I2C_OPENCORES_MIPI_BASE);
	h2p_lw_mix_addr		= virtual_base + calc_lw_address(ALT_VIP_CL_MIXER_0_BASE);
	//h2p_lw_auto_focus	= virtual_base + calc_lw_address(TERASIC_AUTO_FOCUS_0_BASE);
	//h2p_lw_frame_reader = virtual_base + calc_lw_address(ALT_VIP_VFR_0_BASE);
	//h2p_lw_pio_reset_n	= virtual_base + calc_lw_address(PIO_RESET_BASE);

	printf("the address of  LW + Led Base is: 0x%x \n", calc_lw_address(LED_BASE));
	printf("the address of virtual + LW + Led Base is: 0x%x \n", h2p_lw_mipi_contrlr);

	IOWR(h2p_lw_led_addr,int(0),int(0x3f2));
	printf("\nLED_data: 0x%x\n",IORD(h2p_lw_mix_addr,int(0)));

	int mi=0;
	for (mi=0;mi<25;mi++){
		printf("Mixer II %d Register: 0x%x \n",mi,IORD(h2p_lw_mix_addr,mi));
	}


	usleep(20);
	IOWR(h2p_lw_mix_addr,0,int(0x01));
	usleep(20);

	IOWR(h2p_lw_mix_addr,8,0);
	usleep(20);
	IOWR(h2p_lw_mix_addr,9,0);
	usleep(20);

	IOWR(h2p_lw_mix_addr,10,1);
	usleep(20);

	IOWR(h2p_lw_mix_addr,13,0);
	usleep(20);
	IOWR(h2p_lw_mix_addr,14,0);
	usleep(20);

	IOWR(h2p_lw_mix_addr,15,1);
	usleep(20);

	IOWR(h2p_lw_mix_addr,18,0);
	usleep(20);
	IOWR(h2p_lw_mix_addr,19,0);
	usleep(20);

	IOWR(h2p_lw_mix_addr,20,1);
	usleep(20);

	for (mi=0;mi<25;mi++){
		IOWR(h2p_lw_mix_addr,mi,1);
		printf("Mixer II %d Register: 0x%x \n",mi,IORD(h2p_lw_mix_addr,mi));
	}

	printf("DE1-SoC D8M VGA Demo\n");


	IOWR(h2p_lw_mipi_pwdn_n, 0x00, 0x00);
	IOWR(h2p_lw_mipi_rest_n, 0x00, 0x00);

	usleep(200);
	IOWR(h2p_lw_mipi_pwdn_n, 0x00, 0xFF);
	usleep(200);
	IOWR(h2p_lw_mipi_rest_n, 0x00, 0xFF);



	usleep(200);


	printf("\nStart of...... MIPI_Init Initialization!\r\nPIO_RESET_BASE");
	// MIPI Init
	if (!MIPI_Init()){
		printf("MIPI_Init Init failed!\r\n");
	}else{
		printf("MIPI_Init Init successfully!\r\n");
	}



/*

		mipi_clear_error();
		usleep(50*1000);
		mipi_clear_error();
		usleep(1000);
		mipi_show_error_info();
		mipi_show_error_info_more();
		printf("\n");
*/


	usleep(10);
	//Focus_Init();




	loop_count = 0;
	led_mask = 0x01;
	led_direction = 0; // 0: left to right direction



	//-----------------------------------------------15-04-18---------------------------

	h2f_axi_master = mmap( NULL, h2f_axi_master_span, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, h2f_axi_master_ofst );

	if( h2f_axi_master == MAP_FAILED ) {
		printf( "ERROR: AXI Bridge mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	printf("the address of AXI Bridge is: 0x%x \n", h2f_axi_master);
	printf("the address of AXI Bridge + SDRAM Base is: 0x%x \n", h2f_axi_master + SDRAM_BASE);

	sdram_data	= (volatile int *)(h2f_axi_master + SDRAM_BASE);
	for (j=0; j<640; j++)
	for(i=0; i<480; i++){
		mix_data = *(sdram_data+i+j);
		fb_drawPixel(&fb,j,i,(mix_data>>24),(mix_data>>16),(mix_data>>8));
		//printf("Frame Reader data register %d: 0x%x \n",i,mix_data);
	}
	printf("the last of data: 0x%x -> 0x%x  0x%x  0x%x  0x%x \n",(mix_data), ((mix_data>>24) && 0xff), ((mix_data>>16) && 0xff) ,((mix_data>>8) && 0xff), ((mix_data) && 0xff));
	printf("the address of SDRAM: 0x%x \n", sdram_data);
	printf( "Draw everything\n" );
	fb_close(&fb);


	//-----------------------------------------------15-04-18---------------------------

	//-----------------------------------------------30-04-18---------------------------
	if (munmap (virtual_base, HW_REGS_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
	if (munmap (h2f_axi_master, h2f_axi_master_span) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
	close(fd);


	//-----------------------------------------------30-04-18---------------------------
    return 0;
}



