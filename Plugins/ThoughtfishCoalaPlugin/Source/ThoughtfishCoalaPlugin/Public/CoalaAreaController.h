// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include <map>
#include "CoalaRemoteTileRequest.h"
#include "CoreMinimal.h"
#include <cmath> 
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaAreaController.generated.h"

/**
 * 
 */
UCLASS()
class UCoalaAreaController : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
		// ret: new areas to request from coala
		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void OnGpsPositionChanged( uint8 zoom, float lon, float lat, TArray<FCoalaRemoteTileRequest>& newAreasInRange, TArray<FCoalaRemoteTileRequest>& areasOutOfRange, int buffer = 1 );

		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void AddKnownArea( class UCoalaArea* area );
		
		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void MoveCharacter( float lon, float lat, class ACharacter* character );

		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void InitCoala();

		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void CleanupCoala();

		// adding gametags to array (not clearing it befor!)
		UFUNCTION(BlueprintCallable, Category="Coala|AreaController")
		static void GetGametagsFromGpsPosition( uint8 zoom, float lon, float lat, TArray<FString>& gametags );
		
		static FVector GetGpsOffset();
	private:
		static FVector gps_offset;
		static std::map<FCoalaRemoteTileRequest, class UCoalaArea*> known_areas;
};
