#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"

#include "fbdraw.h"
#include "hps_0.h"
#include <stdio.h>


using namespace std;
using namespace cv;

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
int a,i,j,k,sdata;
long l,o;
int loop_count;
int led_direction;
int black_level_0=0, black_level_1=0;
int led_mask;

int64 t0, ttdetect, ttread, ttconvert0, ttconvert1;
int64 ttdetect0, ttdetect1, ttshow0, ttshow1;
int64 ttdetect00, ttdetect11;

uint8_t start_mix = 0x01;
uint8_t stop_mix = 0x00;
int reg1, reg_value1, step;
void *reg, *reg_value;
void *h2p_lw_led_addr;
void *h2p_lw_mipi_pwdn_n;
void *h2p_lw_mipi_rest_n;
void *h2p_lw_mix_addr;
void *h2p_lw_clp_addr;
void *h2p_lw_frame_reader;
void *h2p_lw_dma_control;
void *h2p_lw_dma_alpha_control;

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )


#define IORD(base, index)			(*( ((uint32_t *)base)+index))
#define IOWR(base, index, data)     (*(((uint32_t *)base)+index) = data)




/** Function Headers */
void detectAndDisplay( Mat frame,dev_fb fb );


double scale = 1.2;
const int gr_threshold = 2;
double hit_threshold = 1.05;

Size win_size(64, 128);
Size win_size_d(48, 96);
Size win_stride(4, 4);
Size padding(8,8);
int framecount = 0;

//cv::HOGDescriptor hog(win_size, Size(16,16), Size(16,16), Size(16,16), 9, 1 );

cv::HOGDescriptor hog, hog_d(Size(48, 96), Size(16, 16), Size(8, 8), Size(8, 8), 9);

//Mat img(Size(320,240),CV_8UC1);



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




void   *h2f_axi_master     = NULL;int win_size_=32, win_stride_=4, padd_=8;
size_t h2f_axi_master_span = ALT_FPGASLVS_UB_ADDR - ALT_FPGASLVS_LB_ADDR + 1;
size_t h2f_axi_master_ofst = ALT_FPGASLVS_OFST;

volatile int* sdram_data;



uint32_t mix_data, temp2;

uint32_t led_data = 0x0;

