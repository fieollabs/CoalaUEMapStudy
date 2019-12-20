// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaReadJsonAsync.h"
#include "CoalaConverter.h"
#include "CoalaArea.h"
#include "CoalaBlueprintUtility.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"


//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaReadJsonAsync* FCoalaReadJsonAsync::Runnable = NULL;
//***********************************************************
bool FCoalaReadJsonAsync::isDone = false; 

FCoalaReadJsonAsync::FCoalaReadJsonAsync(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* initializedObject)
{
	Raw = JsonRaw;
	result = initializedObject;
	//result = initializedObject;
	_defaultBuildingLevel = defaultBuildingLevel;
	_clampToDefaultBuildingLevel = clampToDefaultBuildingLevel;
	_limitMaxBuildingLevelTo = limitMaxBuildingLevelTo;
	Thread = FRunnableThread::Create(this, TEXT("FCoalaReadJsonAsync"), 0, TPri_BelowNormal);
	//Link to where data should be stored
	
}

FCoalaReadJsonAsync::~FCoalaReadJsonAsync()
{
	result = NULL;
	delete Thread;
	Thread = NULL;
}

//Init
bool FCoalaReadJsonAsync::Init()
{
	return true;
}
//Run
uint32 FCoalaReadJsonAsync::Run()
{
	FPlatformProcess::Sleep(0.01);

	JsonToCoalaArea(Raw, _defaultBuildingLevel, _clampToDefaultBuildingLevel, _limitMaxBuildingLevelTo, result);
	
	return 0;
}


FCoalaReadJsonAsync* FCoalaReadJsonAsync::JoyInit(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* initializedObject)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaReadJsonAsync(JsonRaw, defaultBuildingLevel, clampToDefaultBuildingLevel, limitMaxBuildingLevelTo, initializedObject);
	}
	return Runnable;
}

void FCoalaReadJsonAsync::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}


UCoalaArea* FCoalaReadJsonAsync::GetResult()
{
	return Runnable->result;
}


void FCoalaReadJsonAsync::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
		
		isDone = false;
	}
}

bool FCoalaReadJsonAsync::IsThreadFinished()
{
	return isDone;
}

void FCoalaReadJsonAsync::JsonToCoalaArea(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* _result)
{

	FCoalaReadJsonAsync::JsonToCoalaArea(*JsonRaw, defaultBuildingLevel, clampToDefaultBuildingLevel, limitMaxBuildingLevelTo, true, _result);
	
	isDone = true;
}

