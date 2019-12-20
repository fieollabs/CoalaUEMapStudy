// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaMeshGenerator.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include <vector>
#include "CoalaArea.h"
#include "CoalaAreaActor.h"
#include "CenterOfMass.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "CoalaMeshActor.h"
#include "libs/mapbox/earcut.h"
#include "DrawDebugHelpers.h"
#include "KismetProceduralMeshLibrary.h"
#include "CoalaTaskCreateCells.h"
#include "CoalaTaskGetGroupedShapes.h"
#include "CoalaTaskCreateStreets.h"
#include "CoalaTaskCreateWater.h"
#include "CoalaTaskCreateBuildings.h"
#include "CoalaBlueprintUtility.h"
#include "CoalaActor.h"

namespace mapbox
{
	namespace util
	{
		// template specialization for FVector
		template <>
		struct nth<0, FVector>
		{
			inline static auto get( const FVector &t )
			{
				return t.X;
			};
		};
		template <>
		struct nth<1, FVector>
		{
			inline static auto get( const FVector &t )
			{
				return t.Y;
			};
		};
	}
}

// custom compare function for "Create Cells" logic
namespace std
{
	template<>
	struct less< std::pair<int, UMaterialInterface*> >
	{
		bool operator() ( const std::pair<int, UMaterialInterface*>& lhs, const std::pair<int, UMaterialInterface*>& rhs ) const
		{
			return lhs.first < rhs.first;
		}
	};
}

ACoalaMeshActor*
UCoalaMeshGenerator::CreateAreaDimensions( AActor* spawnActor, UCoalaArea* area, UMaterialInterface* material )
{
	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAreaDimensions )
	{
		ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAreaDimensions = ret;
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_dimensions";
		area->sceneObject->_refAreaDimensions->Rename( *displayName );
		area->sceneObject->_refAreaDimensions->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAreaDimensions->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAreaDimensions->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// mesh data generation
	{
		UCoalaBounds* areaBounds = area->props->bounds;

		TArray<FVector> basicShape;
		{
			FVector area_fixpoint = CoalaConverter::ToScenePosition( areaBounds->left, areaBounds->top );

			basicShape.Add( CoalaConverter::ToScenePosition( areaBounds->left, areaBounds->top ) - area_fixpoint ); // top left
			basicShape.Add( CoalaConverter::ToScenePosition( areaBounds->right, areaBounds->top ) - area_fixpoint ); // top right
			basicShape.Add( CoalaConverter::ToScenePosition( areaBounds->right, areaBounds->bottom ) - area_fixpoint ); // bottom right
			basicShape.Add( CoalaConverter::ToScenePosition( areaBounds->left, areaBounds->bottom ) - area_fixpoint ); // bottom left
/*			// debug output shape points
			for( int i = 0; i < basicShape.Num(); ++i )
			{
				FVector current = basicShape[i];
				UE_LOG( LogTemp, Warning, TEXT( "%s" ), *current.ToString()  );
			}
*/
		}

		TArray<TArray<FVector> > holes;
		{
			// none
		}

		MeshGenerator::generateMesh( ret->mesh, basicShape, holes, 0, 0, true, true, true );

		if( material )
			ret->mesh->SetMaterial( 0, material );
	}

	return ret;
}

ACoalaMeshActor*
UCoalaMeshGenerator::CreateCell( AActor* spawnActor, UCoalaArea* area, UCoalaCell* cell, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig )
{
	TRACE_BOOKMARK( TEXT( "CreateCell" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateCell );

	ACoalaMeshActor* ret = 0;

	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllCells )
	{
		area->sceneObject->_refAllCells = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllCells->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cells";
		area->sceneObject->_refAllCells->Rename( *displayName );
		area->sceneObject->_refAllCells->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllCells->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllCells->SetActorRelativeLocation( FVector::ZeroVector );
	}

	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// calculate corners
	int count_cells_in_one_row = sqrt( area->grid.Num() );
	double cell_width = (area->props->bounds->right - area->props->bounds->left) / count_cells_in_one_row;
	double cell_height = (area->props->bounds->top - area->props->bounds->bottom) / count_cells_in_one_row;

	double cell_left = area->props->bounds->left + (cell->index->x * cell_width);
	double cell_right = area->props->bounds->left + ((cell->index->x + 1) * cell_width);
	double cell_top = area->props->bounds->bottom - ((-1)*cell->index->y * cell_height);
	double cell_bottom = area->props->bounds->bottom - ((-1)*(cell->index->y + 1) * cell_height);

	FVector location = CoalaConverter::ToScenePosition( cell_left, cell_top ) - area_fixpoint;

	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( location, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllCells->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cell_" + FString::FromInt( cell->index->x ) + "_" + FString::FromInt( cell->index->y );
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif	
		ret->Tags.Add( FName( "COALA_CELL" ) );
		ret->AttachToActor( area->sceneObject->_refAllCells, FAttachmentTransformRules::KeepRelativeTransform );
	}

	// mesh data generation
	{
		cell->top_left = CoalaConverter::ToScenePosition( cell_left, cell_top );
		cell->top_right = CoalaConverter::ToScenePosition( cell_right, cell_top );
		cell->bottom_right = CoalaConverter::ToScenePosition( cell_right, cell_bottom );
		cell->bottom_left = CoalaConverter::ToScenePosition( cell_left, cell_bottom );

		TArray<FVector> shape;
		// shape
		{
			shape.Add( cell->bottom_right - cell->top_left );
			shape.Add( cell->bottom_right - cell->top_right );
			shape.Add( cell->bottom_right - cell->bottom_right );
			shape.Add( cell->bottom_right - cell->bottom_left );
		}

		TArray<TArray<FVector> > holes;
		// holes
		{
			// none
		}

		MeshGenerator::generateMesh( ret->mesh, shape, holes, 0, 0, true, true, true );
	}

	// find material and assign
	{
		// TODO: Refactor to get gametag with hightes priority not only first match !
		UMaterialInterface* material = 0;
		{
			for( auto it = cell->gameTags.begin(); it != cell->gameTags.end(); ++it )
			{
				if( material )
					break;

				FString currentGametagName = it->Key;

				for( int i = 0; i < renderConfig.Num(); ++i )
				{
					if( material )
						break;

					FCoalaCellRenderConfig cfg = renderConfig[i];
					for( int a = 0; a < cfg.gametagNames.Num(); ++a )
					{
						if( material )
							break;

						FString possibleGametagName = cfg.gametagNames[a];
						if( possibleGametagName.Compare( currentGametagName ) == 0 )
						{
							//UE_LOG( LogTemp, Warning, TEXT( "FOUND" ) );
													// actor->createMesh( area->props->bounds, cell, cfg.material );
							if( cfg.onlyIfGametagIsHighest )
							{
								if( cell->isGametagHighest( currentGametagName ) )
									material = cfg.material;
							}
							else
							{
								material = cfg.material;
							}
						}
					}
				}
			}
		}

		if( !material )
			material = defaultRenderConfig.material;

		ret->mesh->SetMaterial( 0, material );
	}

	return ret;
}
//this is a test
void
MeshGenerator::ContinueCellCreation( AActor* spawnActor, UCoalaArea* area, TArray<UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig, FVector area_fixpoint, std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, ACoalaMeshActor* ret, FActorSpawnParameters spawnInfo )
{
	{
		ret = spawnActor->GetWorld()->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllCells->allAttachedActors.Add( ret );
		ret->AttachToActor( area->sceneObject->_refAllCells, FAttachmentTransformRules::KeepRelativeTransform );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cells";
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_CELL" ) );

		FCoalaTaskCreateCells::JoyInit( grouped_shapes, ret->mesh, true, true, true, ret );
	}

}

void
UCoalaMeshGenerator::CreateCellsAsync( AActor* spawnActor, UCoalaArea* area, TArray<UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig )
{
	TRACE_BOOKMARK( TEXT( "CreateCellsAsync" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateCellsAsync );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;
	//	int64 startTicks = FDateTime::UtcNow().GetTicks();
	//UE_LOG(LogTemp, Warning, TEXT("diff0: %llu"), startTicks);
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );
	if( !area->sceneObject->_refAllCells )
	{
		area->sceneObject->_refAllCells = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllCells->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cells";
		area->sceneObject->_refAllCells->Rename( *displayName );
		area->sceneObject->_refAllCells->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllCells->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllCells->SetActorRelativeLocation( FVector::ZeroVector );
	}
	//UE_LOG(LogTemp, Warning, TEXT("diff1: %llu"), FDateTime::UtcNow().GetTicks() - startTicks);
	//startTicks = FDateTime::UtcNow().GetTicks();
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// TODO: Check if problems occure because of local variable. If so, make a "NEW" one !
	std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes = new std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >();
	// make shure that there is scene object to attach at

	FCoalaTaskGetGroupedShapes::JoyInit( spawnActor, area, cells, defaultRenderConfig, renderConfig, area_fixpoint, grouped_shapes, ret, spawnInfo );
}

