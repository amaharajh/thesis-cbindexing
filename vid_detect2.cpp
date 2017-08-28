// Background average sample code done with averages and done with codebooks
// Code sourced from: https://code.ros.org/trac/opencv/browser/trunk/opencv/samples/c/bgfg_codebook.cpp?rev=5420
// Code sourced from: http://anikettatipamula.blogspot.in/2012/02/hand-gesture-using-opencv.html
/* 
************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "opencv\cv.h"
#include "opencv\cvaux.h"
#include "opencv\cxmisc.h"
#include "opencv\highgui.h"

//VARIABLES for CODEBOOK METHOD:
CvBGCodeBookModel* model = 0;
const int NCHANNELS = 3;
bool ch[NCHANNELS]={true,true,true}; // This sets what channels should be adjusted for background bounds
CvCapture* capture = 0;
double tracker = 0.0, millis = 0.0;

//Global variables
const char* filename = 0;
IplImage* rawImage = 0, *yuvImage = 0; //yuvImage is for codebook method
IplImage *ImaskCodeBook = 0,*ImaskCodeBookCC = 0;
int c, nframes = 0;
int nframesToLearnBG = 300;
bool pause = false;
bool singlestep = false;


double  detect(IplImage* img_8uc1,IplImage* img_8uc3);

void help(void)
{
	printf("\nLearn background and find foreground using simple average and average difference learning method:\n"
		"\nUSAGE:\nbgfg_codebook [--nframes=300] [movie filename, else from camera]\n"
		"***Keep the focus on the video windows, NOT the consol***\n\n"
		"INTERACTIVE PARAMETERS:\n"
		"\tESC,q,Q  - quit the program\n"
		"\th	- print this help\n"
		"\tp	- pause toggle\n"
		"\ts	- single step\n"
		"\tr	- run mode (single step off)\n"
		"=== AVG PARAMS ===\n"
		"\t-    - bump high threshold UP by 0.25\n"
		"\t=    - bump high threshold DOWN by 0.25\n"
		"\t[    - bump low threshold UP by 0.25\n"
		"\t]    - bump low threshold DOWN by 0.25\n"
		"=== CODEBOOK PARAMS ===\n"
		"\ty,u,v- only adjust channel 0(y) or 1(u) or 2(v) respectively\n"
		"\ta	- adjust all 3 channels at once\n"
		"\tb	- adjust both 2 and 3 at once\n"
		"\ti,o	- bump upper threshold up,down by 1\n"
		"\tk,l	- bump lower threshold up,down by 1\n"
		"\tSPACE - reset the model\n"
		);
}

int initializeCodeBook(char* file){
	model = cvCreateBGCodeBookModel();

	//Set color thresholds to default values
	model->modMin[0] = 3;
	model->modMin[1] = model->modMin[2] = 3;
	model->modMax[0] = 10;
	model->modMax[1] = model->modMax[2] = 10;
	model->cbBounds[0] = model->cbBounds[1] = model->cbBounds[2] = 10;

	bool pause = false;
	bool singlestep = false;

	filename = file;

	if( !filename )
	{
		printf("Cannot start capture. Invalid file name.\n");
		//printf("Capture from camera\n");
		//capture = cvCaptureFromCAM( 0 );
	}
	else
	{
		printf("Capture from file %s\n",filename);
		capture = cvCreateFileCapture( filename );
	}

	if( !capture )
	{
		printf( "Can not initialize video capturing\n\n" );
		help();
		return -1;
	}
}

double runCodeBook(){
	if( !pause )
	{
		rawImage = cvQueryFrame( capture );
		++nframes;
		if(!rawImage) 
			return -2;
	}
	if( singlestep )
		pause = true;

	//First time:
	if( nframes == 1 && rawImage )
	{
		// CODEBOOK METHOD ALLOCATION
		yuvImage = cvCloneImage(rawImage);
		ImaskCodeBook = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
		ImaskCodeBookCC = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
		cvSet(ImaskCodeBook,cvScalar(255));

		cvNamedWindow( "Raw", 1 );
		cvNamedWindow( "ForegroundCodeBook",1);
		cvNamedWindow( "CodeBook_ConnectComp",1);
	}

	// If we've got an rawImage and are good to go:                
	if( rawImage )
	{
		cvCvtColor( rawImage, yuvImage, CV_BGR2YCrCb );//YUV For codebook method
		//This is where we build our background model
		if( !pause && nframes-1 < nframesToLearnBG  )
			cvBGCodeBookUpdate( model, yuvImage );

		if( nframes-1 == nframesToLearnBG  )
			cvBGCodeBookClearStale( model, model->t/2 );

		//Find the foreground if any
		if( nframes-1 >= nframesToLearnBG  )
		{
			// Find foreground by codebook method
			cvBGCodeBookDiff( model, yuvImage, ImaskCodeBook );
			// This part just to visualize bounding boxes and centers if desired
			cvCopy(ImaskCodeBook,ImaskCodeBookCC);	
			cvSegmentFGMask( ImaskCodeBookCC );
			//bwareaopen_(ImaskCodeBookCC,100);
			cvShowImage( "CodeBook_ConnectComp",ImaskCodeBookCC);
			double detectResult = detect(ImaskCodeBookCC,rawImage);

			if (detectResult != NULL){
				//std::cout<<detectResult<<std::endl;
				return detectResult;
			}

		}
		//Display
		cvShowImage( "Raw", rawImage );
		cvShowImage( "ForegroundCodeBook",ImaskCodeBook);

	}

	// User input:
	c = cvWaitKey(10)&0xFF;
	c = tolower(c);
	// End processing on ESC, q or Q
	if(c == 27 || c == 'q')
		return EXIT_FAILURE;
	//Else check for user input
	switch( c )
	{
	case 'h':
		help();
		break;
	case 'p':
		pause = !pause;
		break;
	case 's':
		singlestep = !singlestep;
		pause = false;
		break;
	case 'r':
		pause = false;
		singlestep = false;
		break;
	case ' ':
		cvBGCodeBookClearStale( model, 0 );
		nframes = 0;
		break;
	}//end of switch-case statement
	return EXIT_SUCCESS;
}

int releaseMemory(){
	cvReleaseCapture( &capture );
	cvReleaseImage(&rawImage);
	cvReleaseImage(&yuvImage);
	cvReleaseImage(&ImaskCodeBook);
	cvReleaseImage(&ImaskCodeBookCC);
	cvDestroyWindow( "Raw" );
	cvDestroyWindow( "ForegroundCodeBook");
	cvDestroyWindow( "CodeBook_ConnectComp");

	//cvDestroyAllWindows();
	return EXIT_SUCCESS;
}

double detect(IplImage* img_8uc1,IplImage* img_8uc3) {


	//cvNamedWindow( "aug", 1 );


	//cvThreshold( img_8uc1, img_edge, 128, 255, CV_THRESH_BINARY );
	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour = NULL;
	CvSeq* maxitem=NULL;
	double area=0,areamax=0;
	int maxn=0;
	int Nc = cvFindContours(
		img_8uc1,
		storage,
		&first_contour,
		sizeof(CvContour),
		CV_RETR_LIST // Try all four values and see what happens
		);
	int n=0;
	//printf( "Total Contours Detected: %d\n", Nc );

	if(Nc>0)
	{
		for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) 
		{

			//cvCvtColor( img_8uc1, img_8uc3, CV_GRAY2BGR );

			area=cvContourArea(c,CV_WHOLE_SEQ );

			if(area>areamax)
			{areamax=area;
			maxitem=c;
			maxn=n;
			}



			n++;


		}


		if(areamax>5000)
		{
			CvPoint pt0;

			CvMemStorage* storage1 = cvCreateMemStorage();
			CvSeq* ptseq = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour),
				sizeof(CvPoint), storage1 );
			CvSeq* hull;

			for(int i = 0; i < maxitem->total; i++ )
			{   CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, maxitem, i );
			pt0.x = p->x;
			pt0.y = p->y;
			cvSeqPush( ptseq, &pt0 );
			}
			hull = cvConvexHull2( ptseq, 0, CV_CLOCKWISE, 0 );
			int hullcount = hull->total;



			pt0 = **CV_GET_SEQ_ELEM( CvPoint*, hull, hullcount - 1 );

			for(int i = 0; i < hullcount; i++ )
			{

				CvPoint pt = **CV_GET_SEQ_ELEM( CvPoint*, hull, i );
				cvLine( img_8uc3, pt0, pt, CV_RGB( 0, 255, 0 ), 1, CV_AA, 0 );
				pt0 = pt;

				//Begin edit to show timestamp here
				millis = cvGetCaptureProperty(capture, CV_CAP_PROP_POS_MSEC);
			}


			cvReleaseMemStorage( &storage );
			cvReleaseMemStorage( &storage1 );

			//Run error measurement check here and return value if no error found. Value quoted here is in milliseconds
			if (millis - tracker >= 3000)
			{
				tracker = millis;
				return millis;
			}

		}
	}
	return NULL;
}
