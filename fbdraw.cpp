// fbDraw.c
// Keith Lee
// Dev Box

#include "fbdraw.h"

int fb_init(dev_fb* fb)
{
	fb->fbfd=0;
	fb->fbp=NULL;
	fb->fbfd=open("/dev/fb0", O_RDWR);

	if(fb->fbfd==-1)
		return FB_OPEN_FAIL;
	if(ioctl(fb->fbfd, FBIOGET_FSCREENINFO, &(fb->finfo))==-1)
		return FB_GET_FINFO_FAIL;
	if(ioctl(fb->fbfd, FBIOGET_VSCREENINFO, &(fb->vinfo))==-1)
		return FB_GET_VINFO_FAIL;

	fb->screensize=fb->vinfo.xres * fb->vinfo.yres * fb->vinfo.bits_per_pixel / 8;

	fb->fbp=(char*)mmap(0,fb->screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fbfd, 0);
	if((int)fb->fbp==-1)
		return FB_MMAP_FAIL;

	return 0;
}

pixel fb_toPixel(int x, int y)
{
	pixel px;
	px.x=x;
	px.y=y;
	return px;
}

int fb_checkPx(dev_fb* fb, int x, int y)
{
 return(x>=0 && y>=0 && x<fb->vinfo.xres && y<fb->vinfo.yres);
}

long int locate(dev_fb* fb, int x, int y)
{
	return (x+fb->vinfo.xoffset) * (fb->vinfo.bits_per_pixel/8) + (y+fb->vinfo.yoffset) * fb->finfo.line_length;
}

void fb_drawPixelPx(dev_fb* fb, pixel px, char r, char g, char b)
{
	fb_drawPixel(fb,px.x,px.y,r,g,b);
}

void fb_drawPixel(dev_fb* fb, int x, int y, char r, char g, char b)
{
	long int location=locate(fb,x,y);
	if(fb_checkPx(fb, x,y))
	{
		if(fb->vinfo.bits_per_pixel==32)
		{
			*(fb->fbp+location)=b;
			*(fb->fbp+location+1)=g;
			*(fb->fbp+location+2)=r;
			*(fb->fbp+location+3)=0;
		}
		else
		{
			unsigned short int t = r<<11 | g << 5 | b;
			*((unsigned short int*)(fb->fbp + location)) = t;
		}
	}
}

void fb_drawPixelwithAlpha(dev_fb* fb, int x, int y, char r, char g, char b, char a)
{
	long int location=locate(fb,x,y);
	if(fb_checkPx(fb, x,y))
	{
		if(fb->vinfo.bits_per_pixel==32)
		{
			*(fb->fbp+location)=b;
			*(fb->fbp+location+1)=g;
			*(fb->fbp+location+2)=r;
			*(fb->fbp+location+3)=a;
		}
		else
		{
			unsigned short int t = r<<11 | g << 5 | b;
			*((unsigned short int*)(fb->fbp + location)) = t;
		}
	}
}

void fb_fillScr(dev_fb* fb, char r, char g, char b)
{
	int x,y;
	long int location;
	for(y=0;y<fb->vinfo.yres;y++)
	{
		for(x=0;x<fb->vinfo.xres;x++)
		{
			location=locate(fb,x,y);
			*(fb->fbp+location)=b;
			*(fb->fbp+location+1)=g;
			*(fb->fbp+location+2)=r;
			*(fb->fbp+location+3)=0;
		}
	}
}

void fb_drawBox(dev_fb* fb, pixel px, int w, int h, char r, char g, char b)
{
	int x,y;

	for(x=0;x<w;x++)
		fb_drawPixel(fb,px.x+x,px.y,r,g,b);
	for(y=1;y<(h-1);y++)
	{
		fb_drawPixel(fb,px.x,px.y+y,r,g,b);
		fb_drawPixel(fb,px.x+(w-1),px.y+y,r,g,b);
	}
	for(x=0;x<w;x++)
		fb_drawPixel(fb,px.x+x,px.y+(h-1),r,g,b);
}

