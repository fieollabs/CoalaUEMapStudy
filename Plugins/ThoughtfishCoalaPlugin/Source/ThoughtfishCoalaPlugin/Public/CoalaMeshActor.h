// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoalaActor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoalaMeshGenerator.h"
#include "CoalaMeshActor.generated.h"

//UCLASS(HideCategories=(Input,Rendering,Replication,Collision,Utilities,LOD,Cooking,Game,Actor,"Coala Mesh Actor"))
UCLASS()
class THOUGHTFISHCOALAPLUGIN_API ACoalaMeshActor
: public ACoalaActor
{
	GENERATED_BODY()
	
	public:	
		// Sets default values for this actor's properties
		ACoalaMeshActor();
		// Called every frame
		virtual void Tick(float DeltaTime) override;

		UPROPERTY( VisibleAnywhere, Category="mesh" ) class UProceduralMeshComponent* mesh;
		
		virtual int cleanupAllAttachedActors();
	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;
};