ACoalaMeshActor*
UCoalaMeshGenerator::CreateCells( AActor* spawnActor, UCoalaArea* area, TArray<UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig )
{
	TRACE_BOOKMARK( TEXT( "CreateCells" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateCells );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllCells )
	{
		area->sceneObject->_refAllCells = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllCells->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cells";
		area->sceneObject->_refAllCells->Rename( *displayName );
		area->sceneObject->_refAllCells->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllCells->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllCells->SetActorRelativeLocation( FVector::ZeroVector );
	}

	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// 1) collect cells with same high priority gametag
	std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > > grouped_shapes; // <material index, list of <shapes,holes> >

	for( int i = 0; i < cells.Num(); ++i )
	{
		UCoalaCell* cell = cells[i];

		int material_index = 0;
		UMaterialInterface* material = 0;
		// TODO: Refactor to get gametag with hightes priority not only first match !
		{
			for( auto it = cell->gameTags.begin(); it != cell->gameTags.end(); ++it )
			{
				if( material )
					break;

				FString currentGametagName = it->Key;

				for( int b = 0; b < renderConfig.Num(); ++b )
				{
					if( material )
						break;

					FCoalaCellRenderConfig cfg = renderConfig[b];
					for( int a = 0; a < cfg.gametagNames.Num(); ++a )
					{
						if( material )
							break;

						FString possibleGametagName = cfg.gametagNames[a];
						if( possibleGametagName.Compare( currentGametagName ) == 0 )
						{
							//UE_LOG( LogTemp, Warning, TEXT( "FOUND" ) );
													// actor->createMesh( area->props->bounds, cell, cfg.material );
							material = cfg.material;
							material_index = b + 1;
						}
					}
				}
			}
			if( !material )
				material = defaultRenderConfig.material;

			std::pair<int, UMaterialInterface*> group_index( material_index, material );

			// mesh data generation
			// calculate corners
			int count_cells_in_one_row = sqrt( area->grid.Num() );
			double cell_width = (area->props->bounds->right - area->props->bounds->left) / count_cells_in_one_row;
			double cell_height = (area->props->bounds->top - area->props->bounds->bottom) / count_cells_in_one_row;
			{
				double cell_left = area->props->bounds->left + (cell->index->x * cell_width);
				double cell_right = area->props->bounds->left + ((cell->index->x + 1) * cell_width);
				double cell_top = area->props->bounds->bottom - ((-1)*cell->index->y * cell_height);
				double cell_bottom = area->props->bounds->bottom - ((-1)*(cell->index->y + 1) * cell_height);

				FVector location = CoalaConverter::ToScenePosition( cell_left, cell_top ) - area_fixpoint;

				cell->top_left = CoalaConverter::ToScenePosition( cell_left, cell_top );
				cell->top_right = CoalaConverter::ToScenePosition( cell_right, cell_top );
				cell->bottom_right = CoalaConverter::ToScenePosition( cell_right, cell_bottom );
				cell->bottom_left = CoalaConverter::ToScenePosition( cell_left, cell_bottom );

				TArray<FVector> shape;
				// shape
				{
					shape.Add( cell->top_left - area_fixpoint );
					shape.Add( cell->top_right - area_fixpoint );
					shape.Add( cell->bottom_right - area_fixpoint );
					shape.Add( cell->bottom_left - area_fixpoint );
				}

				TArray<TArray<FVector> > holes;
				// holes
				{
					// none
				}

				// check if entry exsist
				// std::map< std::pair<int,UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > > grouped_shapes; // <material index, list of shapes>
				auto it_find = grouped_shapes.find( group_index );
				if( it_find == grouped_shapes.end() )
				{
					// new entry
					grouped_shapes[group_index] = std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> >( TArray<TArray<FVector>>(), TArray<TArray<TArray<FVector>>>() );
				}
				// add to collection
				grouped_shapes[group_index].first.Add( shape );
				grouped_shapes[group_index].second.Add( holes );
			}

		}
	}

	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllCells->allAttachedActors.Add( ret );
		ret->AttachToActor( area->sceneObject->_refAllCells, FAttachmentTransformRules::KeepRelativeTransform );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_cells";
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_CELL" ) );
	}

	TArray<int> height = {0};

	for( auto it = grouped_shapes.begin(); it != grouped_shapes.end(); ++it )
	{
		std::pair<int, UMaterialInterface*> current_index = it->first;
		std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > entry = it->second;
		TArray<TArray<FVector>> shapes = entry.first;
		TArray<TArray<TArray<FVector>>> holes = entry.second;

		int mesh_section_index_to_create = current_index.first;
		UMaterialInterface* material = current_index.second;

		MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, mesh_section_index_to_create, true, true, true );
		ret->mesh->SetMaterial( mesh_section_index_to_create, material );
	}

	return ret;
}

void
UCoalaMeshGenerator::CreateWaterAsync( AActor* spawnActor, UCoalaArea* area, UCoalaWater* water, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial )
{
	TRACE_BOOKMARK( TEXT( "CreateWaterAsync" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateWaterAsync );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllWaters )
	{
		area->sceneObject->_refAllWaters = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllWaters->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_waters";
		area->sceneObject->_refAllWaters->Rename( *displayName );
		area->sceneObject->_refAllWaters->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllWaters->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllWaters->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// the spawn pos of this actor is relativ to the area he belongs to
	// therefor we use the top left point as our area_fixpoint
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );
	FVector center = CoalaConverter::ToScenePosition( water->center()->lon, water->center()->lat );

	FVector position_relative_to_area = center - area_fixpoint;
	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( position_relative_to_area, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllWaters->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_water";
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_WATER" ) );
		ret->AttachToActor( area->sceneObject->_refAllWaters, FAttachmentTransformRules::KeepRelativeTransform );
	}
	FCoalaTaskCreateWater::JoyInit( ret, water, area_fixpoint, ret->mesh, center, material, generateUVs, outlineWidth, outlineMaterial );
}

ACoalaMeshActor*
UCoalaMeshGenerator::CreateWater( AActor* spawnActor, UCoalaArea* area, UCoalaWater* water, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial )
{
	TRACE_BOOKMARK( TEXT( "CreateWater" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateWater );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllWaters )
	{
		area->sceneObject->_refAllWaters = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllWaters->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_waters";
		area->sceneObject->_refAllWaters->Rename( *displayName );
		area->sceneObject->_refAllWaters->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllWaters->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllWaters->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// the spawn pos of this actor is relativ to the area he belongs to
	// therefor we use the top left point as our area_fixpoint
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );
	FVector center = CoalaConverter::ToScenePosition( water->center()->lon, water->center()->lat );

	FVector position_relative_to_area = center - area_fixpoint;
	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( position_relative_to_area, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllWaters->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_water";
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_WATER" ) );
		ret->AttachToActor( area->sceneObject->_refAllWaters, FAttachmentTransformRules::KeepRelativeTransform );
	}

	TArray<FVector> shape;
	TArray<TArray<FVector> > holes;
	{
		for( int i = 0; i < water->area.Num(); ++i )
		{
			UCoalaGPSCoordinates* current = water->area[i];
			shape.Add( CoalaConverter::ToScenePosition( current->lon, current->lat ) - center );
		}

		for( int i = 0; i < water->holes.Num(); ++i )
		{
			TArray<UCoalaGPSCoordinates*> currentHole_org = water->holes[i];
			TArray<FVector> currentHole;
			for( int a = 0; a < currentHole_org.Num(); ++a )
			{
				UCoalaGPSCoordinates* current = currentHole_org[a];
				currentHole.Add( CoalaConverter::ToScenePosition( current->lon, current->lat ) - center );
			}
			holes.Add( currentHole );
		}

		MeshGenerator::generateMesh( ret->mesh, shape, holes, 0, 0, generateUVs, true, false );
		if( material )
			ret->mesh->SetMaterial( 0, material );
	}

	/*	// generate outline mesh
		// (working, but unuseable because of the outline z fighting on area edges)
		{
			TArray<int> zOffset = {0};
			TArray<TArray<FVector> > shape_outline = MeshGenerator::generateOutlineMesh( shape, holes );

			TArray<TArray<TArray<FVector> > > holes_outline;
			// add for each shape_outline "no" holes
			for( int i = 0; i < shape_outline.Num(); ++i )
			{
				TArray<TArray<FVector> > no_holes;
				holes_outline.Add( no_holes);
			}
			MeshGenerator::generateMesh( ret->mesh, shape_outline, holes_outline, zOffset, 1, generateUVs, true, false );
		}
	*/

	// outlines generated truth a second little bigger mesh
	if( outlineWidth != 0.0f )
	{
		float outline_width = outlineWidth * UCoalaBlueprintUtility::GetCoalaScale();

		TArray<FVector> shape_outline_mesh;
		TArray<TArray<FVector> > holes_outline_mesh;

		MeshGenerator::generateOutlineMesh_v2( shape, holes, outline_width, shape_outline_mesh, holes_outline_mesh );

		MeshGenerator::generateMesh( ret->mesh, shape_outline_mesh, holes_outline_mesh, 1, -1, generateUVs, true, false );
		if( outlineMaterial )
			ret->mesh->SetMaterial( 1, outlineMaterial );
	}

	return ret;
}

void
MeshGenerator::generateOutlineMesh_v2( TArray<FVector> shape, TArray<TArray<FVector> > holes, float outline_width, TArray<FVector>& shape_increased, TArray<TArray<FVector> >& holes_increased )
{
	// shape
	{
		shape_increased = MeshGenerator::getShape( shape, outline_width, OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE );
	}

	// holes
	{
		for( int i = 0; i < holes.Num(); ++i )
		{
			TArray<FVector> outline_shape = MeshGenerator::getShape( holes[i], outline_width, OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE );
			holes_increased.Add( outline_shape );
		}
	}
}