void fb_fillBox(dev_fb* fb, pixel px, int w, int h, char r, char g, char b)
{
	int x,y;
	for(y=0;y<h;y++)
	{
		for(x=0;x<w;x++)
		{
			fb_drawPixel(fb,px.x+x,px.y+y,r,g,b);
		}
	}
}



/*
	fb_drawLine: Bresenham's line equation
*/
void fb_drawLine(dev_fb* fb, pixel start, pixel end, char r, char g, char b)
{
	int error,x,y;
	char swap;
	char ydir;
	if(abs(end.y-start.y)>abs(end.x-start.x))
	{
		swap=1;
		error=start.x;
		start.x=start.y;
		start.y=error;
		error=end.x;
		end.x=end.y;
		end.y=error;
	}
	else swap=0;

	if(start.x>end.x)
	{
		error=start.x;
		start.x=end.x;
		end.x=error;
		error=start.y;
		start.y=end.y;
		end.y=error;
	}

	int dx=end.x-start.x;
	int dy=abs(end.y-start.y);
	error=-(dx/2);

	if(start.y<end.y)
		ydir=1;
	else
		ydir=-1;

	y=start.y;

	if(swap==1)
	{
		for(x=start.x;x<=end.x;x++)
		{
			fb_drawPixel(fb,y,x,r,g,b);
			error+=dy;
			if(error>0)
			{
				if(ydir==1)
					y++;
				else y--;
				error-=dx;
			}
		}
	}
	else
	{
		for(x=start.x;x<=end.x;x++)
		{
			fb_drawPixel(fb,x,y,r,g,b);
			error+=dy;
			if(error>0)
			{
				if(ydir==1)
				y++;
				else y--;
				error-=dx;
			}
		}
	}
}



