// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaConverter.h"
#include "Json.h"
//#include "UObjectGlobals.h"
#include "CoalaArea.h"
#include "CenterOfMass.h"
#include "GeoConverter.h"
#include "CoalaAreaController.h"

UCoalaArea*
CoalaConverter::JsonToCoalaArea( FString jsonString, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo)
{
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create( jsonString );
	if( !FJsonSerializer::Deserialize( JsonReader, JsonParsed ) )
	{
		UE_LOG( LogTemp, Error, TEXT( "CoalaConverter::JsonToCoalaArea parse JSON failed" ) );
		return 0;
	}
	
	//UE_LOG( LogTemp, Warning, TEXT( "UUnitTest_ParseCoalaResponse json successfuly parsed" ) );

	UCoalaArea* ret = NewObject<UCoalaArea>();
	
	// mark the object as "not delete by garbage collector"
	{
		ret->AddToRoot();
	}

	// PROPS (ONLY TILE)
	////////
	{
		TSharedPtr<FJsonObject> props = JsonParsed->GetObjectField( "props" );
		ret->props = NewObject<UCoalaProperties>();
		ret->props->weather = props->GetStringField("weather");

		// TILE
		////////
		TSharedPtr<FJsonObject> tile = props->GetObjectField( "tile" );
		ret->props->tile = NewObject<UCoalaTile>();
		ret->props->tile->x = tile->GetIntegerField("x");
		ret->props->tile->y = tile->GetIntegerField("y");
		ret->props->tile->z = tile->GetIntegerField("z");
		
		// BOUNDS
		////////
		TSharedPtr<FJsonObject> bounds = props->GetObjectField( "bounds" );
		ret->props->bounds = NewObject<UCoalaBounds>();
		ret->props->bounds->left = bounds->GetNumberField("left");
		ret->props->bounds->bottom = bounds->GetNumberField("bottom");
		ret->props->bounds->right = bounds->GetNumberField("right");
		ret->props->bounds->top = bounds->GetNumberField("top");

		// GAMETAG_MAP
		//////////////
		TMap<int64,FString> tmp_gametag_map;

		TArray<TSharedPtr<FJsonValue>> gametag_map = props->GetArrayField( "gametag_map" );
		for( auto it = gametag_map.begin(); it != gametag_map.end(); ++it )
		{
			TSharedPtr<FJsonObject> entry = (*it)->AsObject();

			FString gametag_name = entry->GetStringField("name");
			int64 gametag_id =  entry->GetIntegerField("id");
			tmp_gametag_map.Add( gametag_id, gametag_name );
		}
		ret->props->initGametagMap( tmp_gametag_map );
	}

	// WEATHER
	//////////
	{
		const TSharedPtr<FJsonObject>* data;
		if( JsonParsed->TryGetObjectField( "weather", data ) )
		{
			ret->weather = NewObject<UCoalaWeather>();
			ret->weather->lastUpdate = (*data)->GetIntegerField("lastUpdate");
			ret->weather->temperature = (*data)->GetStringField("temperature");
			ret->weather->humidity = (*data)->GetStringField("humidity");
			ret->weather->pressure = (*data)->GetStringField("pressure");
			ret->weather->windSpeed = (*data)->GetNumberField("windSpeed");
			ret->weather->windDirection = (*data)->GetIntegerField("windDirection");
		}
	}

	// CELL
	///////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if( JsonParsed->TryGetArrayField( "gametags", data ) )
		{
			int count_rows = data->Num();
			for( int x = 0; x < count_rows; ++x )
			{
				TSharedPtr<FJsonObject> current_row = (*data)[x]->AsObject();
				TArray<TSharedPtr<FJsonValue>> row = current_row->GetArrayField("row");
				for( int y = 0; y < row.Num(); ++y )
				{
					TSharedPtr<FJsonObject> entry = row[y]->AsObject();
					TSharedPtr<FJsonObject> cell = entry->GetObjectField("cell");

					UCoalaGridIndex* gridIndex = NewObject<UCoalaGridIndex>();
					gridIndex->x = x;
					gridIndex->y = y; // correction tile index because of unreals flipped Y-achse

					UCoalaCell* currentCell = NewObject<UCoalaCell>();
					ret->grid.Add( currentCell );
					currentCell->index = gridIndex;

					TArray<TSharedPtr<FJsonValue>> ids = cell->GetArrayField("id");
					TArray<TSharedPtr<FJsonValue>> weights = cell->GetArrayField("weight");
					for( int i = 0; i < ids.Num(); ++i )
					{
						FString gametag_id = ids[i]->AsString();
						int priority = weights[i]->AsNumber();

						FString* foundGametagName = ret->props->getGametagNameById( FCString::Atoi64(*gametag_id) );
						if( foundGametagName )
						{
//UE_LOG( LogTemp, Warning, TEXT( "%s" ), **foundGametagName );
							currentCell->gameTags.Add( *foundGametagName, priority );
						}
					}
				}
			}
		}
	}

	// POI
	//////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if( JsonParsed->TryGetArrayField( "pois", data ) )
		{
			for( int i = 0; i < data->Num(); ++i )
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();
				UCoalaPOI* currentPoi = NewObject<UCoalaPOI>();
				ret->pois.Add(currentPoi);

				currentPoi->pos = UCoalaGPSCoordinates::CREATE( 
					entry->GetNumberField("lon"),
					entry->GetNumberField("lat")
				);
				currentPoi->label = entry->GetStringField("label");
			}
		}
	}

	// WATER
	////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if( JsonParsed->TryGetArrayField( "water", data ) )
		{
			for( int i = 0; i < data->Num(); ++i )
			{
				//UE_LOG( LogTemp, Warning, TEXT( "Current water shape index: %d" ), i );
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaWater* currentWater = NewObject<UCoalaWater>();
				ret->water.Add(currentWater);

				TSharedPtr<FJsonObject> area = entry->GetObjectField("area");

				const TArray<TSharedPtr<FJsonValue>> lon = area->GetArrayField( "lon" );
				const TArray<TSharedPtr<FJsonValue>> lat = area->GetArrayField( "lat" );
				for( int a = 0; a < lon.Num(); ++a )
				{
					//UE_LOG( LogTemp, Warning, TEXT( "%d) %f | %f" ), a, currentLon, currentLat );

					UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
					coordinates->lon = lon[a]->AsNumber();
					coordinates->lat = lat[a]->AsNumber();
					currentWater->area.Add(coordinates);
				}

				const TArray<TSharedPtr<FJsonValue>> holes = entry->GetArrayField("holes");
				for( int a = 0; a < holes.Num(); ++a )
				{
					TArray<UCoalaGPSCoordinates*> dataCurrentHole;

					TSharedPtr<FJsonObject> JsonCurrentHole = holes[a]->AsObject();

					const TArray<TSharedPtr<FJsonValue>> holeLon = JsonCurrentHole->GetArrayField( "lon" );
					const TArray<TSharedPtr<FJsonValue>> holeLat = JsonCurrentHole->GetArrayField( "lat" );

					for( int b = 0; b < holeLon.Num(); ++b )
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon = holeLon[b]->AsNumber();
						coordinates->lat = holeLat[b]->AsNumber();
						dataCurrentHole.Add(coordinates);
					}
					currentWater->holes.Add(dataCurrentHole);
				}
			}
		}
	}

	// STREETS
	//////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if( JsonParsed->TryGetArrayField( "streets", data ) )
		{
			for( int i = 0; i < data->Num(); ++i )
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaStreets* currentStreets = NewObject<UCoalaStreets>();
				ret->streets.Add(currentStreets);
				currentStreets->typ = entry->GetStringField("typ");

				// data
				TArray<TSharedPtr<FJsonValue>> streetTypeData = entry->GetArrayField("data");
				for( int b = 0; b < streetTypeData.Num(); ++b )
				{
					UCoalaStreet* currentStreet = NewObject<UCoalaStreet>();
					currentStreets->data.Add(currentStreet);

					TSharedPtr<FJsonObject> current = streetTypeData[b]->AsObject();
					const TArray<TSharedPtr<FJsonValue>> lon = current->GetArrayField( "lon" );
					const TArray<TSharedPtr<FJsonValue>> lat = current->GetArrayField( "lat" );
					for( int a = 0; a < lon.Num(); ++a )
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon =  lon[a]->AsNumber();
						coordinates->lat = lat[a]->AsNumber();
						currentStreet->points.Add(coordinates);
					}
				}
			}
		}
	}

	// BUILDINGS
	////////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if( JsonParsed->TryGetArrayField( "buildings", data ) )
		{
			for( int i = 0; i < data->Num(); ++i )
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaBuilding* currentBuilding = NewObject<UCoalaBuilding>();
				ret->buildings.Add(currentBuilding);

				// height
				currentBuilding->height = entry->GetIntegerField("height");
				if( currentBuilding->height == 0 )
					currentBuilding->height = defaultBuildingLevel;
				else if( clampToDefaultBuildingLevel )
					currentBuilding->height = defaultBuildingLevel;

				// is "limitMaxBuildingLevelTo" enabled ?
				if( limitMaxBuildingLevelTo > 0 )
				{
					// limit if above
					if( currentBuilding->height > limitMaxBuildingLevelTo )
						currentBuilding->height = limitMaxBuildingLevelTo;
				}

				// area
				TSharedPtr<FJsonObject> area = entry->GetObjectField("area");

				const TArray<TSharedPtr<FJsonValue>> lon = area->GetArrayField( "lon" );
				const TArray<TSharedPtr<FJsonValue>> lat = area->GetArrayField( "lat" );
				for( int a = 0; a < lon.Num(); ++a )
				{
					UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
					coordinates->lon =  lon[a]->AsNumber();
					coordinates->lat = lat[a]->AsNumber();
					currentBuilding->area.Add(coordinates);
				}

				// holes
				const TArray<TSharedPtr<FJsonValue>> holes = entry->GetArrayField("holes");
				for( int a = 0; a < holes.Num(); ++a )
				{
					TArray<UCoalaGPSCoordinates*> dataCurrentHole;

					TSharedPtr<FJsonObject> JsonCurrentHole = holes[a]->AsObject();

					const TArray<TSharedPtr<FJsonValue>> holeLon = JsonCurrentHole->GetArrayField( "lon" );
					const TArray<TSharedPtr<FJsonValue>> holeLat = JsonCurrentHole->GetArrayField( "lat" );

					for( int b = 0; b < holeLon.Num(); ++b )
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon = holeLon[b]->AsNumber();
						coordinates->lat = holeLat[b]->AsNumber();
						dataCurrentHole.Add(coordinates);
					}
					currentBuilding->holes.Add(dataCurrentHole);
				}
			}
		}
	}

	return ret;
}