TArray<TArray<FVector> >
MeshGenerator::generateOutlineMesh( TArray<FVector> shape, TArray<TArray<FVector> > holes )
{
	float outline_width = 100 * UCoalaBlueprintUtility::GetCoalaScale();

	TArray<TArray<FVector> > ret;

	// shape
	{
		TArray<FVector> outline_shape = MeshGenerator::getShape( shape, outline_width, OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_LEFT );
		ret.Add( outline_shape );
	}

	// holes
	{
		for( int i = 0; i < holes.Num(); ++i )
		{
			TArray<FVector> current_shape = holes[i];
			TArray<FVector> outline_shape = MeshGenerator::getShape( current_shape, outline_width, OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_LEFT );

			ret.Add( outline_shape );
		}
	}

	return ret;
}

// create ONE Actor for ONE building
ACoalaMeshActor*
UCoalaMeshGenerator::CreateBuilding( AActor* spawnActor, UCoalaArea* area, UCoalaBuilding* building, int32 createMeshes, UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, float heightPerLevel, bool generateCollisions )
{
	TRACE_BOOKMARK( TEXT( "CreateBuilding" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateBuilding );

	ACoalaMeshActor* ret = 0;
	FActorSpawnParameters spawnInfo;
	UWorld* world = spawnActor->GetWorld();

	// add scaling to all needed values
	{
		heightPerLevel *= UCoalaBlueprintUtility::GetCoalaScale();
	}

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllBuildings )
	{
		area->sceneObject->_refAllBuildings = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllBuildings->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_buildings";
		area->sceneObject->_refAllBuildings->Rename( *displayName );
		area->sceneObject->_refAllBuildings->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllBuildings->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllBuildings->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// the spawn pos of this actor is relativ to the area he belongs to
	// therefor we use the top left point as our area_fixpoint
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );
	FVector center = CoalaConverter::ToScenePosition( building->center()->lon, building->center()->lat );

	FVector position_relative_to_area = center - area_fixpoint;
	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( position_relative_to_area, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllBuildings->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_building";
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_BUILDING" ) );
		ret->AttachToActor( area->sceneObject->_refAllBuildings, FAttachmentTransformRules::KeepRelativeTransform );
	}

	TArray<FVector> shape;
	TArray<TArray<FVector> > holes;
	// transform basic shape from gps to pixel coodinates
	{
		for( int i = 0; i < building->area.Num(); ++i )
		{
			UCoalaGPSCoordinates* current = building->area[i];
			shape.Add( CoalaConverter::ToScenePosition( current->lon, current->lat ) - area_fixpoint - position_relative_to_area );
		}

		for( int i = 0; i < building->holes.Num(); ++i )
		{
			TArray<UCoalaGPSCoordinates*> currentHole_org = building->holes[i];
			TArray<FVector> currentHole;
			for( int a = 0; a < currentHole_org.Num(); ++a )
			{
				UCoalaGPSCoordinates* current = currentHole_org[a];
				currentHole.Add( CoalaConverter::ToScenePosition( current->lon, current->lat ) - area_fixpoint - position_relative_to_area );
			}
			holes.Add( currentHole );
		}
	}

	// MESH GENERATION
	{
		float buildingHight = heightPerLevel * building->height;

		int meshIndex = 0;

		//1 floor
		if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::FLOOR )
		{
			MeshGenerator::generateMesh( ret->mesh, shape, holes, meshIndex, 0, generateUVs, generateCollisions, false );
			if( materialFloor )
			{
				ret->mesh->SetMaterial( meshIndex, materialFloor );
			}
			++meshIndex;
		}

		// 2 - all walls
		if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::WALLS )
		{
			int countCreatedWalls = MeshGenerator::generateWallMesh( ret->mesh, shape, holes, buildingHight, meshIndex, generateUVs, generateCollisions );
			if( materialWall )
			{
				for( int i = 0; i < countCreatedWalls; ++i )
					ret->mesh->SetMaterial( meshIndex + i, materialWall );
			}
			meshIndex += countCreatedWalls;
		}

		// 3 - roof
		if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::ROOF )
		{
			MeshGenerator::generateMesh( ret->mesh, shape, holes, meshIndex, buildingHight, generateUVs, generateCollisions, false );
			if( materialRoof )
			{
				ret->mesh->SetMaterial( meshIndex, materialRoof );
			}
			++meshIndex;
		}
	}

	return ret;
}
void
UCoalaMeshGenerator::CreateBuildingsAsync( AActor* spawnActor, UCoalaArea* area, TArray<UCoalaBuilding*> buildings, int32 createMeshes, UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, float heightPerLevel, bool generateCollisions )
{
	TRACE_BOOKMARK( TEXT( "CreateBuildingsAsync" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateBuildingsAsync );

	ACoalaMeshActor* ret = 0;
	FActorSpawnParameters spawnInfo;
	UWorld* world = spawnActor->GetWorld();

	// add scaling to all needed values
	{
		heightPerLevel *= UCoalaBlueprintUtility::GetCoalaScale();
	}

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllBuildings )
	{
		area->sceneObject->_refAllBuildings = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllBuildings->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_buildings";
		area->sceneObject->_refAllBuildings->Rename( *displayName );
		area->sceneObject->_refAllBuildings->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllBuildings->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllBuildings->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// the spawn pos of this actor is relativ to the area he belongs to
	// therefor we use the top left point as our area_fixpoint
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// spawn actor, arange in area and name
	{
		ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllBuildings->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_buildings_" + FString::FromInt( UCoalaBlueprintUtility::GetGUID() );
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_BUILDING" ) );
		ret->AttachToActor( area->sceneObject->_refAllBuildings, FAttachmentTransformRules::KeepRelativeTransform );
	}

	FCoalaTaskCreateBuildings::JoyInit( ret, buildings, createMeshes, area_fixpoint, ret->mesh, materialFloor, materialWall, materialRoof, generateUVs, generateCollisions, true, heightPerLevel );

}
// create ONE actor for ALL buildings
ACoalaMeshActor*
UCoalaMeshGenerator::CreateBuildings( AActor* spawnActor, UCoalaArea* area, TArray<UCoalaBuilding*> buildings, int32 createMeshes, UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, float heightPerLevel, bool generateCollisions )
{
	TRACE_BOOKMARK( TEXT( "CreateBuildings" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateBuildings );

	ACoalaMeshActor* ret = 0;
	FActorSpawnParameters spawnInfo;
	UWorld* world = spawnActor->GetWorld();

	// add scaling to all needed values
	{
		heightPerLevel *= UCoalaBlueprintUtility::GetCoalaScale();
	}

	// make shure that there is scene object to attach at
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( checkAndCreateSceneObjectIfNeeded );
		checkAndCreateSceneObjectIfNeeded( area, spawnActor );
	}


	if( !area->sceneObject->_refAllBuildings )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( createSceneObjectRefAllBuildings );
		area->sceneObject->_refAllBuildings = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllBuildings->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_buildings";
		area->sceneObject->_refAllBuildings->Rename( *displayName );
		area->sceneObject->_refAllBuildings->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllBuildings->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllBuildings->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// the spawn pos of this actor is relativ to the area he belongs to
	// therefor we use the top left point as our area_fixpoint
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// spawn actor, arange in area and name
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( createSceneObjectCombinedMeshHolder );
		ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllBuildings->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_buildings_" + FString::FromInt( UCoalaBlueprintUtility::GetGUID() );
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_BUILDING" ) );
		ret->AttachToActor( area->sceneObject->_refAllBuildings, FAttachmentTransformRules::KeepRelativeTransform );
	}

	TArray<TArray<FVector>> shapes;
	TArray<TArray<TArray<FVector>>> holes;
	TArray<int32> height;

	//array with no heights to give to the floor
	TArray<int32> emptyHeights;

	// transform shape from gps to pixel coodinates
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( transformShapeFromGpsToPicelCoordinates );

		for( int i = 0; i < buildings.Num(); ++i )
		{
			UCoalaBuilding* current = buildings[i];

			FVector center = CoalaConverter::ToScenePosition( current->center()->lon, current->center()->lat );
			FVector position_relative_to_area = area_fixpoint - center;

			height.Add( heightPerLevel * current->height );
			emptyHeights.Add( 0 );

			TArray<FVector> shape;
			for( int a = 0; a < current->area.Num(); ++a )
			{
				shape.Add( CoalaConverter::ToScenePosition( current->area[a]->lon, current->area[a]->lat ) - area_fixpoint );
			}
			shapes.Add( shape );

			TArray<TArray<FVector>> buildingHoles;
			for( int b = 0; b < current->holes.Num(); ++b )
			{
				TArray<UCoalaGPSCoordinates*> currentHole_org = current->holes[b];
				TArray<FVector> currentHole;
				for( int c = 0; c < currentHole_org.Num(); ++c )
				{
					currentHole.Add( CoalaConverter::ToScenePosition( currentHole_org[c]->lon, currentHole_org[c]->lat ) - area_fixpoint );
				}
				buildingHoles.Add( currentHole );
			}
			holes.Add( buildingHoles );
		}
	}



	int sectionIndexToCreate = 0;
	// 1 - floors
	if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::FLOOR )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( createMeshesFloor );
		//MeshGenerator::generateSingleMesh(ret->mesh, shapes, holes, emptyHeights, sectionIndexToCreate, generateUVs, false, generateCollisions);
		MeshGenerator::generateMesh( ret->mesh, shapes, holes, emptyHeights, sectionIndexToCreate, generateUVs, generateCollisions, false );
		if( materialFloor )
		{
			ret->mesh->SetMaterial( sectionIndexToCreate, materialFloor );
		}
		++sectionIndexToCreate;
	}

	// 2 - all walls
	if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::WALLS )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( createMeshesWalls );
		MeshGenerator::generateSingleWallMesh( ret->mesh, shapes, holes, height, sectionIndexToCreate, generateUVs, false, generateCollisions );
		if( materialWall )
		{
			ret->mesh->SetMaterial( sectionIndexToCreate, materialWall );
		}
		++sectionIndexToCreate;
	}

	// 3 - roofs
	if( createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::ROOF )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( createMeshesRoof );
		//MeshGenerator::generateSingleMesh(ret->mesh, shapes, holes, height, sectionIndexToCreate, generateUVs, false, generateCollisions);
		MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, sectionIndexToCreate, generateUVs, generateCollisions, false );
		if( materialRoof )
		{
			ret->mesh->SetMaterial( sectionIndexToCreate, materialRoof );
		}
	}

	return ret;
}

