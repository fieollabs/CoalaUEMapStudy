// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaActor.h"

// Sets default values
ACoalaActor::ACoalaActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
}

// Called when the game starts or when spawned
void ACoalaActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACoalaActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int ACoalaActor::cleanupAllAttachedActors()
{
	int ret = allAttachedActors.Num();

	for( int i = 0; i < allAttachedActors.Num(); ++i )
	{
		AActor* current = allAttachedActors[i];
		//current->ConditionalBeginDestroy();
		//current->BeginDestroy();
		bool destroyed = current->Destroy();
	}

	return ret;
}