void
FCoalaReadJsonAsync::JsonToCoalaArea(FString jsonString, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, bool async, UCoalaArea* ret)
{
	
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(jsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		UE_LOG(LogTemp, Error, TEXT("CoalaConverter::JsonToCoalaArea parse JSON failed"));
		return;
	}
	// PROPS (ONLY TILE)
	////////
	{

		TSharedPtr<FJsonObject> props = JsonParsed->GetObjectField("props");
		ret->props = NewObject<UCoalaProperties>();
		ret->props->weather = props->GetStringField("weather");

		// TILE
		////////
		TSharedPtr<FJsonObject> tile = props->GetObjectField("tile");
		ret->props->tile = NewObject<UCoalaTile>();
		ret->props->tile->x = tile->GetIntegerField("x");
		ret->props->tile->y = tile->GetIntegerField("y");
		ret->props->tile->z = tile->GetIntegerField("z");
		ret->props->tile->ClearInternalFlags(EInternalObjectFlags::Async);

		// BOUNDS
		////////
		TSharedPtr<FJsonObject> bounds = props->GetObjectField("bounds");
		ret->props->bounds = NewObject<UCoalaBounds>();
		ret->props->bounds->left = bounds->GetNumberField("left");
		ret->props->bounds->bottom = bounds->GetNumberField("bottom");
		ret->props->bounds->right = bounds->GetNumberField("right");
		ret->props->bounds->top = bounds->GetNumberField("top");
		ret->props->bounds->ClearInternalFlags(EInternalObjectFlags::Async);

		// GAMETAG_MAP
		//////////////
		TMap<int64, FString> tmp_gametag_map;

		TArray<TSharedPtr<FJsonValue>> gametag_map = props->GetArrayField("gametag_map");
		for (auto it = gametag_map.begin(); it != gametag_map.end(); ++it)
		{
			TSharedPtr<FJsonObject> entry = (*it)->AsObject();

			FString gametag_name = entry->GetStringField("name");
			int64 gametag_id = entry->GetIntegerField("id");
			tmp_gametag_map.Add(gametag_id, gametag_name);
		}
		ret->props->initGametagMap(tmp_gametag_map);
		ret->props->ClearInternalFlags(EInternalObjectFlags::Async);
	}

	// WEATHER
	//////////
	{
		const TSharedPtr<FJsonObject>* data;
		if (JsonParsed->TryGetObjectField("weather", data))
		{
			ret->weather = NewObject<UCoalaWeather>();
			ret->weather->lastUpdate = (*data)->GetIntegerField("lastUpdate");
			ret->weather->temperature = (*data)->GetStringField("temperature");
			ret->weather->humidity = (*data)->GetStringField("humidity");
			ret->weather->pressure = (*data)->GetStringField("pressure");
			ret->weather->windSpeed = (*data)->GetNumberField("windSpeed");
			ret->weather->windDirection = (*data)->GetIntegerField("windDirection");
			ret->weather->ClearInternalFlags(EInternalObjectFlags::Async);
		}
	}

	// CELL
	///////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if (JsonParsed->TryGetArrayField("gametags", data))
		{
			int count_rows = data->Num();
			for (int x = 0; x < count_rows; ++x)
			{
				TSharedPtr<FJsonObject> current_row = (*data)[x]->AsObject();
				TArray<TSharedPtr<FJsonValue>> row = current_row->GetArrayField("row");
				for (int y = 0; y < row.Num(); ++y)
				{
					TSharedPtr<FJsonObject> entry = row[y]->AsObject();
					TSharedPtr<FJsonObject> cell = entry->GetObjectField("cell");

					UCoalaGridIndex* gridIndex = NewObject<UCoalaGridIndex>();
					gridIndex->x = x;
					gridIndex->y = y; // correction tile index because of unreals flipped Y-achse

					UCoalaCell* currentCell = NewObject<UCoalaCell>();

					gridIndex->ClearInternalFlags(EInternalObjectFlags::Async);
					currentCell->index = gridIndex;
					TArray<TSharedPtr<FJsonValue>> ids = cell->GetArrayField("id");
					TArray<TSharedPtr<FJsonValue>> weights = cell->GetArrayField("weight");
					
					for (int i = 0; i < ids.Num(); ++i)
					{
						FString gametag_id = ids[i]->AsString();
						int priority = weights[i]->AsNumber();

						FString* foundGametagName = ret->props->getGametagNameById(FCString::Atoi64(*gametag_id));
						if (foundGametagName)
						{
							//UE_LOG( LogTemp, Warning, TEXT( "%s" ), **foundGametagName );
							currentCell->gameTags.Add(*foundGametagName, priority);
						}
					}
					currentCell->ClearInternalFlags(EInternalObjectFlags::Async);
					ret->grid.Add(currentCell);
				}
			}
		}
	}

	// POI
	//////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if (JsonParsed->TryGetArrayField("pois", data))
		{
			for (int i = 0; i < data->Num(); ++i)
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();
				UCoalaPOI* currentPoi = NewObject<UCoalaPOI>();
				
				UCoalaGPSCoordinates* coord = UCoalaGPSCoordinates::CREATE(
					entry->GetNumberField("lon"),
					entry->GetNumberField("lat")
				);
				currentPoi->label = entry->GetStringField("label");
				coord->ClearInternalFlags(EInternalObjectFlags::Async);
				currentPoi->pos = coord;
				currentPoi->ClearInternalFlags(EInternalObjectFlags::Async);
				ret->pois.Add(currentPoi);
			}
		}
	}

	// WATER
	////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if (JsonParsed->TryGetArrayField("water", data))
		{
			for (int i = 0; i < data->Num(); ++i)
			{
				//UE_LOG( LogTemp, Warning, TEXT( "Current water shape index: %d" ), i );
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaWater* currentWater = NewObject<UCoalaWater>();

				TSharedPtr<FJsonObject> area = entry->GetObjectField("area");

				const TArray<TSharedPtr<FJsonValue>> lon = area->GetArrayField("lon");
				const TArray<TSharedPtr<FJsonValue>> lat = area->GetArrayField("lat");
				for (int a = 0; a < lon.Num(); ++a)
				{
					//UE_LOG( LogTemp, Warning, TEXT( "%d) %f | %f" ), a, currentLon, currentLat );

					UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
					coordinates->lon = lon[a]->AsNumber();
					coordinates->lat = lat[a]->AsNumber();
					coordinates->ClearInternalFlags(EInternalObjectFlags::Async);
					currentWater->area.Add(coordinates);
					
				}

				const TArray<TSharedPtr<FJsonValue>> holes = entry->GetArrayField("holes");
				for (int a = 0; a < holes.Num(); ++a)
				{
					TArray<UCoalaGPSCoordinates*> dataCurrentHole;
					

					TSharedPtr<FJsonObject> JsonCurrentHole = holes[a]->AsObject();

					const TArray<TSharedPtr<FJsonValue>> holeLon = JsonCurrentHole->GetArrayField("lon");
					const TArray<TSharedPtr<FJsonValue>> holeLat = JsonCurrentHole->GetArrayField("lat");

					for (int b = 0; b < holeLon.Num(); ++b)
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon = holeLon[b]->AsNumber();
						coordinates->lat = holeLat[b]->AsNumber();
						coordinates->ClearInternalFlags(EInternalObjectFlags::Async);
						dataCurrentHole.Add(coordinates);
						
					}
					currentWater->holes.Add(dataCurrentHole);
				}
				currentWater->ClearInternalFlags(EInternalObjectFlags::Async);
				ret->water.Add(currentWater);
			}
		}
	}

	// STREETS
	//////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if (JsonParsed->TryGetArrayField("streets", data))
		{
			for (int i = 0; i < data->Num(); ++i)
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaStreets* currentStreets = NewObject<UCoalaStreets>();
				ret->streets.Add(currentStreets);
				currentStreets->typ = entry->GetStringField("typ");

				// data
				TArray<TSharedPtr<FJsonValue>> streetTypeData = entry->GetArrayField("data");
				for (int b = 0; b < streetTypeData.Num(); ++b)
				{
					UCoalaStreet* currentStreet = NewObject<UCoalaStreet>();
					
					TSharedPtr<FJsonObject> current = streetTypeData[b]->AsObject();
					const TArray<TSharedPtr<FJsonValue>> lon = current->GetArrayField("lon");
					const TArray<TSharedPtr<FJsonValue>> lat = current->GetArrayField("lat");
					for (int a = 0; a < lon.Num(); ++a)
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon = lon[a]->AsNumber();
						coordinates->lat = lat[a]->AsNumber();
						coordinates->ClearInternalFlags(EInternalObjectFlags::Async);
						currentStreet->points.Add(coordinates);
						
					}
					currentStreet->ClearInternalFlags(EInternalObjectFlags::Async);
					currentStreets->data.Add(currentStreet);
				}
				currentStreets->ClearInternalFlags(EInternalObjectFlags::Async);
				
			}
		}
	}

	// BUILDINGS
	////////////
	{
		const TArray<TSharedPtr<FJsonValue>>* data;
		if (JsonParsed->TryGetArrayField("buildings", data))
		{
			for (int i = 0; i < data->Num(); ++i)
			{
				TSharedPtr<FJsonObject> entry = (*data)[i]->AsObject();

				UCoalaBuilding* currentBuilding = NewObject<UCoalaBuilding>();
				// height
				currentBuilding->height = entry->GetIntegerField("height");
				if (currentBuilding->height == 0)
					currentBuilding->height = defaultBuildingLevel;
				else if (clampToDefaultBuildingLevel)
					currentBuilding->height = defaultBuildingLevel;

				// is "limitMaxBuildingLevelTo" enabled ?
				if (limitMaxBuildingLevelTo > 0)
				{
					// limit if above
					if (currentBuilding->height > limitMaxBuildingLevelTo)
						currentBuilding->height = limitMaxBuildingLevelTo;
				}

				// area
				TSharedPtr<FJsonObject> area = entry->GetObjectField("area");

				const TArray<TSharedPtr<FJsonValue>> lon = area->GetArrayField("lon");
				const TArray<TSharedPtr<FJsonValue>> lat = area->GetArrayField("lat");
				for (int a = 0; a < lon.Num(); ++a)
				{
					UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
					coordinates->lon = lon[a]->AsNumber();
					coordinates->lat = lat[a]->AsNumber();
					coordinates->ClearInternalFlags(EInternalObjectFlags::Async);
					currentBuilding->area.Add(coordinates);
					
				}

				// holes
				const TArray<TSharedPtr<FJsonValue>> holes = entry->GetArrayField("holes");
				for (int a = 0; a < holes.Num(); ++a)
				{
					TArray<UCoalaGPSCoordinates*> dataCurrentHole;

					TSharedPtr<FJsonObject> JsonCurrentHole = holes[a]->AsObject();

					const TArray<TSharedPtr<FJsonValue>> holeLon = JsonCurrentHole->GetArrayField("lon");
					const TArray<TSharedPtr<FJsonValue>> holeLat = JsonCurrentHole->GetArrayField("lat");

					for (int b = 0; b < holeLon.Num(); ++b)
					{
						UCoalaGPSCoordinates* coordinates = NewObject<UCoalaGPSCoordinates>();
						coordinates->lon = holeLon[b]->AsNumber();
						coordinates->lat = holeLat[b]->AsNumber();
						coordinates->ClearInternalFlags(EInternalObjectFlags::Async);
						dataCurrentHole.Add(coordinates);
					}
					currentBuilding->holes.Add(dataCurrentHole);
				}
				currentBuilding->ClearInternalFlags(EInternalObjectFlags::Async);
				ret->buildings.Add(currentBuilding);
			}
		}
	}
}