// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaBlueprintUtility.h"
#include "LoadTextFileFromProject.h"
#include "CoalaRemoteTileRequest.h"
#include "CoalaReadJsonAsync.h"
#include "CoalaTaskCreateCells.h"
#include "CoalaTaskCreateStreets.h"
#include "CoalaTaskCreateWater.h"
#include "CoalaTaskCreateBuildings.h"
#include "CoalaRequests.h"
#include "CenterOfMass.h"
#include "CoalaMeshGenerator.h"
#include <math.h>
#include "GeoConverter.h"
#include "CoalaMeshActor.h"
#include "CoalaAreaActor.h"
#include "Http.h"
#include "Json.h"
#include <vector>
//#include "Async.h"
#include "CoalaAreaController.h"


float UCoalaBlueprintUtility::_coalaScale = 1.0f;
int UCoalaBlueprintUtility::_currentGUID = 0;

void
UCoalaBlueprintUtility::BreakCoalaArea(UCoalaArea* InCoalaArea, TArray<UCoalaCell*>& coalaCells, TArray<UCoalaPOI*>& coalaPois, UCoalaTile*& coalaTile, UCoalaBounds*& coalaBounds, TArray<UCoalaStreets*>& coalaStreets, TArray<UCoalaBuilding*>& coalaBuildings, TArray<UCoalaWater*>& coalaWaters, FString& coalaWeather, UCoalaWeather*& weather,
	ACoalaAreaActor*& sceneObject )
{
	coalaCells = InCoalaArea->grid;
	coalaTile = InCoalaArea->props->tile;
	coalaBounds = InCoalaArea->props->bounds;
	coalaWaters = InCoalaArea->water;
	coalaBuildings = InCoalaArea->buildings;
	coalaPois = InCoalaArea->pois;
	coalaWeather  = InCoalaArea->props->weather;
	weather = InCoalaArea->weather;
	coalaStreets = InCoalaArea->streets;

	// representation of objects/actors in sceene
	if( !InCoalaArea->sceneObject )
	{
		sceneObject = 0;
		return;
	}

	sceneObject = InCoalaArea->sceneObject;
}

void 
UCoalaBlueprintUtility::BreakCoalaTile(UCoalaTile* InCoalaTile, int& tile_x, int& tile_y, int& zoom )
{
	tile_x = InCoalaTile->x;
	tile_y = InCoalaTile->y;
	zoom = InCoalaTile->z;
}

void
UCoalaBlueprintUtility::BreakCoalaBounds( UCoalaBounds* InCoalaBounds, float& left, float& bottom, float& right, float& top )
{
	left = InCoalaBounds->left;
	bottom = InCoalaBounds->bottom;
	right = InCoalaBounds->right;
	top = InCoalaBounds->top;
}

void
UCoalaBlueprintUtility::BreakCoalaGridIndex( UCoalaGridIndex* InCoalaGridIndex, int& x, int& y )
{
	x = InCoalaGridIndex->x;
	y = InCoalaGridIndex->y;
}

void 
UCoalaBlueprintUtility::BreakCoalaCell( UCoalaCell* InCoalaCell, TMap<FString,int>& gameTags, UCoalaGridIndex*& index )
{
	gameTags = InCoalaCell->gameTags;
	index = InCoalaCell->index;
}

UCoalaArea*
UCoalaBlueprintUtility::LoadCoalaAreaFromLocalDump( FString assetPathInProject, int defaultBuildingLevel, bool clampToDefaultBuildingLevel )
{
	FString JsonRaw = ULoadTextFileFromProject::LoadTextFileFromProject( assetPathInProject );
	UCoalaArea* ret = CoalaConverter::JsonToCoalaArea( JsonRaw, defaultBuildingLevel, clampToDefaultBuildingLevel );
	return ret;
}

void
UCoalaBlueprintUtility::LoadCoalaAreaFromResponseAsync(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo)
{
	UCoalaArea* ret = NewObject<UCoalaArea>();

	// mark the object as "not delete by garbage collector"
	{
		ret->AddToRoot();
	}
	FCoalaReadJsonAsync::JoyInit(JsonRaw, defaultBuildingLevel, clampToDefaultBuildingLevel, limitMaxBuildingLevelTo, ret);
}

bool
UCoalaBlueprintUtility::AsyncLoadingDone()
{
	return FCoalaReadJsonAsync::IsThreadFinished();
}

UCoalaArea*
UCoalaBlueprintUtility::GetAsyncJsonResult()
{
	UCoalaArea* result = FCoalaReadJsonAsync::GetResult();
	return result;
}

void
UCoalaBlueprintUtility::ResetJsonThread()
{
	FCoalaReadJsonAsync::Shutdown();
}

bool
UCoalaBlueprintUtility::AsyncCellMeshDone()
{
	return FCoalaTaskCreateCells::IsThreadFinished();
}

ACoalaMeshActor* 
UCoalaBlueprintUtility::GetCellMeshResult()
{
	return FCoalaTaskCreateCells::Runnable->GetResult();
}

void
UCoalaBlueprintUtility::ResetCellMeshThread()
{
	FCoalaTaskCreateCells::Shutdown();
}

