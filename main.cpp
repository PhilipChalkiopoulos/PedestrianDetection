#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"

#include "t_includes/terasic_includes.h"
#include "fbdraw.h"
#include "hps_0.h"
#include <stdio.h>

using namespace std;
using namespace cv;

int fd;
void *lw_axi_master = NULL;
void *h2f_axi_master = NULL;

volatile int* lw_fpga_led_addr;
volatile int* lw_fpga_mipi_pwdn_n;
volatile int* lw_fpga_mipi_rest_n;
volatile int* lw_fpga_mix_addr;
volatile int* lw_fpga_clp_addr;
volatile int* lw_fpga_frame_reader;
volatile int* lw_fpga_dma_control;
//volatile int* h2p_lw_dma_alpha_control;

volatile int* onchip_ram;

int a,i,j,k,sdata;
long l,o;
int loop_count;
int led_direction;
int black_level_0=0, black_level_1=0;
int led_mask;

//Image resolution
#define RES_X 320
#define RES_Y 240

int64 t0, ttdetect, ttread, ttconvert0, ttconvert1;
int64 ttdetect0, ttdetect1, ttshow0, ttshow1;
int64 ttdetect00, ttdetect11;

uint8_t start_mix = 0x01;
uint8_t stop_mix = 0x00;

//void *reg, *reg_value;
//TODO: delete HW_REGS_BASE
#define HW_REGS_BASE ( ALT_LWFPGASLVS_OFST )
#define HW_REGS_SPAN ( ALT_LWFPGASLVS_UB_ADDR - ALT_LWFPGASLVS_LB_ADDR + 1)

#define H2F_AXI_SPAN (ALT_FPGASLVS_UB_ADDR - ALT_FPGASLVS_LB_ADDR + 1)

#define IORD(base, index)		(*( ((uint32_t *)base)+index))
#define IOWR(base, index, data)	(*(((uint32_t *)base)+index) = data)

/** Function Headers */
void detectAndDisplay( Mat frame,dev_fb fb );
void display_image(dev_fb fb, Mat image, int offset_x, int offset_y);
void display_logo(String img, dev_fb fb );

double scale = 1.2;
const int gr_threshold = 2;
double hit_threshold = 1.1;

Size win_size(64, 128);
Size win_size_d(48, 96);
Size win_stride(4, 4);
Size padding(8,8);

//cv::HOGDescriptor hog(win_size, Size(16,16), Size(16,16), Size(16,16), 9, 1 );

cv::HOGDescriptor hog_d(Size(48, 96), Size(16, 16), Size(8, 8), Size(8, 8), 9);

uint32_t  temp2;
uint32_t led_data = 0x0;

