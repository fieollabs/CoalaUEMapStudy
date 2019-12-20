// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaMeshActor.h"
#include "ProceduralMeshComponent.h"
#include "CoalaArea.h"
#include "CoalaMeshGenerator.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "GeoConverter.h"
#include "CoalaBlueprintUtility.h"
#include <cmath>

// Sets default values

ACoalaMeshActor::ACoalaMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ACoalaMeshActor/mesh"));
	RootComponent = mesh;
	mesh->bUseAsyncCooking = true;
	bReplicates = true;
}

// Called when the game starts or when spawned
void 
ACoalaMeshActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void 
ACoalaMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int ACoalaMeshActor::cleanupAllAttachedActors()
{
	int ret = ACoalaActor::cleanupAllAttachedActors();

	return ret;
}