bool
UCoalaBlueprintUtility::AsyncStreetsMeshDone()
{
	return FCoalaTaskCreateStreets::IsThreadFinished();
}

ACoalaMeshActor*
UCoalaBlueprintUtility::GetStreetsMeshResult()
{
	return FCoalaTaskCreateStreets::Runnable->GetResult();
}

void
UCoalaBlueprintUtility::ResetStreetsMeshThread()
{
	FCoalaTaskCreateStreets::Shutdown();
}

bool
UCoalaBlueprintUtility::AsyncWaterMeshDone()
{
	return FCoalaTaskCreateWater::IsThreadFinished();
}

ACoalaMeshActor*
UCoalaBlueprintUtility::GetWaterMeshResult()
{
	return FCoalaTaskCreateWater::Runnable->GetResult();
}

void
UCoalaBlueprintUtility::ResetWaterMeshThread()
{
	FCoalaTaskCreateWater::Shutdown();
}
bool
UCoalaBlueprintUtility::AsyncBuildingsMeshDone()
{
	return FCoalaTaskCreateBuildings::IsThreadFinished();
}

ACoalaMeshActor*
UCoalaBlueprintUtility::GetBuildingsMeshResult()
{
	return FCoalaTaskCreateBuildings::Runnable->GetResult();
}

void
UCoalaBlueprintUtility::ResetBuildingsMeshThread()
{
	FCoalaTaskCreateBuildings::Shutdown();
}

UCoalaArea*
UCoalaBlueprintUtility::LoadCoalaAreaFromResponse(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo )
{

	UCoalaArea* ret = CoalaConverter::JsonToCoalaArea(JsonRaw, defaultBuildingLevel, clampToDefaultBuildingLevel, limitMaxBuildingLevelTo );
	return ret;
}
FCoalaRemoteTileRequest
UCoalaBlueprintUtility::CreateRemoteTileRequestFromGpsCoordinates(uint8 zoom, float lon, float lat)
{
	int current_area_tile_x = UGeoConverter::long2tilex(lon, zoom);
	int current_area_tile_y = UGeoConverter::lat2tiley(lat, zoom);

	FCoalaRemoteTileRequest tile;
	tile.tile_x = current_area_tile_x;
	tile.tile_y = current_area_tile_y;
	tile.zoom = zoom;
	return tile;
}

UCoalaGPSCoordinates* 
UCoalaBlueprintUtility::CenterOfMass( TArray<UCoalaGPSCoordinates*> shape )
{
	return CenterOfMass::Get(shape);
}

float 
UCoalaBlueprintUtility::DistanceOfGpsPositions(float lat1, float lng1, float lat2, float lng2) 
{
	float earthRadius = 3958.75; // miles
	float dLat = ToRadians(lat2-lat1);
	float dLng = ToRadians(lng2-lng1);
	float a = sin(dLat/2) * sin(dLat/2) + 
				cos(ToRadians(lat1)) * cos(ToRadians(lat2)) * 
				sin(dLng/2) * sin(dLng/2);
	float c = 2 * atan2(sqrt(a), sqrt(1-a));
	float dist = earthRadius * c;
	float meterConversion = 1609.00; // miles to m
	return dist * meterConversion;
}

FVector
UCoalaBlueprintUtility::PositionCoalaActorInArea( UCoalaTile* areaTile, ACoalaMeshActor* coalaMesh )
{
	double area_offset_lon = UGeoConverter::tilex2long( areaTile->x, areaTile->z );
	double area_offset_lat = UGeoConverter::tiley2lat( areaTile->y, areaTile->z );

	FVector newLocation;




	coalaMesh->SetActorLocation( newLocation );

	return newLocation;
}

// use this function to convert GPS position data (longitude,latitude) to Unreal position data (FVector)
void
UCoalaBlueprintUtility::GpsPositionToWorldPosition( float lon, float lat, FVector& worldPos )
{
	worldPos = CoalaConverter::ToScenePosition( lon, lat );
}

void 
UCoalaBlueprintUtility::WorldPositionToGpsPosition( float x, float y, float& lon, float& lat )
{
	CoalaConverter::PixelToGps( x, y, lon, lat );
}

FString 
UCoalaBlueprintUtility::CoalaBuildingShapeToString( UCoalaBuilding* building )
{
	FString ret;
	
	ret += "area:\r\n";
	for( int i = 0; i < building->area.Num(); ++i )
	{
		UCoalaGPSCoordinates* current = building->area[i];

		ret += FString::SanitizeFloat( current->lon ) + ", " + FString::SanitizeFloat( current->lat ) + "\r\n" ;
	}

	ret += "holes: " + FString::FromInt( building->holes.Num() ) + "\r\n" ;
	ret += "TODO...\r\n";

	return ret;
}

void
UCoalaBlueprintUtility::BreakCoalaWeather( UCoalaWeather* InCoalaWeather, int& lastUpdate, FString& temperature, FString& humidity, FString& pressure, float& windSpeed, int& windDirection )
{
	lastUpdate = InCoalaWeather->lastUpdate;
	temperature = InCoalaWeather->temperature;
	humidity = InCoalaWeather->humidity;
	pressure = InCoalaWeather->pressure;
	windSpeed = InCoalaWeather->windSpeed;
	windDirection = InCoalaWeather->windDirection;
}