void
MeshGenerator::generateMesh( UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, int segmentIndexToCreateMesh, float zOffset, bool generateUVs, bool generateCollisions, bool stretchUVs )
{
	TRACE_CPUPROFILER_EVENT_SCOPE( generateMesh );

	TArray<FVector> vertices;

	if( zOffset != 0 )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( zOffseting );
		for( int i = 0; i < basicShape.Num(); ++i )
		{
			basicShape[i].Z = zOffset;
		}

		for( int i = 0; i < holes.Num(); ++i )
		{
			TArray<FVector> currentHole = holes[i];
			for( int b = 0; b < currentHole.Num(); ++b )
			{
				FVector current = currentHole[b];
				current.Z = zOffset;
				currentHole[b] = current;
			}
			holes[i] = currentHole;
		}
	}

	TArray<int32> triangles;
	/*	Triangles.Add(0);
		Triangles.Add(1);
		Triangles.Add(2);
	*/
	TArray<FVector> normals;
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	//UV0.Add(FVector2D(0, 0));
	//UV0.Add(FVector2D(10, 0));
	//UV0.Add(FVector2D(0, 10));
	if( generateUVs )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( generateUVs );

		double min_x = basicShape[0].X;
		double max_x = basicShape[0].X;

		double min_y = basicShape[0].Y;
		double max_y = basicShape[0].Y;

		for( int i = 0; i < basicShape.Num(); ++i )
		{
			FVector current = basicShape[i];

			if( current.X < min_x ) min_x = current.X;
			if( current.X > max_x ) max_x = current.X;
			if( current.Y < min_y ) min_y = current.Y;
			if( current.Y > max_y ) max_y = current.Y;
		}
		//UE_LOG( LogTemp, Warning, TEXT( "min_x: %f" ), min_x );
		//UE_LOG( LogTemp, Warning, TEXT( "max_x: %f" ), max_x );
		//UE_LOG( LogTemp, Warning, TEXT( "min_y: %f" ), min_y );
		//UE_LOG( LogTemp, Warning, TEXT( "max_y: %f" ), max_y );

		for( int i = 0; i < basicShape.Num(); ++i )
		{
			FVector current = basicShape[i];

			FVector2D uv;
			if( stretchUVs )
			{
				uv = FVector2D(
					((current.X - min_x) / (max_x - min_x)),
					((current.Y - min_y) / (max_y - min_y))
				);
			}
			else
			{
				// using tileset value as tiling value
				uv = FVector2D(
					((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
					((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
				);
			}

			//UE_LOG( LogTemp, Warning, TEXT( "uv: %s" ), *uv.ToString() );
			UV0.Add( uv );
		}

		for( int i = 0; i < holes.Num(); ++i )
		{
			TArray<FVector> currentHole = holes[i];
			for( int a = 0; a < currentHole.Num(); ++a )
			{
				FVector current = currentHole[a];

				FVector2D uv(
					((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
					((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
				);

				UV0.Add( uv );
			}
		}
	}

	TArray<FProcMeshTangent> tangents;
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	{
		std::vector<std::vector<FVector>> polygon;
		{
			TRACE_CPUPROFILER_EVENT_SCOPE( convert to polygon );
			{
				TRACE_CPUPROFILER_EVENT_SCOPE( basicShape );
				// Fill polygon structure with actual data. Any winding order works.
				// The first polyline defines the main polygon.

				std::vector<FVector> vertices_converted;
				for( int i = 0; i < basicShape.Num(); ++i )
				{
					vertices_converted.push_back( basicShape[i] );
				}
				polygon.push_back( vertices_converted );
			}
			{
				TRACE_CPUPROFILER_EVENT_SCOPE( holes );
				// Following polylines define holes.
				// polygon.push_back( {{75, 25}, {75, 75}, {25, 75}, {25, 25}} );
				for( int i = 0; i < holes.Num(); ++i )
				{
					TArray<FVector> currentHole = holes[i];

					std::vector<FVector> vertices_current_hole;
					for( int a = 0; a < currentHole.Num(); ++a )
					{
						vertices_current_hole.push_back( currentHole[a] );
					}
					polygon.push_back( vertices_current_hole );
				}
			}
		}

		{
			TRACE_CPUPROFILER_EVENT_SCOPE( triangulation );

			// Run tessellation
			// Returns array of indices that refer to the vertices of the input polygon.
			// e.g: the index 6 would refer to {25, 75} in this example.
			// Three subsequent indices form a triangle. Output triangles are clockwise.
			std::vector<uint32_t> indices = mapbox::earcut<uint32_t>( polygon );
			//FString debugString;
			//debugString += "indices.size=" + FString::FromInt( indices.size() ) + FString( ": " );
			for( size_t i = 0; i < indices.size(); ++i )
			{
				//UE_LOG( LogTemp, Warning, TEXT( "indices %d: %d" ), i, indices[i] );
				triangles.Insert( indices[i], 0 );
				//Triangles.Add( indices[i] );
//debugString += FString::FromInt( i ) + "=" + FString::FromInt( indices[i] ) + FString( " " );
			}
			//UE_LOG( LogTemp, Warning, TEXT( "%s" ), *debugString );
		}
	}

	{
		TRACE_CPUPROFILER_EVENT_SCOPE( to vertices );
		{
			TRACE_CPUPROFILER_EVENT_SCOPE( shapes );
			for( int i = 0; i < basicShape.Num(); ++i )
				vertices.Add( basicShape[i] );
		}

		{
			TRACE_CPUPROFILER_EVENT_SCOPE( holes );
			for( int i = 0; i < holes.Num(); ++i )
			{
				TArray<FVector> currentHole = holes[i];
				for( int b = 0; b < currentHole.Num(); ++b )
				{
					vertices.Add( currentHole[b] );
				}
			}
		}
	}

	TArray<FLinearColor> vertexColors;
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	{
		// dissabled because of looks bad and need much performance
//		TRACE_CPUPROFILER_EVENT_SCOPE(CalculateTangentsForMesh);
//		UKismetProceduralMeshLibrary::CalculateTangentsForMesh( vertices, triangles, UV0, normals, tangents );
	}

	MeshGenerator::_createMeshOnMainThread( out, segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );
}

void
MeshGenerator::generateWallMeshSegment( UProceduralMeshComponent*& out, int indexMeshSection, FVector p1, FVector p2, float wall_height, bool generateUVs, bool generateCollisions )
{
	FVector topRight( p2.X, p2.Y, wall_height );
	FVector topLeft( p1.X, p1.Y, wall_height );

	TArray<FVector> vertices;
	vertices.Add( topLeft );
	vertices.Add( topRight );
	vertices.Add( p2 );
	vertices.Add( p1 );

	TArray<int32> triangles;
	triangles.Add( 0 );
	triangles.Add( 1 );
	triangles.Add( 2 );

	triangles.Add( 2 );
	triangles.Add( 3 );
	triangles.Add( 0 );

	TArray<FVector> normals;
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	//	UV0.Add(FVector2D(0, 0));
	//	UV0.Add(FVector2D(10, 0));
	//	UV0.Add(FVector2D(0, 10));
	if( generateUVs )
	{
		float wall_width = std::abs( p2.X - p1.X ) + std::abs( p2.Y - p1.Y );

		float min_x = 0;
		float max_x = wall_width / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale());

		float min_y = 0;
		float max_y = wall_height / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale());

		UV0.Add( FVector2D( max_x, min_y ) );
		UV0.Add( FVector2D( min_x, min_y ) );
		UV0.Add( FVector2D( min_x, max_y ) );
		UV0.Add( FVector2D( max_x, max_y ) );

	}

	TArray<FProcMeshTangent> tangents;
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	{
		// dissabled because of looks bad and need much performance
//		TRACE_CPUPROFILER_EVENT_SCOPE(CalculateTangentsForMesh);
//		UKismetProceduralMeshLibrary::CalculateTangentsForMesh( vertices, triangles, UV0, normals, tangents );
	}

	_createMeshOnMainThread( out, indexMeshSection, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );
}

void
MeshGenerator::generateSingleWallMesh( UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool stretchUVs, bool generateCollisions )
{
	/*	TArray<TArray<FVector>> allWallShapes;
		{
			for( int a = 0; a < shapes.Num(); ++a )
			{
				TArray<FVector> current = shapes[a];
				TArray<FVector> currentShapeWallData;

				for (int b = 0; b < current.Num() - 1; ++b)
				{
					addWallShapeTo( current[b], current[b + 1], zOffset[b], currentShapeWallData );
					allWallShapes.Add( currentShapeWallData );
					currentShapeWallData.Empty();
				}

				TArray<TArray<FVector>> currenthole = holes[a];
				for (int c = 0; c < currenthole.Num(); ++c)
				{
					TArray<FVector> temphole = currenthole[c];
					for (int d = 1; d < temphole.Num(); ++d)
					{
						addWallShapeTo( temphole[d - 1], temphole[d], zOffset[a], currentShapeWallData );
						allWallShapes.Add( currentShapeWallData );
						currentShapeWallData.Empty();
					}
				}

			}
		}

		TArray<TArray<TArray<FVector>>> allWallHoles;
		{
			// empty
			for( int a = 0; a < allWallShapes.Num(); ++a )
			{
				TArray<TArray<FVector> > tmpHoles;
				allWallHoles.Add(tmpHoles);
			}
		}

		TArray<int32> wallZOffset;
		{
			// empty
		}

		MeshGenerator::generateMesh( out, allWallShapes, allWallHoles, wallZOffset, segmentIndexToCreateMesh, generateUVs, generateCollisions, false );
	*/

	int indicesOffset = 0;
	TArray<FVector> vertices;
	TArray<int32> triangles;
	TArray<FVector2D> UV0;

	{
		TRACE_CPUPROFILER_EVENT_SCOPE( Triangulation );

		for( int a = 0; a < shapes.Num(); ++a )
		{
			TArray<FVector> current = shapes[a];

			for( int b = 0; b < current.Num() - 1; ++b )
			{
				generateSingleWallMeshSegment( out, current[b], current[b + 1], zOffset[a], indicesOffset, vertices, triangles, UV0, generateUVs );
				indicesOffset += 4;
			}

			TArray<TArray<FVector>> currenthole = holes[a];
			for( int c = 0; c < currenthole.Num(); ++c )
			{
				TArray<FVector> temphole = currenthole[c];
				for( int d = 1; d < temphole.Num(); ++d )
				{
					generateSingleWallMeshSegment( out, temphole[d - 1], temphole[d], zOffset[a], indicesOffset, vertices, triangles, UV0, generateUVs );
					indicesOffset += 4;
				}
			}
		}
	}
	TArray<FVector> normals;
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));

	TArray<FProcMeshTangent> tangents;
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	{
		// dissabled because of looks bad and need much performance
//		TRACE_CPUPROFILER_EVENT_SCOPE(CalculateTangentsForMesh);
//		UKismetProceduralMeshLibrary::CalculateTangentsForMesh( vertices, triangles, UV0, normals, tangents );
	}
	{
		MeshGenerator::_createMeshOnMainThread( out, segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );
	}
}

void
MeshGenerator::generateSingleWallMeshSegment( UProceduralMeshComponent*& out, FVector p1, FVector p2, float wall_height, int indicesOffset, TArray<FVector>& vertices, TArray<int32>& triangles, TArray<FVector2D>& UV0, bool generateUVs )
{
	FVector topRight( p2.X, p2.Y, wall_height );
	FVector topLeft( p1.X, p1.Y, wall_height );
	FVector bottomRight( p2.X, p2.Y, p2.Z );
	FVector bottomLeft( p1.X, p1.Y, p1.Z );

	vertices.Add( topLeft );
	vertices.Add( topRight );
	vertices.Add( bottomRight );
	vertices.Add( bottomLeft );

	triangles.Add( 0 + indicesOffset );
	triangles.Add( 1 + indicesOffset );
	triangles.Add( 2 + indicesOffset );

	triangles.Add( 2 + indicesOffset );
	triangles.Add( 3 + indicesOffset );
	triangles.Add( 0 + indicesOffset );

	if( generateUVs )
	{
		float wall_width = std::abs( (p2.X) - (p1.X) ) + std::abs( (p2.Y) - (p1.Y) );

		float min_x = 0;
		float max_x = wall_width / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale());

		float min_y = 0;
		float max_y = wall_height / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale());

		UV0.Add( FVector2D( max_x, min_y ) );
		UV0.Add( FVector2D( min_x, min_y ) );
		UV0.Add( FVector2D( min_x, max_y ) );
		UV0.Add( FVector2D( max_x, max_y ) );
	}
}