/** @function main */
int main( int argc, const char** argv)
{
	dev_fb fb;
	fb_init(&fb);

	hog.setSVMDetector(hog.getDefaultPeopleDetector());
	cout << endl << "winSize of the hog Detector: " << hog.winSize << endl;

	hog_d.winSize = Size(48, 96);
	hog_d.setSVMDetector(hog.getDaimlerPeopleDetector());
	cout << endl << "winSize of the hog_d Detector: " << hog_d.winSize << endl;

	//printf("hog detector was set...");

	//printf("hog descriptor block stride: %dx%d \n",hog.blockStride.height, hog.blockStride.width);
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

	//printf("the address of virtual base is: 0x%x \n", virtual_base);


	h2p_lw_led_addr		= virtual_base + calc_lw_address(LED_BASE);
	h2p_lw_mipi_pwdn_n	= virtual_base + calc_lw_address(MIPI_PWDN_N_BASE);
	h2p_lw_mipi_rest_n	= virtual_base + calc_lw_address(MIPI_RESET_N_BASE);
	h2p_lw_mipi_camera	= virtual_base + calc_lw_address(I2C_OPENCORES_CAMERA_BASE);
	h2p_lw_mipi_contrlr	= virtual_base + calc_lw_address(I2C_OPENCORES_MIPI_BASE);
	h2p_lw_mix_addr		= virtual_base + calc_lw_address(ALT_VIP_MIX_0_BASE);
	h2p_lw_clp_addr		= virtual_base + calc_lw_address(ALT_VIP_CL_CLP_1_BASE);
	h2p_lw_dma_control  = virtual_base + calc_lw_address(VIDEO_DMA_CONTROLLER_0_BASE);
	h2p_lw_dma_alpha_control  = virtual_base + calc_lw_address(VIDEO_DMA_CONTROLLER_1_BASE);
	//h2p_lw_gamma_control = virtual_base + calc_lw_address();
	//h2p_lw_auto_focus	= virtual_base + calc_lw_address(TERASIC_AUTO_FOCUS_0_BASE);
	//h2p_lw_frame_reader = virtual_base + calc_lw_address(ALT_VIP_VFR_0_BASE);
	//h2p_lw_pio_reset_n	= virtual_base + calc_lw_address(PIO_RESET_BASE);

/*
  	printf("the address of  LW + Led Base is: 0x%x \n", calc_lw_address(LED_BASE));
	printf("the address of virtual + LW + Led Base is: 0x%x \n", h2p_lw_mipi_contrlr);
*/
	IOWR(h2p_lw_led_addr,int(0),int(0x3f2));
/*
	printf("\nLED_data: 0x%x\n",IORD(h2p_lw_led_addr,0));

	printf("\nDMA Buffer Register: 0x%x\n",IORD(h2p_lw_dma_control,0));
	printf("\nDMA BackBuffer Register: 0x%x\n",IORD(h2p_lw_dma_control,1));
	printf("\nDMA Resolution Register: 0x%x\n",IORD(h2p_lw_dma_control,2));
	printf("\nDMA Status Register: 0x%x\n\n",IORD(h2p_lw_dma_control,3));

	printf("\nDMA Alpha Buffer Register: 0x%x\n",IORD(h2p_lw_dma_alpha_control,0));
	printf("\nDMA Alpha BackBuffer Register: 0x%x\n",IORD(h2p_lw_dma_alpha_control,1));
	printf("\nDMA Alpha Resolution Register: 0x%x\n",IORD(h2p_lw_dma_alpha_control,2));
	printf("\nDMA Alpha Status Register: 0x%x\n\n",IORD(h2p_lw_dma_alpha_control,3));
*/

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


	IOWR(h2p_lw_clp_addr,int(0),int(0x0));

	/*int mi=0;
	for (mi=0;mi<25;mi++){
		printf("Mixer II %d Register: 0x%x \n",mi,IORD(h2p_lw_mix_addr,mi));
	}*/

	usleep(20);
	IOWR(h2p_lw_mix_addr,0,0x01);
	usleep(20);

	//Din1 x,y, enable

	IOWR(h2p_lw_mix_addr,2,0x12C);
	usleep(20);
	IOWR(h2p_lw_mix_addr,3,0xC8);
	usleep(20);

	IOWR(h2p_lw_mix_addr,4,0x01);
	usleep(20);


/*

	//Din2 x,y, enable

	IOWR(h2p_lw_mix_addr,5,0x12C);	//0x12C	->	300
	usleep(20);
	IOWR(h2p_lw_mix_addr,6,0xC8);	//0xC8	->	200
	usleep(20);

	IOWR(h2p_lw_mix_addr,7,0x01);
	usleep(20);

*/



	//printf("DE1-SoC D8M VGA Demo\n");


	IOWR(h2p_lw_mipi_pwdn_n, 0x00, 0x00);
	IOWR(h2p_lw_mipi_rest_n, 0x00, 0x00);

	usleep(200);
	IOWR(h2p_lw_mipi_pwdn_n, 0x00, 0xFF);
	usleep(200);
	IOWR(h2p_lw_mipi_rest_n, 0x00, 0xFF);



	usleep(200);


	//printf("\nStart of...... MIPI_Init Initialization!\r\nPIO_RESET_BASE");
	// MIPI Init
	if (!MIPI_Init()){
		printf("MIPI_Init Init failed!\r\n");
	}else{
		printf("MIPI_Init Init successfully!\r\n");
		MIPI_BIN_LEVEL(3);
	}

	IOWR(h2p_lw_clp_addr,int(0),int(0x01));
/*
	usleep(5000000);

	IOWR(h2p_lw_mix_addr,4,0x0);
	usleep(100000);
	IOWR(h2p_lw_clp_addr,int(0),int(0x0));
*/
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

	//IOWR(h2p_lw_clp_addr,int(0),int(0x00));



	//cout << "\n* for camera black level 0 [integer]: "; cin >> black_level_0;
	//cout << "\n* for camera black level 1 [integer]: "; cin >> black_level_1;
	//BLC_LEVEL(black_level_0,black_level_1);

	alt_u16 focus_value=0; //0-1023

	while(focus_value!=1050){
		cout << "\n* for camera focus value [0~1023] [-1 for skipping]: "; cin >> focus_value;
		if((focus_value>0) && (focus_value<1024)) OV8865_FOCUS_Move_to(focus_value);
	}
	usleep(20);

	//-----------------------------------------------15-04-18---------------------------

	h2f_axi_master = mmap( NULL, h2f_axi_master_span, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, h2f_axi_master_ofst );

	if( h2f_axi_master == MAP_FAILED ) {
		printf( "ERROR: AXI Bridge mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	//printf("the address of AXI Bridge is: 0x%x \n", h2f_axi_master);
	//printf("the address of AXI Bridge + SDRAM Base is: 0x%x \n", h2f_axi_master + ONCHIP_MEMORY_BASE);

	sdram_data	= (int*)(h2f_axi_master + ONCHIP_MEMORY_BASE);

	uchar image_array[320*240];
	Mat img = Mat(240, 320, CV_8UC1, &image_array);
	printf( "Mat Array initialized...\n" );


	char pixel1,pixel2,pixel3,pixel4;
	int iterations;
	cout << "\n* Enter the preferred detect iterations: "; cin >> iterations;

for (a=0;a<iterations;a++){ //pedestrian detect iterations TODO: Change to while 1;
	// calculate timings
	t0 = cv::getTickCount();

	for (j=0; j<240; j++){
		l=j*80;
		o = j*320;
		for(i=0; i<320; i+=4){
			int m = i/4;
			//mix_data = *(sdram_data+i+j);//*(((uint32_t *)sdram_data)+i+j);
			temp2 = *(sdram_data+m+l);
			pixel1= uchar(temp2 & 0xff);
			pixel2= uchar((temp2>>8) & 0xff);
			pixel3= uchar((temp2>>16) & 0xff);
			pixel4= uchar((temp2>>24) & 0xff);

			/*fb_drawPixel(&fb,i,j,pixel1,pixel1,pixel1);
			fb_drawPixel(&fb,i+1,j,pixel2,pixel2,pixel2);
			fb_drawPixel(&fb,i+2,j,pixel3,pixel3,pixel3);
			fb_drawPixel(&fb,i+3,j,pixel4,pixel4,pixel4);
*/
			image_array[i+o]=pixel1;
			image_array[i+1+o]=pixel2;
			image_array[i+2+o]=pixel3;
			image_array[i+3+o]=pixel4;
		}
	}
	ttread = cv::getTickCount();

	//-----------------------------------------------15-04-18---------------------------

	//-----------------------------------------------30-05-18---------------------------

/*
	usleep(5000000);
	printf("\nThe 500th data of image array %d\n",image_array[500]);
	printf("\nThe size of image array %d\n",sizeof(image_array));

	uchar gray;
	for( i = 0; i < 240; ++i)
		{
		o = i*320;
			for ( j = 0; j < 320; j++)
			{
				gray=image_array[o+j];

				//printf("[%d][%d],%d,%d,%d\n",y,x,point,point+1,point+2);
				fb_drawPixel(&fb,i,j,gray,gray,gray);
			}
	}*/


	//memcpy(img.data,&image_array,sizeof(image_array));

//	printf("\nThe addresses for images %d %d\n",image_array,img.data);

/*	int channels = img.channels();

	int nRows = img.rows;
	int nCols = img.cols * channels;*/
/*

	printf("\nrows and columns of the image array: %d  -- %d \n",nRows,nCols);
	printf("\nchannels of the image array: %d \n",channels);

*/
	//p = inputImage.ptr<uchar>(0);
	//p = img.ptr<uchar>(r);

//	usleep(5000000);
/*

	for( i = 0; i < nRows; ++i)
	{
		o = i*nCols;
		for ( j = 0; j < nCols; j++)
		{

			gray = img.at<uchar>(o+j);

			//printf("[%d][%d],%d,%d,%d\n",y,x,point,point+1,point+2);
			fb_drawPixel(&fb,j,i,gray,gray,gray);
		}
	}

*/
//	printf("\nThe 50th data of img mat %d\n",img.at<char>(500));
	//-----------------------------------------------30-05-18---------------------------


	//-----------------------------------------------01-06-18---------------------------

    //hog.setSVMDetector(hog.getDefaultPeopleDetector());

 //   printf("hog detector was set...");

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


void detectAndDisplay( Mat frame_gray, dev_fb fb )
{



    std::vector<Rect> rects,rects_d;
    Mat frame,frame_d;
    /*
    dev_fb fb;
    fb_init(&fb);
    */

    //GammaCorrection(frame,frame,5);

    //cvtColor( frame, frame_gray, COLOR_BGR2GRAY );

    //equalizeHist( frame_gray, frame_gray );
    //cvtColor(frame_gray,frame,COLOR_GRAY2BGR);
    ttconvert0  = cv::getTickCount();
    //cvtColor(frame_gray,frame,COLOR_GRAY2BGR);
    cvtColor(frame_gray,frame_d,COLOR_GRAY2BGR);
    ttconvert1  = cv::getTickCount();

    //people_cascade.detectMultiScale( frame_gray, rects, 1.1, 3, 0|CASCADE_SCALE_IMAGE, Size(60, 30) );
    ttdetect0  = cv::getTickCount();
    //hog.detectMultiScale(frame_gray, rects, hit_threshold, win_stride, padding, scale, gr_threshold);
    //cout << "Frame: " << framecount++<< "     Person Detected: " << rects.size() << endl;
    ttdetect1  = cv::getTickCount();

    ttdetect00  = cv::getTickCount();
    hog_d.detectMultiScale(frame_gray, rects_d, hit_threshold, win_stride, padding, scale, gr_threshold);
    for (size_t i = 0; i < rects_d.size(); i++){
    	rectangle(frame_d, rects_d[i], CV_RGB(0, 255, 0), 1);
    	//printf("rect data: x: %d, y: %d, height: %d, width:%d\n", rects[i].x, rects[i].y, rects[i].height, rects[i].width);
    }
    ttdetect11  = cv::getTickCount();

    int channels = frame.channels();
    //printf("\nnumber of people detected: %d\n", rects.size());
    ttshow0 = cv::getTickCount();
    int nRows = frame.rows;
	int nCols = frame.cols * channels;
    int i,j,x,y;
	uchar r,g,b;
	/*
	//uchar* p;
	//p = inputImage.ptr<uchar>(0);
	//p = frame.ptr<uchar>(r,g,b);
	for( i = 0; i < nRows; ++i)
	{
		for ( j = 0; j < nCols; j=j+3)
		{
			x = i ;
			y = j/3 ;

			r = frame.at<char>(i,j);
			g = frame.at<char>(i,j+1);
			b = frame.at<char>(i,j+2);
			//printf("[%d][%d],%d,%d,%d\n",y,x,point,point+1,point+2);
			fb_drawPixel(&fb,y,x,b,g,r); //if we want offset of the image x+ y+
		}
	}
*/

	channels = frame_d.channels();
	//printf("\nnumber of people detected: %d\n", rects.size());
	nRows = frame_d.rows;
	nCols = frame_d.cols * channels;

	for( i = 0; i < nRows; ++i)
	{
		for ( j = 0; j < nCols; j=j+3)
		{
			x = i ;
			y = j/3 ;

			r = frame_d.at<char>(i,j);
			g = frame_d.at<char>(i,j+1);
			b = frame_d.at<char>(i,j+2);
			//printf("[%d][%d],%d,%d,%d\n",y,x,point,point+1,point+2);
			fb_drawPixel(&fb,y,x+350,b,g,r); //if we want offset of the image x+ y+
		}
	}
	ttshow1 = cv::getTickCount();
	//printf( "Draw everything\n" );
}


