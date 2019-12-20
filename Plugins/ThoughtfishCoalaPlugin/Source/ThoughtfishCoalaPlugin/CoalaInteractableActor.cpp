// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaInteractableActor.h"
#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "CoalaCharacterBase.h"
#include "Engine.h"

// Sets default values
ACoalaInteractableActor::ACoalaInteractableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ACoalaInteractableActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACoalaInteractableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
