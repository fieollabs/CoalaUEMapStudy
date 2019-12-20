// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TopDownPlayerController.generated.h"

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API ATopDownPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	ATopDownPlayerController();
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	void SetInput(bool input);
	
	FVector GetHitpointWithWorldAtCourse();
	FVector MoveToTouchLocation(ETouchIndex::Type type, FVector location);

protected:

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;
	bool InputActive;

	// Begin PlayerController interface
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	//void MoveToMouseCursor();

	/** Navigate player to the current touch location. */
	

	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnMoveForward(float Value);
	void OnMoveRight(float Value);
};
