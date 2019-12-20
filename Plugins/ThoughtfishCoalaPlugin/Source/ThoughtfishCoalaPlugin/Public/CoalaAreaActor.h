// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoalaAreaActor.generated.h"

UCLASS()
class ACoalaAreaActor : public AActor
{
	GENERATED_BODY()
	
	public:	
		// Sets default values for this actor's properties
		ACoalaAreaActor();

		class ACoalaMeshActor* _refAreaDimensions;

		class ACoalaActor* _refAllCells;
		class ACoalaActor* _refAllWaters;
		class ACoalaActor* _refAllBuildings;
		class ACoalaActor* _refAllPOIs;
		class ACoalaActor* _refAllStreets;

		class ACoalaActor* _refAllDecorations;

		int cleanupHoldedCoalaActors();

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;

	public:	
		// Called every frame
		virtual void Tick(float DeltaTime) override;


};
