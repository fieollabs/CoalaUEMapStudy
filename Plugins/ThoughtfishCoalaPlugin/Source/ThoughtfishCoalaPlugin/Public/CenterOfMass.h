// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class CenterOfMass
{
	public:
		static class UCoalaGPSCoordinates* Get( const TArray<FVector>& vertices );
		static class UCoalaGPSCoordinates* Get( const TArray<UCoalaGPSCoordinates*>& pos );

	private:
		// helper
		static double get_area(class UCoalaGPSCoordinates* p1, class UCoalaGPSCoordinates* p2, class UCoalaGPSCoordinates* p3);
		static class UCoalaGPSCoordinates* get_center(class UCoalaGPSCoordinates* p1, class UCoalaGPSCoordinates* p2, class UCoalaGPSCoordinates* p3);
};
