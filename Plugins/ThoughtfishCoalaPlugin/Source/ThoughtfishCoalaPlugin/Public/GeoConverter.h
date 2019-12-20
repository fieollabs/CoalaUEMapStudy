// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#define DEG2RAD(a)   ((a) / (180 / M_PI))
#define RAD2DEG(a)   ((a) * (180 / M_PI))

#include "CoalaBlueprintUtility.h"
#include "CoreMinimal.h"
#include "CoalaArea.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeoConverter.generated.h"

/**
 * 
 */
UCLASS()
class UGeoConverter
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
		static int long2tilex(float lon, int z)
		{ 
			return (floor(((double)lon + 180.0) / 360.0 * pow(2.0, z))); 
		}
		
		UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
		static int lat2tiley(float lat, int32 z)
		{ 
			auto ret = (floor((1.0 - log( tan((double)lat * PI/180.0) + 1.0 / cos((double)lat * PI/180.0)) / PI) / 2.0 * pow(2.0, z))); 
			return ret;
		}

		UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
		static float tilex2long(int x, int z)
		{
			return (float)(x / pow(2.0, z) * 360.0 - 180);
		}

		UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
		static float tiley2lat(int y, int z)
		{
			double n = PI - 2.0 * PI * y / pow(2.0, z);
			return (float)(180.0 / PI * atan(0.5 * (exp(n) - exp(-n))));
		}
};

// https://wiki.openstreetmap.org/wiki/Mercator
struct MercatorConverter
{
	/* The following functions take their parameter and return their result in degrees */
	static double y2lat_d(double y)   { return RAD2DEG( atan(exp( DEG2RAD(y) )) * 2 - M_PI/2 ); }
	static double x2lon_d(double x)   { return x; }

	static double lat2y_d(double lat) { return RAD2DEG( log(tan( DEG2RAD(lat) / 2 +  M_PI/4 )) ); }
	static double lon2x_d(double lon) { return lon; }

	/* The following functions take their parameter in something close to meters, along the equator, and return their result in degrees */
	static double y2lat_m(double y)   { return RAD2DEG(2 * atan(exp( y/(COALA_MAP_SIZE*UCoalaBlueprintUtility::GetCoalaScale()) )) - M_PI/2); }
	static double x2lon_m(double x)   { return RAD2DEG(              x/(COALA_MAP_SIZE*UCoalaBlueprintUtility::GetCoalaScale())           ); }

	/* The following functions take their parameter in degrees, and return their result in something close to meters, along the equator */
	static double lat2y_m(double lat) { return log(tan( DEG2RAD(lat) / 2 + M_PI/4 )) * (COALA_MAP_SIZE*UCoalaBlueprintUtility::GetCoalaScale()); }
	static double lon2x_m(double lon) { return          DEG2RAD(lon)                 * (COALA_MAP_SIZE*UCoalaBlueprintUtility::GetCoalaScale()); }
};