/** @function main */
int main( int argc, const char** argv)
{
	dev_fb fb;
	fb_init(&fb);

	fb_fillScr(&fb,0,0,0);

	display_logo("upatras.bmp", fb);

	hog_d.winSize = Size(48, 96);
	hog_d.setSVMDetector(hog_d.getDaimlerPeopleDetector());
	cout << endl << "winSize of the hog_d Detector: " << hog_d.winSize << endl;

	// map the address space for the Hardware Components registers into user space so
	// we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various
	// registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	lw_axi_master = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

	if( lw_axi_master == MAP_FAILED ) {
		printf( "ERROR: Lightweight H2F AXI Bridge mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	h2f_axi_master = mmap( NULL, H2F_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, ALT_FPGASLVS_OFST );

	if( h2f_axi_master == MAP_FAILED ) {
		printf( "ERROR: H2F AXI Bridge mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	lw_fpga_led_addr	= (int*)(lw_axi_master + LED_BASE);
	lw_fpga_mipi_pwdn_n	= (int*)(lw_axi_master + MIPI_PWDN_N_BASE);
	lw_fpga_mipi_rest_n	= (int*)(lw_axi_master + MIPI_RESET_N_BASE);
	h2p_lw_mipi_camera	= (int*)(lw_axi_master + I2C_OPENCORES_CAMERA_BASE);
	h2p_lw_mipi_contrlr	= (int*)(lw_axi_master + I2C_OPENCORES_MIPI_BASE);
	lw_fpga_mix_addr	= (int*)(lw_axi_master + ALT_VIP_MIX_0_BASE);
	lw_fpga_clp_addr	= (int*)(lw_axi_master + ALT_VIP_CL_CLP_1_BASE);
	lw_fpga_dma_control = (int*)(lw_axi_master + VIDEO_DMA_CONTROLLER_0_BASE);

	onchip_ram	= (int*)(h2f_axi_master + ONCHIP_MEMORY_BASE);

	IOWR(lw_fpga_led_addr,int(0),int(0x3ff));

    //TODO:hardware gamma correction
/*
    double gamma_;
    cout << "* Enter the gamma value [0-100]: "; cin >> gamma_;

    Mat lookUpTable(1, 256, CV_8U);
	uchar* p = lookUpTable.ptr();
	for( int i = 0; i < 256; ++i)
		p[i] = saturate_cast<uchar>(pow((i / 255.0), gamma_) * 255.0);
	 for (i= 0; i<256; i+=4)
		IOWR(h2p_lw_gamma_control, i, p[i]);
		//or this: *(h2p_lw_gamma_control+i) = p[i];
*/


	// Detection parameters inputs
	cout << "\n* Enter the scale value [default: 1.2]: "; cin >> scale;
	//cout << "\n* Enter the hit_threshold value [default :1.5]: "; cin >> hit_threshold;


	IOWR(lw_fpga_clp_addr,int(0),int(0x0));

	usleep(20);
	//Enable the Alpha Blending Mixer Component
	IOWR(lw_fpga_mix_addr,0,0x01);
	usleep(20);

	//Din1 x,y, enable

	//Set x,y for the displayed Layer 1 of Mixer
	IOWR(lw_fpga_mix_addr,2,0x022B); //555
	usleep(20);
	IOWR(lw_fpga_mix_addr,3,0x96); //200
	usleep(20);

	//Display Layer 1 of Mixer
	IOWR(lw_fpga_mix_addr,4,0x01);
	usleep(20);


	IOWR(lw_fpga_mipi_pwdn_n, 0x00, 0x00);
	IOWR(lw_fpga_mipi_rest_n, 0x00, 0x00);

	usleep(200);
	IOWR(lw_fpga_mipi_pwdn_n, 0x00, 0xFF);
	usleep(200);
	IOWR(lw_fpga_mipi_rest_n, 0x00, 0xFF);

	usleep(200);

	// MIPI Init
	if (!MIPI_Init()){
		printf("MIPI_Init Init failed!\r\n");
	}else{
		printf("MIPI_Init successfully!\r\n");
		MIPI_BIN_LEVEL(3);
	}

	IOWR(lw_fpga_clp_addr,int(0),int(0x01));

	usleep(10);

	alt_u16 focus_value=0; //0-1023

	while(focus_value!=1050){
		cout << "\n* for camera focus value [0~1023] [1050 for skipping]: "; cin >> focus_value;
		if((focus_value>0) && (focus_value<1024)) OV8865_FOCUS_Move_to(focus_value);
	}
	usleep(20);


	//---------Set the Mat Array for the frame to detect --------//

	uchar image_array[RES_X*RES_Y];
	Mat img = Mat(RES_Y, RES_X, CV_8UC1, &image_array);
	printf( "Mat Array initialized...\n" );

	//TODO: check this
	//*image_array = *(uint8_t *)onchip_ram;

	int iterations;
	cout << "\n* Enter the preferred detect iterations: "; cin >> iterations;

	for (a=0;a<iterations;a++){ //pedestrian detect iterations TODO: Change to while 1;
		// calculate timings
		if (led_data == 0){
			led_data = 0x200;
		}else{
			led_data = (led_data >> 1);
		}
		IOWR(lw_fpga_led_addr,int(0),int(led_data));

		t0 = cv::getTickCount();

		for (j=0; j<RES_Y; j++){
			l=j*(RES_Y/3);
			o = j*RES_X;
			for(i=0; i<RES_X; i+=4){
				int m = i/4;

				temp2 = *(onchip_ram+m+l); //4 Bytes

				image_array[i+o]=uchar(temp2 & 0xff);
				image_array[i+1+o]=uchar((temp2>>8) & 0xff);
				image_array[i+2+o]=uchar((temp2>>16) & 0xff);
				image_array[i+3+o]=uchar((temp2>>24) & 0xff);
			}
		}

		ttread = cv::getTickCount();

		detectAndDisplay(img, fb);

		ttdetect = cv::getTickCount();

	}
	//-----------------------------------------------5-11-18---------------------------
	double det_secs = (ttdetect-t0)/cv::getTickFrequency();
	double read_secs = (ttread - t0)/cv::getTickFrequency();
	double conv_secs = (ttconvert1 - ttconvert0)/cv::getTickFrequency();
	double multiscale_sec = (ttdetect1 - ttdetect0)/cv::getTickFrequency();
	double multiscale_sec_d = (ttdetect11 - ttdetect00)/cv::getTickFrequency();
	double show_sec = (ttshow1 - ttshow0)/cv::getTickFrequency();

	cout << "\nHog Detection performed in: " << det_secs << " seconds\n";
	cout << "\nReading process performed in: " << read_secs << " seconds\n";
	cout << "\nConvert Gray2RGB performed in: " << conv_secs << " seconds\n";
	cout << "\nDetect Multiscale performed in: " << multiscale_sec << " seconds\n";
	cout << "\nDetect Multiscale with daimler performed in: " << multiscale_sec_d << " seconds\n";
	cout << "\nShow image process performed in: " << show_sec << " seconds\n";

	//-----------------------------------------------5-11-18---------------------------

	fb_close(&fb);
	if (munmap (lw_axi_master, HW_REGS_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
	if (munmap (h2f_axi_master, H2F_AXI_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
	close(fd);


	//-----------------------------------------------30-04-18---------------------------
    return 0;
}


void detectAndDisplay( Mat frame_gray, dev_fb fb )
{
    std::vector<Rect> rects_d;
    Mat frame_d;

    ttconvert0  = cv::getTickCount();
    cvtColor(frame_gray,frame_d,COLOR_GRAY2BGR);
    ttconvert1  = cv::getTickCount();

    ttdetect00  = cv::getTickCount();
    hog_d.detectMultiScale(frame_gray, rects_d, hit_threshold, win_stride, padding, scale, gr_threshold);
    for (size_t i = 0; i < rects_d.size(); i++){
    	rectangle(frame_d, rects_d[i], CV_RGB(0, 255, 0), 1);
    }
    ttdetect11  = cv::getTickCount();

    ttshow0 = cv::getTickCount();

    int channels = frame_d.channels();
    int nRows = frame_d.rows;
	int nCols = frame_d.cols * channels;
    int i,j,x,y;
	uchar r,g,b;

	for( i = 0; i < nRows; ++i)
	{
		for ( j = 0; j < nCols; j=j+3)
		{
			x = i ;
			y = j/3 ;

			r = frame_d.at<char>(i,j);
			g = frame_d.at<char>(i,j+1);
			b = frame_d.at<char>(i,j+2);

			fb_drawPixel(&fb,y+150,x+150,b,g,r); //if we want offset of the image x+ y+
		}
	}
	ttshow1 = cv::getTickCount();
}


void display_logo(String img, dev_fb fb ){

	Mat image;
	image = imread(img, CV_LOAD_IMAGE_COLOR);   // Read the file

	if(! image.data )                              // Check for invalid input
	{
		cout <<  "Could not open or find the image" << std::endl ;
	}

	int channels = image.channels();
	int nRows = image.rows;
	int nCols = image.cols * channels;
	int i,j,x,y;
	uchar r,g,b;
	for( i = 0; i < nRows; ++i)
	{
		for ( j = 0; j < nCols; j=j+3)
		{
			x = i ;
			y = j/3 ;

			r = image.at<char>(i,j);
			g = image.at<char>(i,j+1);
			b = image.at<char>(i,j+2);
			//if we want offset of the image x+ y+
			fb_drawPixel(&fb,y+106,x+450,b,g,r);
		}
	}
}


void display_image(dev_fb fb, Mat image, int offset_x, int offset_y){
	int channels = image.channels();
	int nRows = image.rows;
	int nCols = image.cols * channels;
	int i,j,x,y;
	uchar r,g,b;
	for( i = 0; i < nRows; ++i)
	{
		for ( j = 0; j < nCols; j=j+3)
		{
			x = i ;
			y = j/3 ;

			r = image.at<char>(i,j);
			g = image.at<char>(i,j+1);
			b = image.at<char>(i,j+2);
			//if we want offset of the image x+ y+
			fb_drawPixel(&fb,y+offset_y,x+offset_x,b,g,r);
		}
	}
}
