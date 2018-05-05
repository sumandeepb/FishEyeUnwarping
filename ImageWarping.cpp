/*
    Copyright 2014-2018 Sumandeep Banerjee

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
Image warping for ultra wide angle lens

Date Created: August 13, 2014
Author: Sumandeep Banerjee
Email: sumandeep.banerjee@gmail.com
*/

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// methods
typedef enum EMethod { HEMICYLINDER, MIDPOINTCIRCLE };

// distortion models
typedef enum EDistortionModel { NODISTORTION = -1, EQUIDISTANT, EQUISOLID };

// print command line help
void PrintHelp()
{
	cout << "ImageWarping Usage :\n";
	cout << "ImageWarping.exe inputimagefile outputimagefile [method]\n";
	cout << "inputimagefile : input file name with path (*.bmp, *.png, *.jpg)\n";
	cout << "outputimagefile : output image file with path\n";
	cout << "[method] : optional correction algorithm\n";
	cout << "\t 0 - (Default) Hemicylinder method, requires no manual input, works only for full circular hemi images\n";
	cout << "\t 1 - Mid point circle method, requires manual marking of 12 points on circular boundary\n";
}

// click count
int g_cClicks = 0;

// mouse click capture
void CallBackFunc( int event, int x, int y, int flags, void* userdata )
{
	if  ( event == EVENT_LBUTTONDOWN )
	{
		vector<Point> *aPoints = (vector<Point> *)userdata;
		Point point;
		point.x = x;
		point.y = y;
		(*aPoints).push_back( point );
#ifdef _DEBUG
		cout << "Point No. " << g_cClicks << " - position (" << x << ", " << y << ")" << endl;
#endif
		g_cClicks++;
	}
	//else if  ( event == EVENT_RBUTTONDOWN )
	//{
	//     cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	//}
	//else if  ( event == EVENT_MBUTTONDOWN )
	//{
	//     cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	//}
	//else if ( event == EVENT_MOUSEMOVE )
	//{
	//     cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;
	//}
}

Point PointMean( const vector<Point> &aPoints )
{
	Point mean;
	for( unsigned int i = 0; i < aPoints.size(); i++ )
	{
		mean.x += aPoints[i].x;
		mean.y += aPoints[i].y;
	}
	mean.x /= aPoints.size();
	mean.y /= aPoints.size();

	return mean;
}

