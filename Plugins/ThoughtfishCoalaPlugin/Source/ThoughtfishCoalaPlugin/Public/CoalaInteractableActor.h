// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CoalaInteractableActor.generated.h"

UCLASS()
class ACoalaInteractableActor : public AActor
{
	GENERATED_BODY()
	
	public:	
		// Sets default values for this actor's properties
		ACoalaInteractableActor();

	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Popup)
		bool isVisible;

	public:	
		// Called every frame
		virtual void Tick(float DeltaTime) override;
};