UCoalaBounds*
CoalaConverter::calculateBounds( const TArray<UCoalaGPSCoordinates*>& area )
{
	UCoalaBounds* ret = NewObject<UCoalaBounds>();
	ret->left = area[0]->lon;
	ret->right = area[0]->lon;
	ret->top = area[0]->lat;
	ret->bottom = area[0]->lat;

	for( int i = 0; i < area.Num(); ++i )
	{
		UCoalaGPSCoordinates* current = area[i];

		if( current->lon < ret->left ) ret->left = current->lon;
		if( current->lon > ret->right ) ret->right = current->lon;
		if( current->lat < ret->bottom ) ret->bottom = current->lat;
		if( current->lat > ret->top ) ret->top = current->lat;
	}

	return ret;
}

FVector 
CoalaConverter::ToScenePosition( double lon, double lat )
{
	// adjust values if out of world
	if( lat < -86 )
		lat = -86;
	if( lat > 86 )
		lat = 86;

	if( lon < -180 )
		lon = -180;
	if( lon > 180 )
		lon = 180;

	FVector ret = FVector::ZeroVector;

	ret.X = MercatorConverter::lon2x_m(lon);
	ret.Y = MercatorConverter::lat2y_m(lat);

	// flip y achses because of unreal
	ret.Y *= -1;

	//UE_LOG( LogTemp, Warning, TEXT( "( %f | %f ) -> ( %f | %f )" ), lon, lat, ret.X, ret.Y  );

	return ret;
}

void
CoalaConverter::PixelToGps( float x, float y, float& lon, float& lat )
{
	FVector global_gps_offset = UCoalaAreaController::GetGpsOffset();
	x += global_gps_offset.X;
	y += global_gps_offset.Y;
	y *= -1;

	lon = MercatorConverter::x2lon_m(x);
	lat = MercatorConverter::y2lat_m(y);
}
