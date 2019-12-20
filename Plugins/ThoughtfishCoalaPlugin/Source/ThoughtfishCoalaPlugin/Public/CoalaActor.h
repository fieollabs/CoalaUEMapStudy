// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoalaActor.generated.h"

UCLASS()
class ACoalaActor : public AActor
{
	GENERATED_BODY()
	
	public:	
		// Sets default values for this actor's properties
		ACoalaActor();
	
		TArray<class AActor*> allAttachedActors;
		
		// Called every frame
		virtual void Tick(float DeltaTime) override;
		
		virtual int cleanupAllAttachedActors();

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;
};