TArray<uint32_t>
MeshGenerator::triangulate( const TArray<FVector>& shapeVertices, const TArray<TArray<FVector>>& holeVertices )
{
	TRACE_CPUPROFILER_EVENT_SCOPE( MeshGenerator::triangulate );
	std::vector<std::vector<FVector>> polygon;

	{
		TRACE_CPUPROFILER_EVENT_SCOPE( convert to polygon );
		// Fill polygon structure with actual data. Any winding order works.
		// The first polyline defines the main polygon.
		std::vector<FVector> vertices_converted;

		TRACE_CPUPROFILER_EVENT_SCOPE( shapeVertices );
		{
			for( int i = 0; i < shapeVertices.Num(); ++i )
				vertices_converted.push_back( shapeVertices[i] );
			polygon.push_back( vertices_converted );
		}

		// Following polylines define holes.
		// polygon.push_back( {{75, 25}, {75, 75}, {25, 75}, {25, 25}} );
		TRACE_CPUPROFILER_EVENT_SCOPE( holeVertices );
		{
			for( int i = 0; i < holeVertices.Num(); ++i )
			{
				TArray<FVector> currentHole = holeVertices[i];

				std::vector<FVector> vertices_current_hole;
				for( int a = 0; a < currentHole.Num(); ++a )
					vertices_current_hole.push_back( currentHole[a] );

				polygon.push_back( vertices_current_hole );
			}
		}
	}

	// Run tessellation
	// Returns array of indices that refer to the vertices of the input polygon.
	// e.g: the index 6 would refer to {25, 75} in this example.
	// Three subsequent indices form a triangle. Output triangles are clockwise.
	std::vector<uint32_t> indices;
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( earcut );
		indices = mapbox::earcut<uint32_t>( polygon );
	}

	TArray<uint32_t> ret;
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( convert back );
		for( size_t i = 0; i < indices.size(); ++i )
			ret.Insert( indices[i], 0 );
	}

	return ret;
}

void
MeshGenerator::generateBuildingMesh( UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, float buildingHight, bool generateUVs, bool generateCollisions )
{
	//1 floor
	generateMesh( out, basicShape, holes, 0, 0, generateUVs, generateCollisions, false );
	// 2 - all walls
	int countCreatedWalls = generateWallMesh( out, basicShape, holes, buildingHight, 1, generateUVs, generateCollisions );
	// 3 - roof
	generateMesh( out, basicShape, holes, countCreatedWalls + 2, buildingHight, generateUVs, generateCollisions, false );
}

int
MeshGenerator::generateWallMesh( UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, float buildingHight, int meshSegmentStartIndex, bool generateUVs, bool generateCollisions )
{
	TArray<FVector> vertices;

	int meshSegmentIndex = 0;
	for( int i = 0; i < basicShape.Num() - 1; ++i )
	{
		//UE_LOG( LogTemp, Warning, TEXT( "%d" ), meshSegmentIndex );
		generateWallMeshSegment( out, meshSegmentStartIndex + meshSegmentIndex, basicShape[i], basicShape[i + 1], buildingHight, generateUVs, generateCollisions );
		++meshSegmentIndex;
	}

	for( int a = 0; a < holes.Num(); ++a )
	{
		TArray<FVector> currentHole = holes[a];
		for( int b = 1; b < currentHole.Num(); ++b )
		{
			FVector current = currentHole[b];
			FVector prev = currentHole[b - 1];

			//UE_LOG( LogTemp, Warning, TEXT( "%d" ), meshSegmentIndex );
			generateWallMeshSegment( out, meshSegmentStartIndex + meshSegmentIndex, prev, current, buildingHight, generateUVs, generateCollisions );
			++meshSegmentIndex;
		}
	}

	return meshSegmentIndex;
}

void
UCoalaMeshGenerator::PlacePoi( AActor*& inst, AActor* spawnActor, UCoalaPOI* poi, UCoalaArea* area, TMap<FString, TSubclassOf<AActor>> poiConfiguration )
{
	FActorSpawnParameters spawnInfo;
	UWorld* world = spawnActor->GetWorld();

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllPOIs )
	{
		area->sceneObject->_refAllPOIs = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllPOIs->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_pois";
		area->sceneObject->_refAllPOIs->Rename( *displayName );
		area->sceneObject->_refAllPOIs->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllPOIs->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllPOIs->SetActorRelativeLocation( FVector::ZeroVector );
	}

	// search for a configuration
	TSubclassOf<AActor>* poiToSpawn = poiConfiguration.Find( poi->label );
	if( !poiToSpawn || !poiToSpawn->Get() )
		return;

	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );
	FVector spawn_pow = CoalaConverter::ToScenePosition( poi->pos->lon, poi->pos->lat );

	// spawn actor, arange in area and name
	{
		inst = world->SpawnActor<AActor>( poiToSpawn->Get(), spawn_pow - area_fixpoint, FRotator::ZeroRotator, spawnInfo );
		// set scale for actor
		{
			FVector scale = FVector::OneVector;
			scale *= UCoalaBlueprintUtility::GetCoalaScale();
			inst->SetActorScale3D( scale );
		}

		area->sceneObject->_refAllPOIs->allAttachedActors.Add( inst );
#if WITH_EDITOR
		FString displayName = inst->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_poi";
		inst->Rename( *displayName );
		inst->SetActorLabel( *displayName );
#endif
		inst->AttachToActor( area->sceneObject->_refAllPOIs, FAttachmentTransformRules::KeepRelativeTransform );
	}
}