int main( int argc, char *argv[] )
{
	// check for valid command line arguments
	if( argc < 3 )
	{
		PrintHelp();
		// return with error
		return -1; 
	}

	// get file names from command line arguments
	String strInputFile = argv[1];
	String strOutputFile = argv[2];

	// load input image
	Mat	inputFrame = imread( strInputFile );
	if( !inputFrame.data )
	{
		cout << "Error in loading input file\n";
		PrintHelp();
		// return with error
		return -1;
	}

	// create output image buffer
	Mat outputFrame( inputFrame.rows, inputFrame.cols, inputFrame.type() );

#ifdef _DEBUG
	// show input frame
	imshow( "Input Frame", inputFrame );
#endif

	// allocate transfomation maps
	int nWidth = inputFrame.cols;
	int nHeight = inputFrame.rows;
	int Cx, Cy;
	Mat	transformMapX( nHeight, nWidth, CV_32FC1 );
	Mat	transformMapY( nHeight, nWidth, CV_32FC1 );

	// select correction algorithm
	EMethod eMethod = ( argc >= 4 )?(EMethod)atoi( argv[3] ): HEMICYLINDER;
	if( HEMICYLINDER == eMethod )
	{
		// distortion parameters
		EDistortionModel eDistModel = EQUIDISTANT;
		Cx = nWidth / 2;
		Cy = nHeight / 2;
		double F = (double)nWidth / CV_PI;

		// generate transformation map
		for( int v = 0; v < nHeight; v++ )
		{
			for( int u = 0; u < nWidth; u++ )
			{
				// implement hemi-cylinder target model
				double xt = double( u );
				double yt = double( v - Cy );

				double r = (double)nWidth / CV_PI;
				double alpha = double( nWidth - xt ) / r;
				double xp = r * cos( alpha );
				double yp = /*((double)nWidth / (double)nHeight) **/ yt;
				double zp = r * fabs( sin( alpha ) );

				double rp = sqrt( xp * xp + yp * yp );
				double theta = atan( rp / zp );

				double x1;
				double y1;
				// select lens distortion model
				switch( eDistModel )
				{
				case( EQUIDISTANT ):
					x1 = F * theta * xp / rp;
					y1 = F * theta * yp / rp;
					break;
				case( EQUISOLID ):
					x1 = 2.0 * F * sin( theta / 2.0 ) * xp / rp;
					y1 = 2.0 * F * sin( theta / 2.0 ) * yp / rp;		
					break;
				case( NODISTORTION ):
				default:
					x1 = xt;
					y1 = yt;
					break;
				};

				transformMapX.at<float>( v, u ) = (float)x1 + (float)Cx;
				transformMapY.at<float>( v, u ) = (float)y1 + (float)Cy;
			}
		}
	}
	else if( MIDPOINTCIRCLE == eMethod )
	{
		cout << "\nPlease mark 12 points on the boundary of the circle\n\n";

		// show input frame for marking circle
		Mat tempFrame = inputFrame.clone();
		imshow( "Input Frame", tempFrame );

		// initialize point list
		g_cClicks = 0;
		unsigned int nNumPoints = 12;
		vector<Point> aPoints;
		vector<bool> aPointDrawn;

		//set the callback function for mouse event
		setMouseCallback( "Input Frame", CallBackFunc, &aPoints );

		// wait till all 12 points input and any button is pressed
		do
		{
			waitKey(250);

			while( aPointDrawn.size() < aPoints.size() )
			{
				aPointDrawn.push_back( false );
			}

			for( unsigned int iPoints = 0; iPoints < aPointDrawn.size(); iPoints++ )
			{
				if( !aPointDrawn[iPoints] )
				{
					// draw cross mark at selected point locations
					int s = 5;
					line( tempFrame, Point( aPoints[iPoints].x - s, aPoints[iPoints].y - s),
						Point( aPoints[iPoints].x + s, aPoints[iPoints].y + s), CV_RGB( 255, 0, 0 ) );
					line( tempFrame, Point( aPoints[iPoints].x - s, aPoints[iPoints].y + s),
						Point( aPoints[iPoints].x + s, aPoints[iPoints].y - s), CV_RGB( 255, 0, 0 ) );
					//circle( tempFrame, aPoints[iPoints], 3, CV_RGB( 255, 0, 0 ) );
					aPointDrawn[iPoints] = true;
				}
			}

			imshow( "Input Frame", tempFrame );
		}while( aPoints.size() < nNumPoints );

		// reset the mouse callback
		setMouseCallback( "Input Frame", NULL, NULL );

		// compute summations
		Point mean = PointMean( aPoints );
		int sum_xi = 0, sum_yi = 0, sum_xi_2 = 0, sum_yi_2 = 0, sum_xi_3 = 0, sum_yi_3 = 0;
		int sum_xi_yi = 0, sum_xi_yi_2 = 0, sum_xi_2_yi = 0;
		for( unsigned int iPoints = 0; iPoints < nNumPoints; iPoints++ )
		{
			int xi = aPoints[iPoints].x - mean.x;
			int yi = aPoints[iPoints].y - mean.y;

			sum_xi += xi;
			sum_yi += yi;
			sum_xi_2 += xi * xi;
			sum_yi_2 += yi * yi;
			sum_xi_3 += xi * xi * xi;
			sum_yi_3 += yi * yi * yi;
			sum_xi_yi += xi * yi;
			sum_xi_yi_2 += xi * yi * yi;
			sum_xi_2_yi += xi * xi * yi;
		}

		// frame circle fitting as least squares problem
		Mat D( 3, 3, CV_64FC1 ), E( 3, 1, CV_64FC1 ), Q( 3, 1, CV_64FC1 );

		D.at<double>(0, 0) = double(2 * sum_xi);
		D.at<double>(0, 1) = double(2 * sum_yi);
		D.at<double>(0, 2) = double(nNumPoints);
		D.at<double>(1, 0) = double(2 * sum_xi_2);
		D.at<double>(1, 1) = double(2 * sum_xi_yi);
		D.at<double>(1, 2) = double(sum_xi);
		D.at<double>(2, 0) = double(2 * sum_xi_yi);
		D.at<double>(2, 1) = double(2 * sum_yi_2);
		D.at<double>(2, 2) = double(sum_yi);

		E.at<double>(0, 0) = double(sum_xi_2 + sum_yi_2);
		E.at<double>(1, 0) = double(sum_xi_3 + sum_xi_yi_2);
		E.at<double>(2, 0) = double(sum_xi_2_yi + sum_yi_3);

		// solve the least squares
		solve( D, E, Q , DECOMP_LU );
		double A = Q.at<double>(0, 0);
		double B = Q.at<double>(1, 0);
		double C = Q.at<double>(2, 0);

		// compute parameters
		Cx = (int)A + mean.x;
		Cy = (int)B + mean.y;
		int R = (int)sqrt (C + A * A + B * B);

#ifdef _DEBUG
		cout << "Cx = " << Cx << ", Cy = " << Cy << ", R = " << R << endl;
#endif

		// draw computed circle
		circle( tempFrame, Point( Cx, Cy ), R, CV_RGB( 0, 255, 0 ) );
		imshow( "Input Frame", tempFrame );

		// generate transformation map
		for( int u = 0; u < nWidth; u++ )
		{
			for( int v = 0; v < nHeight; v++ )
			{
				float x1, y1;
				float xt = float(u - Cx);
				float yt = float(v - Cy);
				if( xt != 0 ) // non limiting case
				{
					float AO1 = (xt * xt + float(R * R)) / (2.0f * xt);
					float AB = sqrt( xt * xt + float(R * R) );
					float AP = yt;
					float PE = float(R - yt);

					float a = AP / PE;
					float b = 2.0f * asin( AB / (2.0f * AO1) );

					float alpha = a * b / (a + 1.0f);
					x1 = xt - AO1 + AO1 * cos( alpha );
					y1 = AO1 * sin( alpha );
				}
				else // limiting case
				{
					x1 = (float)xt;
					y1 = (float)yt;
				}
				transformMapX.at<float>( v, u ) = x1 + (float)Cx;
				transformMapY.at<float>( v, u ) = y1 + (float)Cy;
			}
		}
	}
	else
	{
		cout << "Error in loading input file\n";
		PrintHelp();
		// return with error
		return -1;
	}

#ifdef _DEBUG
	imwrite( "MapX.bmp", transformMapX );
	imwrite( "MapY.bmp", transformMapY );
#endif

	// transform image
	Mat tempFrame( inputFrame.rows, inputFrame.cols, inputFrame.type() );
	remap( inputFrame, tempFrame, transformMapX, transformMapY, CV_INTER_CUBIC, BORDER_CONSTANT );

	if( MIDPOINTCIRCLE == eMethod )
	{
		// rescale y transform
		for( int u = 0; u < nWidth; u++ )
		{
			float min_y1 = (float)nHeight;
			float max_y1 = 0.0;
			for( int v = 0; v < nHeight; v++ )
			{
				float y1 = transformMapY.at<float>( v, u );
				// compute maximum y1
				if (y1 < min_y1)
				{
					min_y1 = y1;
				}
				// compute minimum y1
				if (y1 > max_y1)
				{
					max_y1 = y1;
				}
			}

			// compute range of y1
			float range = max_y1 - min_y1;

			// compress range to H
			float factor = range / (float)nHeight;
			//cout << u << " " << factor << ", ";
			factor = ( factor > 1.0f )?pow( factor, 1.0f ):factor;
			for( int v = 0; v < nHeight; v++ )
			{
				transformMapX.at<float>( v, u ) = (float)u;
				transformMapY.at<float>( v, u ) = float( v - Cy ) / factor + float(Cy);
			}
		}

		// save output file
		imwrite( "temp.jpg", tempFrame );

		remap( tempFrame, outputFrame, transformMapX, transformMapY, CV_INTER_CUBIC, BORDER_CONSTANT );
	}
	else
	{
		outputFrame = tempFrame;
	}

	// save output file
	imwrite( strOutputFile, outputFrame );

#ifdef _DEBUG
	// show output frame
	imshow( "Output Frame", outputFrame );
	waitKey();
#endif

	// return without error
	return 0;
}
