// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaDecorator.h"
#include "CoalaAreaController.h"
#include "CoalaArea.h"
#include "CoalaAreaActor.h"
#include "CoalaBlueprintUtility.h"
#include "CoalaMeshActor.h"
#include "DrawDebugHelpers.h"
#include "CoalaActor.h"
#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"

void
UCoalaDecorator::DecorateArea( UCoalaArea* area, TArray<FCoalaAreaDecorationConfiguration> configs )
{
	// this node can only do his work if there is a object in the scene to get the world for spawning decoration stuff in it
	if( !area || !area->sceneObject )
		return;

	if( !area->sceneObject->_refAllDecorations )
	{
		FActorSpawnParameters spawnInfo;
		UWorld* world = area->sceneObject->GetWorld();
		area->sceneObject->_refAllDecorations = world->SpawnActor<ACoalaActor>(FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
#if WITH_EDITOR
		FString displayName = area->sceneObject->_refAllDecorations->GetActorLabel() + "_area_" + FString::FromInt(area->props->tile->x) + "_" + FString::FromInt(area->props->tile->y) +"_decorations";
		area->sceneObject->_refAllDecorations->Rename(*displayName);
		area->sceneObject->_refAllDecorations->SetActorLabel(*displayName);
#endif
		area->sceneObject->_refAllDecorations->AttachToActor( area->sceneObject, FAttachmentTransformRules::SnapToTargetIncludingScale );
		area->sceneObject->_refAllDecorations->SetActorRelativeLocation(FVector::ZeroVector);
	}

	for( int i = 0; i < configs.Num(); ++i )
	{
		FCoalaAreaDecorationConfiguration cfg = configs[i];

		if( (int32)cfg.useAt & (int32)OPTIONS_AREA_DECORATION_USE_CONFIG_TO::CELLS )
		{
			ACoalaActor* actor_with_all_attached_decorations = decorateCells( i, area, cfg );
		}
	}
}

ACoalaActor*
UCoalaDecorator::decorateCells( int config_index, UCoalaArea* area, FCoalaAreaDecorationConfiguration config )
{
	UWorld* world = area->sceneObject->GetWorld();
	FActorSpawnParameters spawnInfo;

	TArray<UCoalaCell*> cells_with_gametag;
	// 1 - find all cells with that gametag[s]
	{
		for( int i = 0; i < area->grid.Num(); ++i )
		{
			UCoalaCell* current_cell = area->grid[i];
			for( int a = 0; a < config.gametag.Num(); ++a )
			{
				FString current_gametag = config.gametag[a];

				if( config.onlyIfGametagIsHighest )
				{
					if( current_cell->isGametagHighest( current_gametag ) )
					{
						cells_with_gametag.Add( current_cell );
						break;
					}
				}
				else
				{
					if( current_cell->hastGametag( current_gametag ) )
					{
						cells_with_gametag.Add( current_cell );
						break;
					}
				}
				
			}
		}
	}

	ACoalaActor* ret = 0;
	// 1.5 - spawn parent for attachment place
	if( cells_with_gametag.Num() > 0 )
	{
		ret = world->SpawnActor<ACoalaActor>(FVector::ZeroVector, FRotator::ZeroRotator, spawnInfo );
		area->sceneObject->_refAllDecorations->allAttachedActors.Add(ret);
#if WITH_EDITOR
		FString displayName = ret->GetActorLabel() + "_area_" + FString::FromInt(area->props->tile->x) + "_" + FString::FromInt(area->props->tile->y) + "_decoration_cell_" + FString::FromInt(config_index);
		ret->Rename(*displayName);
		ret->SetActorLabel(*displayName);
#endif
		ret->AttachToActor( area->sceneObject->_refAllDecorations, FAttachmentTransformRules::KeepRelativeTransform );
	}
	
	FVector area_fixpoint = CoalaConverter::ToScenePosition( area->props->bounds->left , area->props->bounds->top );

	// 2 - spawn on center
	{
		for( int i = 0; i < cells_with_gametag.Num(); ++i )
		{
			UCoalaCell* current_cell = cells_with_gametag[i];

			double left = current_cell->bottom_left.X;
			double right = current_cell->bottom_right.X;
			double top = current_cell->top_left.Y;
			double bottom = current_cell->bottom_left.Y;

			FVector pos = FVector::ZeroVector;
			for( int a = 0; a < config.countRetriesIfPositionIsOccupied; ++a )
			{
				if( pos != FVector::ZeroVector )
					break;

				double random_x = FMath::RandRange( 0.0f, (right-left) );
				double random_y = FMath::RandRange( 0.0f, (top-bottom) );

				pos = FVector(
					left + (right-left) - random_x,
					bottom + (top-bottom) - random_y ,
					0
				);


				FHitResult hit;
				// figure out if something is already there
				{
					FVector start = pos;
					start.Z = 100000;
					FVector end = pos;
					end.Z = -100000;

//DrawDebugDirectionalArrow( world, start, end, 10000, FColor::Magenta, true, -1, 0, 100 );

					TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;
					TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
					world->LineTraceSingleByObjectType( hit, start, end, TraceObjectTypes );
				}

				ACoalaMeshActor* is_coala_mesh_actor = Cast<ACoalaMeshActor>(hit.Actor);
				if( !is_coala_mesh_actor )
				{
					pos = FVector::ZeroVector;
					continue;
				}
//UE_LOG( LogTemp, Warning, TEXT( "%s)" ), *is_coala_mesh_actor->GetActorLabel() );

				if( (int32)config.skipIf & (int32)OPTIONS_AREA_DECORATION_IGNORE::BUILDINGS )
					if( is_coala_mesh_actor->ActorHasTag("COALA_BUILDING") )
					{
						pos = FVector::ZeroVector;
						continue;
					}

				if( (int32)config.skipIf & (int32)OPTIONS_AREA_DECORATION_IGNORE::STREETS )
					if( is_coala_mesh_actor->ActorHasTag("COALA_STREET") )
					{
						pos = FVector::ZeroVector;
						continue;
					}

				if( (int32)config.skipIf & (int32)OPTIONS_AREA_DECORATION_IGNORE::WATER )
					if( is_coala_mesh_actor->ActorHasTag("COALA_WATER") )
					{
						pos = FVector::ZeroVector;
						continue;
					}
			}

			if( pos == FVector::ZeroVector )
			{
				// no spawn point found from all retrys
				// going to next cell
				continue;
			}

			FRotator rotation = FRotator::ZeroRotator;
			rotation.SetComponentForAxis( EAxis::Z, FMath::RandRange( 0.0f, 360.0f ) );
			
			pos -= area_fixpoint;
			AStaticMeshActor* inst = world->SpawnActor<AStaticMeshActor>(pos, rotation, spawnInfo );
			inst->SetMobility( EComponentMobility::Movable );
			bool b = inst->GetStaticMeshComponent()->SetStaticMesh( config.decoration );

			FVector scaling(
				FMath::RandRange( config.randomScaleMin.X, config.randomScaleMax.X ) * UCoalaBlueprintUtility::GetCoalaScale(),
				FMath::RandRange( config.randomScaleMin.Y, config.randomScaleMax.Y ) * UCoalaBlueprintUtility::GetCoalaScale(),
				FMath::RandRange( config.randomScaleMin.Z, config.randomScaleMax.Z ) * UCoalaBlueprintUtility::GetCoalaScale()
			);

			inst->SetActorScale3D(scaling);
			inst->AttachToActor( ret, FAttachmentTransformRules::KeepRelativeTransform );
			ret->allAttachedActors.Add(inst);
		}
	}

	return ret;
}
