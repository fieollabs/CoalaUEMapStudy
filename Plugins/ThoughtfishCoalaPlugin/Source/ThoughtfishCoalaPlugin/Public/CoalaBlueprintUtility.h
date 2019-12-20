// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaArea.h"
#include "CoalaRemoteTileRequest.h"
#include "BluePrintHttpGetRequest.h"
#include "CoalaBlueprintUtility.generated.h"


/**
 * 
 */
UENUM(BlueprintType)
enum class EOutputPins_CoalaRequestResult : uint8
{
	OnSuccess,
	OnError
};

UENUM( BlueprintType, Meta = (Bitflags) )
enum class REQUEST_CONTEXT : uint8
{
	GAMETAGS = 1,	
	POIS,		
	WEATHER,	
	STREETS,	
	BUILDINGS,	
	WATER,		
	CONTEXT,	
	TIMEZONE
};
ENUM_CLASS_FLAGS( REQUEST_CONTEXT )

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaBlueprintUtility
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:

	UFUNCTION(BlueprintCallable, Category="Coala|Utility", meta=(ExpandEnumAsExecs="Branches"))
	static class UCoalaArea* RequestCoalaApi( class UCoalaRequestBase* request_type, EOutputPins_CoalaRequestResult& Branches )
	{ return 0;
	
	}

	UFUNCTION(BlueprintPure, Category = "Coala|Utility")
	static class UCoalaArea* AreaFlowControl(UCoalaArea* areaAsync, UCoalaArea* area, bool AsyncRendering)
	{
		if (AsyncRendering)
		{
			return areaAsync;
		}
		return area;
	}

	UFUNCTION(BlueprintPure, Category="Coala|Utility")
	static FCoalaRemoteTileRequest CreateRemoteTileRequestFromIndex( uint8 zoom, int tile_x, int tile_y )
	{ return FCoalaRemoteTileRequest(); }

	UFUNCTION(BlueprintPure, Category = "Coala|Utility")
		static FCoalaRemoteTileRequest CreateRemoteTileRequestFromGpsCoordinates(uint8 zoom, float lon, float lat);

	// requests functions V1
	UFUNCTION(BlueprintPure, Category="Coala|Utility")
	static class UGolfcourseRequest* CreateGolfcourseRequest( FString api_key, FString course_id )
	{ return 0; }

	UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
	static class UVectorTileRequest* CreateVectorTileRequest( FString api_key, FCoalaRemoteTileRequest tile )
	{ return 0; }

	UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
	static class UWeatherRequest* CreateWeatherRequest( FString api_key, FCoalaRemoteTileRequest tile )
	{ return 0; }

	UFUNCTION( BlueprintPure, Category = "Coala|Utility" )
	static class UGeofencingRequest* CreateGeofencingRequest( FString api_key, float lon, float lat, FString firebaseDeviceToken )
	{ return 0; }

	// requests functions V2
	UFUNCTION(BlueprintCallable, Category = "Coala|Utility", meta = (ExpandEnumAsExecs = "Branches"))
	static UBluePrintHttpGetRequest* MakeCoalaRequest(FString api_key, FCoalaRemoteTileRequest tile, UPARAM(meta = (Bitmask, BitmaskEnum = "REQUEST_CONTEXT")) int32 context, EOutputPins_CoalaRequestResult& Branches);

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
	static UCoalaArea* LoadCoalaAreaFromResponse(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo = 0);

	//--------------------------------Async block--------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void LoadCoalaAreaFromResponseAsync(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo);

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static bool AsyncLoadingDone();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static UCoalaArea* GetAsyncJsonResult();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void ResetJsonThread();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static bool AsyncCellMeshDone();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static ACoalaMeshActor* GetCellMeshResult();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void ResetCellMeshThread();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static bool AsyncStreetsMeshDone();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static ACoalaMeshActor* GetStreetsMeshResult();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void ResetStreetsMeshThread();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static bool AsyncWaterMeshDone();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static ACoalaMeshActor* GetWaterMeshResult();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void ResetWaterMeshThread();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static bool AsyncBuildingsMeshDone();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static ACoalaMeshActor* GetBuildingsMeshResult();

	UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void ResetBuildingsMeshThread();



	// Break functions
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaArea(class UCoalaArea* InCoalaArea, TArray<class UCoalaCell*>& coalaCells, TArray<class UCoalaPOI*>& coalaPois, class UCoalaTile*& coalaTile, class UCoalaBounds*& coalaBounds, TArray<class UCoalaStreets*>& coalaStreets, TArray<class UCoalaBuilding*>& coalaBuildings, TArray<class UCoalaWater*>& coalaWaters, FString& coalaWeather, class UCoalaWeather*& weather, class ACoalaAreaActor*& sceneObject );
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaTile(UCoalaTile* InCoalaTile, int& tile_x, int& tile_y, int& zoom );
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaBounds( UCoalaBounds* InCoalaBounds, float& left, float& bottom, float& right, float& top );
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaPOI( UCoalaPOI* InCoalaPoi, UCoalaGPSCoordinates*& gpsCoordinates, FString& poiNameInBackend )
	{};
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaGPSCoordinates( UCoalaGPSCoordinates* InCoalaGpsCoordinates, float& lon, float& lat )
	{};

	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaStreet( UCoalaStreet* InCoalaStreet, TArray<UCoalaGPSCoordinates*>& pos )
	{};

	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaBuilding( UCoalaBuilding* InCoalaBuilding, TArray<UCoalaGPSCoordinates*>& buildingArea, TArray<UCoalaHole*>& buildingHoles, UCoalaGPSCoordinates*& center )
	{};

	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaHole( UCoalaHole* InCoalaHole, TArray<UCoalaGPSCoordinates*>& positions)
	{};
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaWater( UCoalaWater* InCoalaWater, TArray<UCoalaGPSCoordinates*>& waterArea, TArray<UCoalaHole*>& waterHoles, UCoalaGPSCoordinates*& center )
	{};
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaCell( UCoalaCell* InCoalaCell, TMap<FString,int>& gameTags, UCoalaGridIndex*& index );

	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaGridIndex( UCoalaGridIndex* InCoalaGridIndex, int& x, int& y );

	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaGameTag( UCoalaGameTag* InCoalaGameTag, int& id, FString& name, FString& debugColor, int& weight )
	{};
	
	UFUNCTION(BlueprintPure, Category="Coala|Utility", meta=(NativeBreakFunc))
	static void BreakCoalaWeather( UCoalaWeather* InCoalaWeather, int& lastUpdate, FString& temperature, FString& humidity, FString& pressure, float& windSpeed, int& windDirection );

	// other helper
	UFUNCTION(BlueprintCallable, Category="Coala|Utility")
	static UCoalaArea* LoadCoalaAreaFromLocalDump( FString assetPathInProject, int defaultBuildingLevel = 1, bool clampToDefaultBuildingLevel = false );

	UFUNCTION(BlueprintPure, Category="Coala|Utility")
	static UCoalaGPSCoordinates* CenterOfMass( TArray<UCoalaGPSCoordinates*> shape );

	UFUNCTION(BlueprintPure, Category="Coala|Utility")
	static float DistanceOfGpsPositions( float lat1, float lng1, float lat2, float lng2 );

	static float ToRadians(float degrees) 
	{
		float radians = degrees * PI / 180;
		return radians;
	}

	UFUNCTION(BlueprintPure, Category="Coala|Utility")
	static FVector PositionCoalaActorInArea( class UCoalaTile* areaTile, class ACoalaMeshActor* coalaMesh );

	UFUNCTION(BlueprintPure, Category="Coala|Utility|Converter")
	static void GpsPositionToWorldPosition( float lon, float lat, FVector& worldPos );

	UFUNCTION(BlueprintPure, Category="Coala|Utility|Converter")
	static void WorldPositionToGpsPosition( float x, float y, float& lon, float& lat );
	
