// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaAreaController.h"
#include "CoalaConverter.h"
#include "GeoConverter.h"
#include <vector>
#include <cmath> 
#include "GameFramework/Character.h"
#include "CoalaAreaActor.h"
#include "Engine/World.h"
#include "CoalaBlueprintUtility.h"
#include "Engine.h"

// custom compare function for "known_areas" logic


std::map<FCoalaRemoteTileRequest, UCoalaArea*> UCoalaAreaController::known_areas = std::map<FCoalaRemoteTileRequest, UCoalaArea*>();
FVector UCoalaAreaController::gps_offset = FVector::ZeroVector;

void
UCoalaAreaController::OnGpsPositionChanged( uint8 zoom, float lon, float lat, TArray<FCoalaRemoteTileRequest>& newAreasInRange, TArray<FCoalaRemoteTileRequest>& areasOutOfRange, int buffer )
{
#if UE_BUILD_DEVELOPMENT
	UE_LOG( LogTemp, Warning, TEXT( "OnGpsPositionChanged - lon: %f lat: %f" ), lon, lat );
#endif
	if( buffer < 0 ) buffer = 0;
	if( buffer > 2 ) buffer = 2;

	// 0 - check if gps_offset is set
	/////////////////////////////////
	if( gps_offset == FVector::ZeroVector )
	{
		gps_offset = CoalaConverter::ToScenePosition( lon, lat );
	}

	// 1 - calculate possible areas in range
	////////////////////////////////////////
	TArray<FCoalaRemoteTileRequest> possibleNewAreasInRange;
	{
		int current_area_tile_x = UGeoConverter::long2tilex( lon, zoom );
		int current_area_tile_y = UGeoConverter::lat2tiley( lat, zoom );

		for( int x = current_area_tile_x-buffer; x <= current_area_tile_x + buffer; ++x )
		{
			for( int y = current_area_tile_y - buffer; y <= current_area_tile_y + buffer; ++y )
			{
				FCoalaRemoteTileRequest tile;
				tile.tile_x = x;
				tile.tile_y = y;
				tile.zoom = zoom;

				possibleNewAreasInRange.Add( tile );
			}
		}
	}

	// 2 - figure out areas to remove
	/////////////////////////////////
	areasOutOfRange.Empty();
	{
		for( auto it = UCoalaAreaController::known_areas.begin(); it != UCoalaAreaController::known_areas.end(); ++it )
		{
			FCoalaRemoteTileRequest current = it->first;

			bool remove_current = true;
			for( int i = 0; i < possibleNewAreasInRange.Num(); ++i )
			{
				FCoalaRemoteTileRequest to_check = possibleNewAreasInRange[i];
				// the != operator not worked somehow ?!
/*				if( current != to_check )
				{
					areas_to_remove.push_back(current);
					break;
				}
				
				if( current.operator!=(to_check) )
				{
					areas_to_remove.push_back(current);
					break;
				}
*/
				if( current == to_check )
				{
					remove_current = false;
					break;
				}
			}

			if( remove_current )
				areasOutOfRange.Add(current);
		}
	}
	if( areasOutOfRange.Num() != 0 )
	{
#if UE_BUILD_DEVELOPMENT
		UE_LOG( LogTemp, Warning, TEXT( "OnGpsPositionChanged - areas_to_remove: %d" ), areasOutOfRange.Num() );
#endif
		int deletedActors = 0;
		for( int i = 0; i < areasOutOfRange.Num(); ++i )
		{
			FCoalaRemoteTileRequest current = areasOutOfRange[i];
			auto it_find = UCoalaAreaController::known_areas.find(current);
			if( it_find == UCoalaAreaController::known_areas.end() )
				continue;
			
			UCoalaArea* current_area = it_find->second;
			if( !current_area )
				continue;

			//UWorld* world = area_ingame_representation->GetWorld();
			if( current_area->sceneObject )
			{
				// a sceene object is present (mesh holder)
				int deleted_actors_for_this_area = current_area->sceneObject->cleanupHoldedCoalaActors();
				UE_LOG( LogTemp, Warning, TEXT( "OnGpsPositionChanged - deleted Actors for area %d: %d" ), i, deleted_actors_for_this_area );
				deletedActors += deleted_actors_for_this_area;

				current_area->sceneObject->Destroy();
				current_area->sceneObject->ConditionalBeginDestroy();
				current_area->sceneObject = 0;

				// mark the object for garbage collection
				current_area->RemoveFromRoot();
			}
			//world->ForceGarbageCollection(true);

			UCoalaAreaController::known_areas.erase( it_find );
		}
#if UE_BUILD_DEVELOPMENT
		UE_LOG( LogTemp, Warning, TEXT( "OnGpsPositionChanged - total deleted Actors: %d" ), deletedActors );
#endif
	}

	// 3 - figure out unknown areas
	///////////////////////////////
	newAreasInRange.Empty();
	{
		for( int i = 0; i < possibleNewAreasInRange.Num(); ++i )
		{
			FCoalaRemoteTileRequest current = possibleNewAreasInRange[i];

			auto it_find = UCoalaAreaController::known_areas.find(current);
			if( it_find != UCoalaAreaController::known_areas.end() )
			{
				// exsist in map
				continue;
			}

			UCoalaAreaController::known_areas[current] = 0;
			newAreasInRange.Add( current );
		}
	}
#if UE_BUILD_DEVELOPMENT
	if( newAreasInRange.Num() != 0 )
		UE_LOG( LogTemp, Warning, TEXT( "OnGpsPositionChanged - newAreasInRange: %d" ), newAreasInRange.Num() );
#endif
}