void fb_drawChar(dev_fb* fb, char c, pixel start, short h, char r, char g, char b)
{
	short w=h/3;
	if(w%2)
		w++;
	pixel topRt, btmLt, btmRt, midLt, midRt;
	topRt.x=start.x+w;
	topRt.y=start.y;
	btmLt.x=start.x;
	btmLt.y=start.y+h;
	btmRt.x=start.x+w;
	btmRt.y=start.y+h;
	midLt=fb_toPixel(start.x, start.y+(h/2));
	midRt=fb_toPixel(topRt.x, midLt.y);

	switch(c)
	{
	case '0':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,btmLt,topRt,r,g,b);
		break;
	case '1':
		fb_drawLine(fb,fb_toPixel(start.x,start.y+(h/4)),fb_toPixel(start.x+(w/2),start.y),r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y),fb_toPixel(start.x+(w/2),start.y+h),r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case '2':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,midRt,r,g,b);
		fb_drawLine(fb,midRt,midLt,r,g,b);
		fb_drawLine(fb,midLt,btmLt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case '3':
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case '4':
		fb_drawLine(fb,topRt,midLt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		break;
	case 's':
	case 'S':
	case '5':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,start,midLt,r,g,b);
		fb_drawLine(fb,midRt,midLt,r,g,b);
		fb_drawLine(fb,midRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case '6':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,midRt,midLt,r,g,b);
		fb_drawLine(fb,midRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case '7':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmLt,r,g,b);
		break;
	case '8':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case '9':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,midLt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case '!':
		fb_drawLine(fb, fb_toPixel(start.x+(w/2), start.y), fb_toPixel(start.x+(w/2),btmRt.y-(h/4)),r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y+h),fb_toPixel(start.x+(w/2),start.y+h-1),r,g,b);
		break;
	case '?':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,midRt,r,g,b);
		fb_drawLine(fb,midRt,fb_toPixel(midLt.x+(w/4),midLt.y),r,g,b);
		fb_drawLine(fb, fb_toPixel(midLt.x+(w/4),midLt.y), fb_toPixel(start.x+(w/4),btmRt.y-(h/4)),r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/4),start.y+h),fb_toPixel(start.x+(w/4),start.y+h-1),r,g,b);
		break;
	case '.':
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y+h),fb_toPixel(start.x+(w/2)+1,start.y+h),r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y+h-1),fb_toPixel(start.x+(w/2)+1,start.y+h-1),r,g,b);
		break;
	case ',':
		fb_drawLine(fb,fb_toPixel(start.x+(w/2)+1,start.y+h),fb_toPixel(start.x+(w/2)+1,start.y+h-2),r,g,b);
		break;
	case '/':
		fb_drawLine(fb,btmLt,topRt,r,g,b);
		break;
	case'\\':
		fb_drawLine(fb,start,btmRt,r,g,b);
		break;
	case '(':
	case '{':
	case '[':
		fb_drawLine(fb,start,fb_toPixel(start.x+(w/2),start.y),r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,btmLt,fb_toPixel(start.x+(w/2),btmLt.y),r,g,b);
		break;
	case ')':
	case '}':
	case ']':
		fb_drawLine(fb,topRt,fb_toPixel(start.x+(w/2),start.y),r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmRt,fb_toPixel(start.x+(w/2),btmLt.y),r,g,b);
		break;
	case 'a':
	case 'A':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case 'b':
	case 'B':
		fb_drawLine(fb,start,fb_toPixel(topRt.x-(w/3),start.y),r,g,b);
		fb_drawLine(fb,fb_toPixel(topRt.x-(w/3),topRt.y),fb_toPixel(topRt.x,topRt.y+(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(topRt.x,topRt.y+(h/8)),fb_toPixel(topRt.x,midRt.y-(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(topRt.x,midRt.y-(h/8)),fb_toPixel(midRt.x-(w/3),midRt.y),r,g,b);
		fb_drawLine(fb,midLt,fb_toPixel(midRt.x-(w/3),midRt.y),r,g,b);
		fb_drawLine(fb,fb_toPixel(midRt.x-(w/3),midRt.y),fb_toPixel(midRt.x,midRt.y+(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(midRt.x,midRt.y+(h/8)),fb_toPixel(btmRt.x,btmRt.y-(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(btmRt.x,btmRt.y-(h/8)),fb_toPixel(btmRt.x-(w/3),btmRt.y),r,g,b);
		fb_drawLine(fb,btmLt,fb_toPixel(btmRt.x-(w/3),btmRt.y),r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		break;
	case 'c':
	case 'C':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		break;
	case 'd':
	case 'D':
		fb_drawLine(fb,start,fb_toPixel(topRt.x-(w/3),start.y),r,g,b);
		fb_drawLine(fb,fb_toPixel(topRt.x-(w/3),topRt.y),fb_toPixel(topRt.x,topRt.y+(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(topRt.x,topRt.y+(h/8)),fb_toPixel(btmRt.x,btmRt.y-(h/8)),r,g,b);
		fb_drawLine(fb,fb_toPixel(btmRt.x,btmRt.y-(h/8)),fb_toPixel(btmRt.x-(w/3),btmRt.y),r,g,b);
		fb_drawLine(fb,btmLt,fb_toPixel(btmRt.x-(w/3),btmRt.y),r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		break;
	case 'e':
	case 'E':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case 'f':
	case 'F':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case 'g':
	case 'G':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,midRt,fb_toPixel(midRt.x-(w/3),midRt.y),r,g,b);
		fb_drawLine(fb,midRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case 'h':
	case 'H':
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case 'i':
	case 'I':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y),fb_toPixel(start.x+(w/2),btmRt.y),r,g,b);
		break;
	case 'j':
	case 'J':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y),fb_toPixel(start.x+(w/2),btmRt.y),r,g,b);
		fb_drawLine(fb,btmLt,fb_toPixel(start.x+(w/2),btmRt.y),r,g,b);
		fb_drawLine(fb,midLt,btmLt,r,g,b);
		break;
	case 'k':
	case 'K':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,midLt,topRt,r,g,b);
		fb_drawLine(fb,midLt,btmRt,r,g,b);
		break;
	case 'l':
	case 'L':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	case 'm':
	case 'M':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,start,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		fb_drawLine(fb,topRt,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		break;
	case 'n':
	case 'N':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmRt,r,g,b);
		break;
	case 'o':
	case 'O':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		break;
	case 'p':
	case 'P':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,midRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		break;
	case 'q':
	case 'Q':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,fb_toPixel(midLt.x+w/2,midLt.y),btmRt,r,g,b);
		break;
	case 'r':
	case 'R':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,topRt,midRt,r,g,b);
		fb_drawLine(fb,midLt,midRt,r,g,b);
		fb_drawLine(fb,midLt,btmRt,r,g,b);
		break;
	case 't':
	case 'T':
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,fb_toPixel(start.x+(w/2),start.y),fb_toPixel(start.x+(w/2),btmRt.y),r,g,b);
		break;
	case 'u':
	case 'U':
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		fb_drawLine(fb,start,btmLt,r,g,b);
		break;
	case 'v':
	case 'V':
		fb_drawLine(fb,start,fb_toPixel(btmLt.x+w/2, btmLt.y),r,g,b);
		fb_drawLine(fb,topRt,fb_toPixel(btmLt.x+w/2, btmLt.y),r,g,b);
		break;
	case 'w':
	case 'W':
		fb_drawLine(fb,start,btmLt,r,g,b);
		fb_drawLine(fb,topRt,btmRt,r,g,b);
		fb_drawLine(fb,btmLt,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		fb_drawLine(fb,btmRt,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		break;
	case 'x':
	case 'X':
		fb_drawLine(fb,start,btmRt,r,g,b);
		fb_drawLine(fb,topRt,btmLt,r,g,b);
		break;
	case 'y':
	case 'Y':
		fb_drawLine(fb,start,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		fb_drawLine(fb,topRt,fb_toPixel(midLt.x+(w/2),midLt.y),r,g,b);
		fb_drawLine(fb,fb_toPixel(midLt.x+(w/2),midLt.y),fb_toPixel(midLt.x+(w/2),btmLt.y),r,g,b);
		break;
	case 'z':
	case 'Z':
		fb_drawLine(fb,topRt,btmLt,r,g,b);
		fb_drawLine(fb,start,topRt,r,g,b);
		fb_drawLine(fb,btmLt,btmRt,r,g,b);
		break;
	default:
		break;
	}
}

void fb_printStr(dev_fb* fb, const char* str, pixel* cursor, short height, char r, char g, char b)
{
	size_t l=strlen(str);
	short w=height/3;
	int i,j;
	int lnStart=cursor->x;
	if(w%2)
		w++;
	short c_offset=(2*w);

	for(i=0;i<l;i++)
	{
		if(str[i]=='\n')
		{
			cursor->y+=3*height/2;
			cursor->x=lnStart;
		}
		else if(str[i]=='\t')
		{
			for(j=0;j<4;j++)
			{
				fb_drawChar(fb,' ',*cursor,height,r,g,b);
				cursor->x+=c_offset;
				if(cursor->x+w>fb->vinfo.xres)
				{
					cursor->y+=3*height/2;
					cursor->x=lnStart;
					j=0;
				}
			}
		}
		else
		{
			fb_drawChar(fb,str[i],*cursor,height,r,g,b);
			cursor->x+=c_offset;
			if(cursor->x+w>fb->vinfo.xres)
			{
				cursor->y+=3*height/2;
				cursor->x=lnStart;
			}
		}
	}


}

void fb_close(dev_fb* fb)
{
	munmap(fb->fbp, fb->screensize);
	close(fb->fbfd);
}
