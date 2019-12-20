// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CenterOfMass.h"
#include "CoalaArea.h"
#include <cmath>

UCoalaGPSCoordinates* 
CenterOfMass::Get( const TArray<FVector>& vertices )
{
	TArray<UCoalaGPSCoordinates*> tmp;
	for( int i = 0; i < vertices.Num(); ++i )
	{
		UCoalaGPSCoordinates* pos = NewObject<UCoalaGPSCoordinates>();
		pos->lon = vertices[i].X;
		pos->lat = vertices[i].Y;
		tmp.Add( pos );
	}

	return CenterOfMass::Get(tmp);
}

UCoalaGPSCoordinates* 
CenterOfMass::Get( const TArray<UCoalaGPSCoordinates*>& pos )
{
	// from http://www.skycoyote.com/cntr/
	UCoalaGPSCoordinates* center = NewObject<UCoalaGPSCoordinates>();
	double area = 0;

	if( pos.Num() == 2 )
	{
		center->lon = (pos[0]->lon + pos[1]->lon) * 0.5;
		center->lat = (pos[0]->lat + pos[1]->lat) * 0.5;
		return center;
	}

	// loop through all traingles in figure
	for( int i = 2; i < pos.Num(); i++ )
	{
		UCoalaGPSCoordinates* p1 = pos[0];
		UCoalaGPSCoordinates* p2 = pos[i-1];
		UCoalaGPSCoordinates* p3 = pos[i];

		// compute area of triangle
		double a = CenterOfMass::get_area(p1, p2, p3);
		area += a;	// sum area
	
		//  only consider non-degenerate triangles
		if( a != 0 )
		{
			// compute center of triangle
			UCoalaGPSCoordinates* cntr = CenterOfMass::get_center(p1, p2, p3);
		
			// weight center by area, and add to center coordinates
			center->lon += a * cntr->lon;
			center->lat += a * cntr->lat;
		}
	}
	
	if( area != 0 )
	{
		center->lon /= area;	// renormalize coords by total area
		center->lat /= area;	// renormalize coords by total area
	}

	// area is actually a positive value
	area = std::abs( area );

	return center;
}

double
CenterOfMass::get_area(UCoalaGPSCoordinates* p1, UCoalaGPSCoordinates* p2, UCoalaGPSCoordinates* p3)
{
	double v1x = p2->lon - p1->lon;
	double v1y = p2->lat - p1->lat; 

	double v2x = p3->lon - p1->lon; 
	double v2y = p3->lat - p1->lat; 

	// return cross product of side vectors / 2
	return 0.5 * (v1x * v2y - v1y * v2x);
}

UCoalaGPSCoordinates*
CenterOfMass::get_center(UCoalaGPSCoordinates* p1, UCoalaGPSCoordinates* p2, UCoalaGPSCoordinates* p3)
{
	UCoalaGPSCoordinates* cntr = NewObject<UCoalaGPSCoordinates>();
	double x1, y1, x2, y2, x3, y3, x4, y4;

	x1 = p1->lon;	// get endpoints of first median
	y1 = p1->lat;	// get endpoints of first median
	x2 = (p2->lon + p3->lon) / 2.0;	// get endpoints of first median
	y2 = (p2->lat + p3->lat) / 2.0;	// get endpoints of first median

	x3 = p2->lon;	// get endpoints of second median (only need two)
	y3 = p2->lat;	// get endpoints of second median
	x4 = (p3->lon + p1->lon) / 2.0;	// get endpoints of second median
	y4 = (p3->lat + p1->lat) / 2.0;	// get endpoints of second median

	// see if either median is vertical (slope == infinity)
	if (x1 == x2)	// if so...
	{
		x1 = p3->lon;	// use third median (can't be two vertical medians)
		y1 = p3->lat; // use third median
		x2 = (p1->lon + p2->lon) / 2.0; // use third median
		y2 = (p1->lat + p2->lat) / 2.0; // use third median
	}
	else if (x3 == x4)
	{
		x3 = p3->lon;
		y3 = p3->lat;
		x4 = (p1->lon + p2->lon) / 2.0;
		y4 = (p1->lat + p2->lat) / 2.0;
	}
	
	double a1, a2, b1, b2;

	a1 = (y2 - y1) / (x2 - x1);	// compute slope of first median
	b1 = y1 - a1 * x1;	// compute intercept of first median

	a2 = (y4 - y3) / (x4 - x3);	// compute slope of second median
	b2 = y3 - a2 * x3;	// compute intercept of second median

	// solve a1 * x + b1 = a2 * x + b2

	cntr->lon = ((b2 - b1) / (a1 - a2));	// solve for x coordinate of intersection
	cntr->lat = (a1 * cntr->lon + b1);	// solve for y coordinate of intersection

	return cntr;	// return center
}