//	UFUNCTION(BlueprintCallable, Category="Coala|Utility")
//	static void DecorateArea( class UCoalaArea* area,  );CoalaDecorator

	// development
	UFUNCTION(BlueprintPure, Category="Coala|Development helper")
	static FString CoalaBuildingShapeToString( class UCoalaBuilding* building );

	UFUNCTION(BlueprintCallable, Category = "Coala|Development helper")
	static class UCoalaArea* MakeCoalaAreaWithBoundsAndTile( float left, float bottom, float right, float top, int tile_x, int tile_y );

	UFUNCTION(BlueprintCallable, Category = "Coala|Development helper")
	static void SetActorDisplayNameInWorldOutliner( class AActor* actor, FString newDisplayName );

	static int _currentGUID;
	UFUNCTION(BlueprintCallable, Category = "Coala|Development helper")
	static int GetGUID()
	{
		_currentGUID += 1;

		int ret = _currentGUID;
		return _currentGUID;
	}
	UFUNCTION(BlueprintPure, Category = "Coala|Utility", meta = (NativeBreakFunc))
		static void BreakSceneObject(ACoalaAreaActor * Area, ACoalaMeshActor *& RefAreaDimensions, ACoalaActor *& RefAllCells, ACoalaActor *& RefAllWaters, ACoalaActor *& RefAllBuildings, ACoalaActor *& RefAllPOIs, ACoalaActor *& RefAllStreets);

	UFUNCTION(BlueprintPure, Category = "Coala|Utility" )
		static void GetAllCoalaMeshActorChildren(ACoalaActor* RefObjekt, TArray<ACoalaMeshActor*>& Values);

		
	// scale
	public:
		// Allows you to change the scale of the coala objects. The value between 1(100%) .. 0.01(1%)
		UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static void SetCoalaScale(float newScale);

		//Scale in Percent (prob. so between 0-1)
		UFUNCTION(BlueprintCallable, Category = "Coala|Utility")
		static float GetCoalaScale();

	private:
		static float _coalaScale;

};