UBluePrintHttpGetRequest*
UCoalaBlueprintUtility::MakeCoalaRequest(FString api_key, FCoalaRemoteTileRequest tile, UPARAM(meta = (Bitmask, BitmaskEnum = "REQUEST_CONTEXT")) int32 context, EOutputPins_CoalaRequestResult& Branches)
{
UE_LOG(LogTemp, Warning, TEXT("UCoalaBlueprintUtility::MakeCoalaRequest context: %d"), context);

	std::vector<FString> contents;
	//https://answers.unrealengine.com/questions/489492/c-bitmask-enums-appear-to-be-offset-by-1.html
	if( context & ( 1 << (int32)REQUEST_CONTEXT::GAMETAGS) )
		contents.push_back("gametags");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::POIS) )
		contents.push_back("pois");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::WEATHER) )
		contents.push_back("weather");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::STREETS) )
		contents.push_back("streets");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::BUILDINGS) )
		contents.push_back("buildings");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::WATER) )
		contents.push_back("water");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::CONTEXT) )
		contents.push_back("context");
	if( context & ( 1 << (int32)REQUEST_CONTEXT::TIMEZONE) )
		contents.push_back("timezone");

	UBluePrintHttpGetRequest* target = NewObject<UBluePrintHttpGetRequest>();
	FString request("http://api.coala.thoughtfish.de:25020/thoughtfish/coala?format=JSON_V2");
	request += "&api_key=" + api_key;

	request += "&zoom=" + FString::FromInt((int)tile.zoom);
	request += "&tile_x=" + FString::FromInt(tile.tile_x);
	request += "&tile_y=" + FString::FromInt(tile.tile_y);
	if( contents.size() != 0 )
	{
		request += "&content=";
		for( auto it = contents.begin(); it != contents.end(); ++it )
		{
			FString current = *it;
			request += current;

			if( std::distance(it, contents.end()) > 1 )
				request += ",";
		}
	}

UE_LOG(LogTemp, Warning, TEXT("url: %s"), *request);
	// Create the HTTP request
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	// make shure url starts with "http"
	HttpRequest->SetURL(request);
	HttpRequest->OnProcessRequestComplete().BindUObject(target, &UBluePrintHttpGetRequest::OnResponseReceived);
	//	target->AddToRoot();

	HttpRequest->ProcessRequest();

	return target;
}


UCoalaArea*
UCoalaBlueprintUtility::MakeCoalaAreaWithBoundsAndTile( float left, float bottom, float right, float top, int tile_x, int tile_y )
{
	UCoalaArea* ret = NewObject<UCoalaArea>();

	ret->props = NewObject<UCoalaProperties>();
	ret->props->bounds = NewObject<UCoalaBounds>();
	ret->props->bounds->left = left;
	ret->props->bounds->bottom = bottom;
	ret->props->bounds->right = right;
	ret->props->bounds->top = top;

	ret->props->tile = NewObject<UCoalaTile>();
	ret->props->tile->x = tile_x;
	ret->props->tile->y = tile_y;

	return ret;
}

void
UCoalaBlueprintUtility::SetActorDisplayNameInWorldOutliner( AActor* actor, FString displayName )
{
#if WITH_EDITOR
	actor->Rename(*displayName);
	actor->SetActorLabel(*displayName);
#endif
}

void 
UCoalaBlueprintUtility::SetCoalaScale(float newScale)
{
	if( newScale > 1 ) newScale = 1;
	if( newScale < 0.01 ) newScale = 0.01;
	UCoalaBlueprintUtility::_coalaScale = newScale;
}

float
UCoalaBlueprintUtility::GetCoalaScale()
{
	return UCoalaBlueprintUtility::_coalaScale;
}

void UCoalaBlueprintUtility::BreakSceneObject(ACoalaAreaActor* Area, ACoalaMeshActor*& RefAreaDimensions, ACoalaActor*& RefAllCells, ACoalaActor*& RefAllWaters, ACoalaActor*& RefAllBuildings, ACoalaActor*& RefAllPOIs, ACoalaActor*& RefAllStreets)
{
	RefAreaDimensions = Area->_refAreaDimensions;

	RefAllCells = Area->_refAllCells;
	RefAllWaters = Area->_refAllWaters;
	RefAllBuildings = Area->_refAllBuildings;
	RefAllPOIs = Area->_refAllPOIs;

	RefAllStreets = Area->_refAllStreets;
}
void UCoalaBlueprintUtility::GetAllCoalaMeshActorChildren(ACoalaActor* RefObjekt, TArray<ACoalaMeshActor*>& Values)
{
	TArray <AActor*> actors = RefObjekt->allAttachedActors;
	for (int i = 0; i < actors.Num(); i++)
	{
		ACoalaMeshActor* cast = Cast<ACoalaMeshActor>(actors[i]);
		if (cast)
		{
			Values.Add(cast);
		}
	}
}