void
UCoalaAreaController::AddKnownArea( UCoalaArea* area )
{
	FCoalaRemoteTileRequest tile;
	tile.tile_x = area->props->tile->x;
	tile.tile_y = area->props->tile->y;
	tile.zoom = area->props->tile->z;

	UCoalaAreaController::known_areas[tile] = area;
	
	// update world pos of area scene object if present 
	if( !area->sceneObject )
		return;

	FVector pos = area->sceneObject->GetActorLocation();
	pos -= UCoalaAreaController::gps_offset;

	pos.Z = 0;
	area->sceneObject->TeleportTo( pos, FRotator::ZeroRotator );
}

void 
UCoalaAreaController::MoveCharacter( float lon, float lat, ACharacter* character )
{
	if( !character )
		return;

	FVector original_world_position = character->GetActorLocation();

	FVector updated_world_position = CoalaConverter::ToScenePosition( lon, lat );
	updated_world_position -= UCoalaAreaController::gps_offset;

	updated_world_position.Z = original_world_position.Z;
	character->TeleportTo( updated_world_position, FRotator::ZeroRotator, false, true );
}

void
UCoalaAreaController::InitCoala()
{
	UCoalaAreaController::known_areas.clear();
	UCoalaAreaController::gps_offset = FVector::ZeroVector;
}

void
UCoalaAreaController::CleanupCoala()
{
	// how to call it on exit

	for( auto it = UCoalaAreaController::known_areas.begin(); it != UCoalaAreaController::known_areas.end(); ++it )
	{
		UCoalaArea* area = it->second;
		if( !area )
			continue;

		// mark for garbe collect
		area->RemoveFromRoot();
		//area->BeginDestroy();
		//delete area;

		
	}

	GEngine->ForceGarbageCollection(true);
}

FVector UCoalaAreaController::GetGpsOffset(){ return UCoalaAreaController::gps_offset; }

void
UCoalaAreaController::GetGametagsFromGpsPosition( uint8 zoom, float lon, float lat, TArray<FString>& gametags )
{
	// 1 - find area if present
	///////////////////////////
	int area_tile_x = UGeoConverter::long2tilex( lon, zoom );
	int area_tile_y = UGeoConverter::lat2tiley( lat, zoom );

	UCoalaArea* area = 0;
	for( auto it = UCoalaAreaController::known_areas.begin(); it != UCoalaAreaController::known_areas.end(); ++it )
	{
		FCoalaRemoteTileRequest tile = it->first;

		if( tile.zoom == zoom
			&& tile.tile_x == area_tile_x
			&& tile.tile_y == area_tile_y )
		{
			area = it->second;
		}
	}
	if( !area )
		return;
	// 2 - get cell
	///////////////

	//calculate grid index for this cell
	double geo_width = std::abs(area->props->bounds->right) - std::abs(area->props->bounds->left);
	double geo_height = std::abs(area->props->bounds->top) - std::abs(area->props->bounds->bottom);

	int count_cells_one_row = sqrt(area->grid.Num());

	double one_cell_width = geo_width / count_cells_one_row;
	double one_cell_height = geo_height / count_cells_one_row;

	int index_x = std::floor((lon - area->props->bounds->left) / one_cell_width);
	int index_y = std::floor((lat - area->props->bounds->bottom) / one_cell_height);

	// check index_x & index_y
	UCoalaCell* cell = 0;
	for( int i = 0; i < area->grid.Num(); ++i )
	{
		UCoalaCell* current = area->grid[i];

		if( current->index->x == index_x
			&& current->index->y == index_y )
		{
			cell = current;
			break;
		}
	}

	if( !cell )
		return;

	// 3 - gametags from cell
#if UE_BUILD_DEVELOPMENT
	UE_LOG(LogTemp, Warning, TEXT("Cell: %d|%d"), cell->index->x, cell->index->y );
#endif
	for( auto it = cell->gameTags.begin(); it != cell->gameTags.end(); ++it )
	{
		FString current = it->Key;
		int priority = it->Value;

		gametags.Add( current );		
#if UE_BUILD_DEVELOPMENT
	UE_LOG(LogTemp, Warning, TEXT("%d - %s"), priority, *current);
#endif
	}
}