void
UCoalaMeshGenerator::checkAndCreateSceneObjectIfNeeded( UCoalaArea* area, AActor* spawnActor )
{
	if( area->sceneObject )
		return;

	FVector area_position_in_scene = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );
	// offset testing and reminder
//	UE_LOG( LogTemp, Warning, TEXT("REMOVE TEST OFSET AFTER WORKING") );
//	area_position_in_scene = FVector(1000,1000,0);
	FActorSpawnParameters spawnInfo;

	//UGameplayStatics::GetAllActorsOfClass( )
	area->sceneObject = spawnActor->GetWorld()->SpawnActor<ACoalaAreaActor>( area_position_in_scene, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
	FString displayName = area->sceneObject->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y );
	area->sceneObject->Rename( *displayName );
	area->sceneObject->SetActorLabel( *displayName );
#endif
}
void
UCoalaMeshGenerator::CreateStreetsAsync( AActor* spawnActor, UCoalaArea* area, UCoalaStreets* streets, FCoalaStreetRenderConfig defaultRenderConfig, TArray<FCoalaStreetRenderConfig> renderConfig )
{
	TRACE_BOOKMARK( TEXT( "CreateStreetsAsync" ) );
	TRACE_CPUPROFILER_EVENT_SCOPE( UCoalaMeshGenerator::CreateStreetsAsync );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllStreets )
	{
		area->sceneObject->_refAllStreets = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllStreets->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_streets";
		area->sceneObject->_refAllStreets->Rename( *displayName );
		area->sceneObject->_refAllStreets->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllStreets->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllStreets->SetActorRelativeLocation( FVector::ZeroVector );
	}


	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// find matching street render configuration
	FCoalaStreetRenderConfig usedConfig;
	{
		for( int i = 0; i < renderConfig.Num(); ++i )
		{
			FCoalaStreetRenderConfig current = renderConfig[i];

			if( current.types.Contains( streets->typ ) )
			{
				// create actor with this config and break
				ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
				//				actor->createMesh( areaBounds, streets, current );
				usedConfig = current;
				break;
			}
		}

		// no actor created by defined config, using default
		if( !ret )
		{
			ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
			usedConfig = defaultRenderConfig;
		}
	}
	{
		area->sceneObject->_refAllStreets->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_streets_" + streets->typ + "_" + FString::FromInt( streets->data.Num() );
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_STREET" ) );
		ret->AttachToActor( area->sceneObject->_refAllStreets, FAttachmentTransformRules::KeepRelativeTransform );
	}

	FCoalaTaskCreateStreets::JoyInit( ret, usedConfig, streets, area_fixpoint, ret->mesh );

}
ACoalaMeshActor*
UCoalaMeshGenerator::CreateStreets( AActor* spawnActor, UCoalaArea* area, UCoalaStreets* streets, FCoalaStreetRenderConfig defaultRenderConfig, TArray<FCoalaStreetRenderConfig> renderConfig )
{
	TRACE_BOOKMARK( TEXT( "UCoalaMeshGenerator::Streets" ) );

	ACoalaMeshActor* ret = 0;
	UWorld* world = spawnActor->GetWorld();
	FActorSpawnParameters spawnInfo;

	// make shure that there is scene object to attach at
	checkAndCreateSceneObjectIfNeeded( area, spawnActor );

	if( !area->sceneObject->_refAllStreets )
	{
		area->sceneObject->_refAllStreets = world->SpawnActor<ACoalaActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllStreets->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_streets";
		area->sceneObject->_refAllStreets->Rename( *displayName );
		area->sceneObject->_refAllStreets->SetActorLabel( *displayName );
#endif
		area->sceneObject->_refAllStreets->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllStreets->SetActorRelativeLocation( FVector::ZeroVector );
	}


	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left, area->props->bounds->top );

	// find matching street render configuration
	FCoalaStreetRenderConfig usedConfig;
	{
		for( int i = 0; i < renderConfig.Num(); ++i )
		{
			FCoalaStreetRenderConfig current = renderConfig[i];

			if( current.types.Contains( streets->typ ) )
			{
				// create actor with this config and break
				ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
				//				actor->createMesh( areaBounds, streets, current );
				usedConfig = current;
				break;
			}
		}

		// no actor created by defined config, using default
		if( !ret )
		{
			ret = world->SpawnActor<ACoalaMeshActor>( FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
			usedConfig = defaultRenderConfig;
		}
	}

	// spawn actor, arange in area and name
	{
		area->sceneObject->_refAllStreets->allAttachedActors.Add( ret );
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt( area->props->tile->x ) + "_" + FString::FromInt( area->props->tile->y ) + "_streets_" + streets->typ + "_" + FString::FromInt( streets->data.Num() );
		ret->Rename( *displayName );
		ret->SetActorLabel( *displayName );
#endif
		ret->Tags.Add( FName( "COALA_STREET" ) );
		ret->AttachToActor( area->sceneObject->_refAllStreets, FAttachmentTransformRules::KeepRelativeTransform );
	}

	// one material for all street meshes that are actual one mesh
	{
		TArray<TArray<FVector>> shapes;
		TArray<TArray<TArray<FVector> > > holes;

		for( int i = 0; i < streets->data.Num(); ++i )
		{
			UCoalaStreet* currentStreet = streets->data[i];

			// convert data to world coordinated
			TArray<FVector> convertedShape;
			for( int a = 0; a < currentStreet->points.Num(); ++a )
			{
				UCoalaGPSCoordinates* p = currentStreet->points[a];

				convertedShape.Add( CoalaConverter::ToScenePosition( p->lon, p->lat ) - area_fixpoint );
			}


			TArray<TArray<FVector> > no_holes;
			{
				// no holes for streets
			}

			TArray<FVector> streetShape = MeshGenerator::getShape( convertedShape, usedConfig.width*UCoalaBlueprintUtility::GetCoalaScale() );

			shapes.Add( streetShape );
			holes.Add( no_holes );
		}

		TArray<int> no_height = {0};

		MeshGenerator::generateMesh( ret->mesh, shapes, holes, no_height, 0, false, true, false );
		if( usedConfig.material )
			ret->mesh->SetMaterial( 0, usedConfig.material );
	}
	return ret;
}

