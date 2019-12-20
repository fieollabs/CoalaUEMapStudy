// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoalaConverter.h"
#include "CenterOfMass.h"
#include "CoalaDevelopmentConfigurations.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaArea.generated.h"

/**
 *
 */

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaGPSCoordinates : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public: 
	UCoalaGPSCoordinates()
	: Super()
	{
		this->lon = 0;
		this->lat = 0;
	}

	UCoalaGPSCoordinates( double lon, double lat )
	{
		this->lon = lon;
		this->lat = lat;
	}

	static UCoalaGPSCoordinates* CREATE( double lon, double lat )
	{
		UCoalaGPSCoordinates* ret = NewObject<UCoalaGPSCoordinates>();
		ret->lon = lon;
		ret->lat = lat;
		return ret;
	}

	UPROPERTY(EditAnywhere, Category="lon") double lon;
	UPROPERTY(EditAnywhere, Category="lat") double lat;
};

UCLASS(BlueprintType)
class THOUGHTFISHCOALAPLUGIN_API UCoalaArea : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UPROPERTY(EditAnywhere, Category="props") UCoalaProperties* props;
		UPROPERTY(EditAnywhere, Category="water") TArray<UCoalaWater*> water;
		UPROPERTY(EditAnywhere, Category="buildings") TArray<UCoalaBuilding*> buildings;
		UPROPERTY(EditAnywhere, Category="pois") TArray<UCoalaPOI*> pois;
		UPROPERTY(EditAnywhere, Category="weather") UCoalaWeather* weather;
		UPROPERTY(EditAnywhere, Category="grid") TArray<UCoalaCell*> grid;
		UPROPERTY(EditAnywhere, Category="streets") TArray<UCoalaStreets*> streets;

		UPROPERTY(EditAnywhere, Category="sceneObject") class ACoalaAreaActor* sceneObject;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaTile : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UPROPERTY(EditAnywhere, Category="x") int x;
		UPROPERTY(EditAnywhere, Category="y") int y;
		UPROPERTY(EditAnywhere, Category="z") int z;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaBounds : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UPROPERTY(EditAnywhere, Category="left") double left;
		UPROPERTY(EditAnywhere, Category="bottom") double bottom;
		UPROPERTY(EditAnywhere, Category="right") double right;
		UPROPERTY(EditAnywhere, Category="top") double top;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaProperties : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UPROPERTY(EditAnywhere, Category="tile") UCoalaTile* tile;
		UPROPERTY(EditAnywhere, Category="bounds") UCoalaBounds* bounds;
		UPROPERTY(EditAnywhere, Category="weather") FString weather;

		FString* getGametagNameById( int64 gametag_id )
		{
			return gametag_map.Find(gametag_id);
		}

		void initGametagMap( TMap<int64, FString> data )
		{
			this->gametag_map = data;
		}

	private:
		// maybe later usefull
		UPROPERTY(EditAnywhere, Category="gametag_map") TMap<int64,FString> gametag_map;
};


UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaWater : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UCoalaWater()
		: Super()
		{
			_bounds = 0;
			_center = 0;
		}

		UPROPERTY(EditAnywhere, Category="area") TArray<UCoalaGPSCoordinates*> area;
		TArray< TArray<UCoalaGPSCoordinates*> > holes;
		
		// localy calculated
		UCoalaGPSCoordinates* center()
		{
			if( !_center )
				_center =  CenterOfMass::Get( area );

			return _center;
		}
		UCoalaBounds* bounds()
		{
			if( !_bounds )
				_bounds = CoalaConverter::calculateBounds( area );

			return _bounds;
		}

	private:
		UPROPERTY(EditAnywhere, Category="center") UCoalaGPSCoordinates* _center;
		UPROPERTY(EditAnywhere, Category="bounds") UCoalaBounds* _bounds;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaBuilding : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	public:
		UCoalaBuilding()
		: Super()
		{
			height = 0;
			_bounds = 0;
			_center = 0;
		}

		int height;
		UPROPERTY(EditAnywhere, Category="area") TArray<UCoalaGPSCoordinates*> area;
		TArray< TArray<UCoalaGPSCoordinates*> > holes;

		// localy calculated
		UCoalaGPSCoordinates* center()
		{
			if( !_center )
				_center =  CenterOfMass::Get( area );

			return _center;
		}
		UCoalaBounds* bounds()
		{
			if( !_bounds )
				_bounds = CoalaConverter::calculateBounds( area );

			return _bounds;
		}

	private:
		UPROPERTY(EditAnywhere, Category="center") UCoalaGPSCoordinates* _center;
		UPROPERTY(EditAnywhere, Category="bounds") UCoalaBounds* _bounds;
};

// Streets
UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaStreets : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, Category="typ") FString typ;
		UPROPERTY(EditAnywhere, Category="data") TArray<UCoalaStreet*> data;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaStreet : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, Category="points") TArray<UCoalaGPSCoordinates*> points;
		
		// localy calculated
		UCoalaGPSCoordinates* center()
		{
			if( !_center )
				_center =  CenterOfMass::Get( points );

			return _center;
		}
		UCoalaBounds* bounds()
		{
			if( !_bounds )
				_bounds = CoalaConverter::calculateBounds( points );

			return _bounds;
		}

	private:
		UPROPERTY(EditAnywhere, Category="center") UCoalaGPSCoordinates* _center;
		UPROPERTY(EditAnywhere, Category="bounds") UCoalaBounds* _bounds;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaPOI : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		
	public:
		UPROPERTY(EditAnywhere, Category="pos") UCoalaGPSCoordinates* pos;
		UPROPERTY(EditAnywhere, Category="label") FString label;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaWeather : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		
	public:
		UPROPERTY(EditAnywhere, Category="lastUpdate") int lastUpdate;
		UPROPERTY(EditAnywhere, Category="temperatur") FString temperature;
		UPROPERTY(EditAnywhere, Category="humidit") FString humidity;
		UPROPERTY(EditAnywhere, Category="pressure") FString pressure;
		UPROPERTY(EditAnywhere, Category="windSpeed") float windSpeed;
		UPROPERTY(EditAnywhere, Category="windDirection") int windDirection;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaGridIndex : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UCoalaGridIndex()
		: Super()
		{
			x = 0;
			y = 0;
		}

		UPROPERTY(EditAnywhere, Category="x") int x;
		UPROPERTY(EditAnywhere, Category="y") int y;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaCell : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, Category="index") class UCoalaGridIndex* index;
		
		// TODO: Refactor to get gametag with hightes priority not only first match !
		UPROPERTY(EditAnywhere, Category="gameTags") TMap<FString,int> gameTags;	

		bool hastGametag( FString gametag );
		bool isGametagHighest( FString gametag );

		// position in unreal units
		FVector top_left;
		FVector top_right;
		FVector bottom_right;
		FVector bottom_left;
};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaHole : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

};

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaGameTag : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

};