void
MeshGenerator::generateMesh( UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs )
{
	TRACE_CPUPROFILER_EVENT_SCOPE( generateCombinedMeshs );

	TArray<FVector> vertices;
	TArray<int32> triangles;
	int indicesOffset = 0;
	//UE_LOG(LogTemp, Warning, TEXT("amount shapes %d:"), shapes.Num());
	int heightArrayLength = zOffset.Num();
	if( heightArrayLength > 0 )
	{
		{
			TRACE_CPUPROFILER_EVENT_SCOPE( zOffsetting );
			{
				TRACE_CPUPROFILER_EVENT_SCOPE( shapes );
				for( int z = 0; z < shapes.Num(); ++z )
				{
					TArray<FVector> currentShape = shapes[z];
					for( int y = 0; y < currentShape.Num(); ++y )
					{
						FVector current = currentShape[y];
						current.Z = zOffset[z%heightArrayLength];
						currentShape[y] = current;
					}
					shapes[z] = currentShape;
				}
			}
			{
				TRACE_CPUPROFILER_EVENT_SCOPE( holes );
				for( int z = 0; z < holes.Num(); ++z )
				{
					TArray<TArray<FVector>> currentBuilding = holes[z];
					for( int y = 0; y < currentBuilding.Num(); ++y )
					{
						TArray<FVector> currentHole = currentBuilding[y];
						for( int u = 0; u < currentHole.Num(); ++u )
						{
							FVector current = currentHole[u];
							current.Z = zOffset[z%heightArrayLength];
							currentHole[u] = current;
						}
						currentBuilding[y] = currentHole;
					}
					holes[z] = currentBuilding;
				}
			}
		}

		TRACE_CPUPROFILER_EVENT_SCOPE( triangulate all shapes and her holes );
		for( int a = 0; a < shapes.Num(); ++a )
		{
			TArray<FVector> current = shapes[a];

			for( int b = 0; b < current.Num(); ++b )
				vertices.Add( current[b] );

			TArray<TArray<FVector>> currenthole = holes[a];
			for( int b = 0; b < currenthole.Num(); ++b )
			{
				TArray<FVector> temphole = currenthole[b];
				for( int d = 0; d < temphole.Num(); ++d )
					vertices.Add( temphole[d] );
			}

			TArray<uint32> indices = triangulate( current, currenthole );
			for( int c = 0; c < indices.Num(); ++c )
				triangles.Add( indices[c] + indicesOffset );

			indicesOffset = vertices.Num();
		}
	}
	TArray<FVector> normals;
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	//	UV0.Add(FVector2D(0, 0));
	//	UV0.Add(FVector2D(10, 0));
	//	UV0.Add(FVector2D(0, 10));
	if( generateUVs )
	{
		TRACE_CPUPROFILER_EVENT_SCOPE( generateUVs );

		for( int i = 0; i < shapes.Num(); ++i )
		{
			TArray<FVector> shape = shapes[i];
			float min_x = shape[0].X;
			float max_x = shape[0].X;

			float min_y = shape[0].Y;
			float max_y = shape[0].Y;

			for( int a = 0; a < shape.Num(); ++a )
			{
				FVector current = shape[a];

				if( current.X < min_x ) min_x = current.X;
				if( current.X > max_x ) max_x = current.X;
				if( current.Y < min_y ) min_y = current.Y;
				if( current.Y > max_y ) max_y = current.Y;
			}

			for( int a = 0; a < shape.Num(); ++a )
			{
				FVector current = shape[a];

				FVector2D uv;
				if( stretchUVs )
				{
					uv = FVector2D(
						((current.X - min_x) / (max_x - min_x)),
						((current.Y - min_y) / (max_y - min_y))
					);
				}
				else
				{
					// using tileset value as tiling value
					uv = FVector2D(
						((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
						((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
					);
				}

				UV0.Add( uv );
			}

			TArray<TArray<FVector>> shapeHoles = holes[i];
			for( int a = 0; a < shapeHoles.Num(); ++a )
			{
				TArray<FVector> currentHole = shapeHoles[a];
				for( int b = 0; b < currentHole.Num(); ++b )
				{
					FVector current = currentHole[b];

					FVector2D uv(
						((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
						((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
					);

					UV0.Add( uv );
				}
			}
		}
	}

	TArray<FProcMeshTangent> tangents;
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	{
		// dissabled because of looks bad and need much performance
//		TRACE_CPUPROFILER_EVENT_SCOPE(CalculateTangentsForMesh);
//		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(vertices, triangles, UV0, normals, tangents);
	}
	MeshGenerator::_createMeshOnMainThread( out, segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );
}

TArray<FVector> MeshGenerator::getShape( UCoalaStreet* streetData, float width )
{
	TArray<FVector> converted;

	for( int i = 0; i < streetData->points.Num(); ++i )
	{
		UCoalaGPSCoordinates* coords = streetData->points[i];
		converted.Add(
			FVector(
				coords->lon,
				coords->lat,
				0
			)
		);
	}

	return MeshGenerator::getShape( converted, width, OPTIONS_SHAPE_GENERATION_FROM_LINE::BOTH_SIDES );
}

TArray<FVector> MeshGenerator::getShape( TArray<FVector>& streetData, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option )
{
	TArray<FVector> calculatedShape;

	if( streetData.Num() == 0 )
		return calculatedShape;

	bool isClosed = false;
	if( streetData[0] == streetData[streetData.Num() - 1] )
		isClosed = true;

	/*	TArray<int> toRemoveElements;
		for( int t = 1; t < streetData.Num() - 1; ++t)
		{

			float dist = sqrt(pow(streetData[t + 1].X - streetData[t].X, 2) + pow(streetData[t + 1].Y - streetData[t].Y, 2));
			float distx = sqrt(pow(streetData[t + 1].X - streetData[t].X, 2));
			float disty = sqrt(pow(streetData[t + 1].Y - streetData[t].Y, 2));
			if(
				(distx < 50*UCoalaBlueprintUtility::GetCoalaScale() && disty < 300*UCoalaBlueprintUtility::GetCoalaScale())
				|| (disty < 50*UCoalaBlueprintUtility::GetCoalaScale() && distx < 300*UCoalaBlueprintUtility::GetCoalaScale()))
			{
				toRemoveElements.Push(t);

			}
		}
		if (toRemoveElements.Num() > 0)
		{
			for (int t = toRemoveElements.Num(); t >= 1; --t)
			{
				streetData.RemoveAt(toRemoveElements[t-1]);
			}
		}
	*/
	for( int i = 0; i < streetData.Num(); ++i )
	{
		// to debug a specific point, uncommen below
//			UE_LOG( LogTemp, Warning, TEXT( "%d)" ), i);
/*			if( i == 90 )
			this->Test_CalculateAndDrawStreetCenterSegment( streetData[i], streetData[i+1], streetData[i+2], true );
		else
*/

		if( i == 0 )
		{
			// first
			calculateAndDrawStreetBeginSegment( streetData[0], streetData[1], calculatedShape, width, generation_option );
		}
		else if( i == streetData.Num() - 1 )
		{
			int lastIndex = streetData.Num() - 1;
			if( !isClosed || generation_option == OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE )
			{
				// last
				calculateAndDrawStreetEndSegment( streetData[lastIndex - 1], streetData[lastIndex], calculatedShape, width, generation_option );
			}
			else
			{
				// closed shape means, last element is same as first element
				// so this connection has to be included:
				calculateAndDrawStreetCenterSegment( streetData[i - 1], streetData[0], streetData[1], calculatedShape, width, generation_option );
				FVector help_point_to_connect = calculatedShape[std::floor( calculatedShape.Num() / 2 ) - 1];
				calculatedShape.Insert( help_point_to_connect, 0 );
			}
		}
		else
		{
			// center element
			//ignore too close points

			FVector start = streetData[i - 1];
			FVector center = streetData[i];
			FVector end = streetData[i + 1];

			// skipp possible doublicated nodes
			if( start == center )
				continue;

			calculateAndDrawStreetCenterSegment( start, center, end, calculatedShape, width, generation_option );
			//calculateAndDrawStreetEndSegment(streetData[i], streetData[i+1], calculatedShape, width);
		}
	}
	// close shape
//	FVector firstEntry = calculatedShape[0];
//	calculatedShape.Add( firstEntry );

	/*
	TArray<FVector> calculatedShapemoved = calculatedShape;
	for (int i = 0; i < calculatedShapemoved.Num(); i++)
	{
		calculatedShapemoved[i].X = calculatedShapemoved[i].X - calculatedShapemoved[0].X;
		calculatedShapemoved[i].Y = calculatedShapemoved[i].Y - calculatedShapemoved[0].Y;
	}
	drawLines(calculatedShape, FColor::Red, 40);
	drawLines(streetData, FColor::Green, 40);
	*/
	return calculatedShape;

}

void
MeshGenerator::drawDegreFanAround( FVector start, FVector end, float drawDegre )
{
	FVector direction = end - start;

	drawLine( start, end, FColor::Blue );
	drawLine( start, direction.RotateAngleAxis( 0, FVector::UpVector ) + start, FColor::Green );
	drawLine( start, direction.RotateAngleAxis( 10, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 20, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 30, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 40, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 45, FVector::UpVector ) + start, FColor::Yellow );
	drawLine( start, direction.RotateAngleAxis( 50, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 60, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 70, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 80, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 90, FVector::UpVector ) + start, FColor::Blue );
	drawLine( start, direction.RotateAngleAxis( 100, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 110, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 120, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 130, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 140, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 150, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 160, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 170, FVector::UpVector ) + start, FColor::Red );
	drawLine( start, direction.RotateAngleAxis( 180, FVector::UpVector ) + start, FColor::Red );

	if( drawDegre != 0.0f )
	{
		direction *= 2;
		drawLine( start, direction.RotateAngleAxis( drawDegre, FVector::UpVector ) + start, FColor::White );
	}
}

void
MeshGenerator::drawLine( FVector p1, FVector p2, FColor color, float thickness )
{
	//DrawDebugLine( GetWorld(), p1, p2, color, true, -1, 0, 1 );
	DrawDebugDirectionalArrow( GWorld, p1, p2, 100, color, true, -1, 0, thickness );
}

void
MeshGenerator::drawLines( TArray<FVector>& streetData, FColor color, float thickness )
{
	for( int i = 0; i < streetData.Num() - 1; ++i )
	{
		drawLine( streetData[i], streetData[i + 1], color, thickness );
	}
}

float
MeshGenerator::getAngle( FVector p1, FVector p2, FVector p3, bool drawTwoDirectionVectors )
{
	float dx21 = p1.X - p2.X;
	float dy21 = p1.Y - p2.Y;

	float dx31 = p3.X - p2.X;
	float dy31 = p3.Y - p2.Y;

	float m12 = sqrt( dx21*dx21 + dy21 * dy21 );
	float m13 = sqrt( dx31*dx31 + dy31 * dy31 );
	float nen = dx21 * dx31 + dy21 * dy31;
	float valueforAcos = nen / (m12 * m13);
	float theta = 1.0;
	if( valueforAcos < -1.0 )
	{
		theta = acos( -1.0 );
	}
	else if( valueforAcos > 1.0 )
	{
		theta = acos( 1.0 );
	}
	else
	{
		theta = acos( valueforAcos );
	}

	double deg = theta * 180.0 / PI;

	// negate "deg" if p2 is left of line "p1 to p3"
	float d = (p2.X - p1.X)*(p3.Y - p1.Y) - (p2.Y - p1.Y)*(p3.X - p1.X);
	//UE_LOG( LogTemp, Warning, TEXT( "d: %f" ), d);
	if( d > 0 )
	{
		if( drawTwoDirectionVectors )
		{
			float org = deg + 90;
			UE_LOG( LogTemp, Warning, TEXT( "change deg from %f to %f" ), org, deg );
		}
	}

	// debug: angle between this two lines
	if( drawTwoDirectionVectors )
	{
		//UE_LOG( LogTemp, Warning, TEXT( "theta: %f deg: %f" ), theta, deg );

		drawLine( FVector::ZeroVector, FVector( dx21, dy21, 0 ), FColor::Green );
		drawLine( FVector::ZeroVector, FVector( dx31, dy31, 0 ), FColor::Yellow );

		drawLine( FVector::ZeroVector, FVector( dx21, dy21, 0 ).RotateAngleAxis( deg*0.5f, FVector::UpVector ), FColor::White );
		drawLine( FVector::ZeroVector, FVector( dx21, dy21, 0 ).RotateAngleAxis( deg, FVector::UpVector ) * 2, FColor::White );
	}
	int res = -1;
	if( d < 0 )
	{
		res = 1;
	}
	return res * deg;
}

void
MeshGenerator::calculateAndDrawStreetBeginSegment( FVector start, FVector end, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors )
{
	if( drawMiddleVectors )
	{
		//		DrawDebugBox( GetWorld(), start, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		//		DrawDebugBox( GetWorld(), end, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, start, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, end, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );

		drawLine( start, end, FColor::Magenta );
	}

	FVector direction = end - start;
	float mul = 1;

	float angle_up = mul * (+90);
	float angle_down = mul * (-90);
	//UE_LOG(LogTemp, Warning, TEXT("begin direction x : %f , direction y: %f"), direction.X, direction.Y);
	//UE_LOG(LogTemp, Warning, TEXT("begin angle_up : %f , angle_down : %f"), angle_up, angle_down);
		// draw debug angle fan to see wich direction we are rotating
	if( drawMiddleVectors ) MeshGenerator::drawDegreFanAround( start, end, angle_up );

	FVector point_rotated_up = direction.RotateAngleAxis( angle_up, FVector::UpVector );
	FVector point_rotated_down = direction.RotateAngleAxis( angle_down, FVector::UpVector );
	point_rotated_up.Normalize();
	point_rotated_down.Normalize();
	point_rotated_up = point_rotated_up * width + start;
	point_rotated_down = point_rotated_down * width + start;

	if( drawMiddleVectors )
	{
		drawLine( end, point_rotated_up, FColor::Emerald );
		drawLine( end, point_rotated_down, FColor::Emerald );
	}

	switch( generation_option )
	{
		case OPTIONS_SHAPE_GENERATION_FROM_LINE::BOTH_SIDES:
		addTo.Add( point_rotated_up );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_LEFT:
		addTo.Add( start );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE:
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_RIGHT:
		addTo.Add( point_rotated_up );
		addTo.Insert( start, 0 );
		break;
	}
}

void
MeshGenerator::calculateAndDrawStreetEndSegment( FVector start, FVector end, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors )
{
	if( drawMiddleVectors )
	{
		//		DrawDebugBox( GetWorld(), start, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		//		DrawDebugBox( GetWorld(), end, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, start, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, end, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );

		drawLine( start, end, FColor::Magenta );
	}

	FVector direction = end - start;
	float mul = 1;

	float angle_up = mul * (+90);
	float angle_down = mul * (-90);
	//UE_LOG(LogTemp, Warning, TEXT("end direction x : %f , direction y: %f"), direction.X, direction.Y);
	//UE_LOG(LogTemp, Warning, TEXT("end angle_up : %f , angle_down : %f"), angle_up, angle_down);
		// draw debug angle fan to see wich direction we are rotating
	if( drawMiddleVectors ) MeshGenerator::drawDegreFanAround( start, end, angle_up );

	FVector point_rotated_up = direction.RotateAngleAxis( angle_up, FVector::UpVector );
	FVector point_rotated_down = direction.RotateAngleAxis( angle_down, FVector::UpVector );
	point_rotated_up.Normalize();
	point_rotated_down.Normalize();
	point_rotated_up = point_rotated_up * width + end;
	point_rotated_down = point_rotated_down * width + end;

	if( drawMiddleVectors )
	{
		drawLine( end, point_rotated_up, FColor::Emerald );
		drawLine( end, point_rotated_down, FColor::Emerald );
	}

	switch( generation_option )
	{
		case OPTIONS_SHAPE_GENERATION_FROM_LINE::BOTH_SIDES:
		addTo.Add( point_rotated_up );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_LEFT:
		addTo.Add( end );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE:
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_RIGHT:
		addTo.Add( point_rotated_up );
		addTo.Insert( end, 0 );
		break;
	}
}

void
MeshGenerator::calculateAndDrawStreetCenterSegment( FVector p1, FVector p2, FVector p3, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors )
{
	if( drawMiddleVectors )
	{
		//		DrawDebugBox( GetWorld(), p1, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		//		DrawDebugBox( GetWorld(), p2, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		//		DrawDebugBox( GetWorld(), p3, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, p1, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, p2, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );
		DrawDebugBox( GWorld, p3, FVector::OneVector, FColor::Blue, true, 0, 100, 5 );

		drawLine( p1, p2, FColor::Magenta );
		drawLine( p2, p3, FColor::Magenta );
	}
	float angle_up = 0;
	float angle_down = 0;
	float angle = MeshGenerator::getAngle( p1, p2, p3, drawMiddleVectors );
	float angle_first = angle * 0.5;

	int anglePrefix = 1;

	if( angle_first > 0 )
	{
		anglePrefix = -1;
	}
	float angle_second = anglePrefix * 180 + angle_first;
	if( drawMiddleVectors ) UE_LOG( LogTemp, Warning, TEXT( "angle: %f angle_up: %f angle_down: %f" ), angle, angle_up, angle_down );

	// draw debug angle fan to see wich direction we are rotating
	if( drawMiddleVectors ) drawDegreFanAround( p2, p1, angle );
	//FVector directionForSides = p1 - p2;
	FVector directionForSides = p2 - p1;
	float mul = 1;

	if( angle < 0 && angle >= -180.01 )
	{
		angle_down = angle_first;
		angle_up = angle_second;
	}
	else if( angle < -180 )
	{
		angle_down = angle_second;
		angle_up = angle_first;
	}
	else if( angle > 180 )
	{
		angle_down = angle_first;
		angle_up = angle_second;
	}
	else
	{
		angle_down = angle_second;
		angle_up = angle_first;
	}
	//UE_LOG(LogTemp, Warning, TEXT("mid direction x : %f , direction y: %f"), directionForSides.X, directionForSides.Y);
	//UE_LOG(LogTemp, Warning, TEXT("mid angle_up : %f , angle_down : %f"), mul*angle_up, mul*angle_down);

	FVector point_rotated_up = directionForSides.RotateAngleAxis( mul*angle_up, FVector::UpVector );
	FVector point_rotated_down = directionForSides.RotateAngleAxis( mul*angle_down, FVector::UpVector );
	point_rotated_up.Normalize();
	point_rotated_down.Normalize();
	point_rotated_up = point_rotated_up * width + p2;
	point_rotated_down = point_rotated_down * width + p2;

	if( drawMiddleVectors )
	{
		drawLine( p2, point_rotated_up, FColor::Emerald );
		drawLine( p2, point_rotated_down, FColor::Emerald );
	}

	switch( generation_option )
	{
		case OPTIONS_SHAPE_GENERATION_FROM_LINE::BOTH_SIDES:
		addTo.Add( point_rotated_up );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_LEFT:
		addTo.Add( p2 );
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::INCREASE_SIZE:
		addTo.Insert( point_rotated_down, 0 );
		break;

		case OPTIONS_SHAPE_GENERATION_FROM_LINE::ONLY_RIGHT:
		addTo.Add( point_rotated_up );
		addTo.Insert( p2, 0 );
		break;
	}
}

void
MeshGenerator::_createMeshOnMainThread( UProceduralMeshComponent*& mesh, const int segmentIndexToCreateMesh, const TArray<FVector>& vertices, const TArray<int32>& triangles, const TArray<FVector>& normals, const TArray<FVector2D>& UV0, const TArray<FLinearColor>& vertexColors, const TArray<FProcMeshTangent>& tangents, bool generateCollisions )
{
	TRACE_CPUPROFILER_EVENT_SCOPE( createMeshOnMainThread );

	uint32 threadId = FPlatformTLS::GetCurrentThreadId();
	FString threadName = FThreadManager::Get().GetThreadName( threadId );

#if WITH_EDITOR
	// none
#else 
	// for testing on android
	generateCollisions = false;
#endif

	if( threadName.IsEmpty() )
	{

		// New in UE 4.17, multi-threaded PhysX cooking.
		mesh->bUseAsyncCooking = false;

		// Enable collision data
		mesh->ContainsPhysicsTriMeshData( false );
		// main thread
		mesh->CreateMeshSection_LinearColor( segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );

	}
	else
	{
		// worker
		AsyncTask( ENamedThreads::GameThread, [=]()
		{
			// New in UE 4.17, multi-threaded PhysX cooking.
			mesh->bUseAsyncCooking = true;
			// Enable collision data
			mesh->ContainsPhysicsTriMeshData( false );

			mesh->CreateMeshSection_LinearColor( segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions );
		} );
	}
}
