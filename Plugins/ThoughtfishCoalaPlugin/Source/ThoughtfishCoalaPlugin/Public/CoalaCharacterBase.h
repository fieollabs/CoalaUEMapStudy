// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CoalaCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FOnWorldPositionChanged, float, lon, float, lat );

UCLASS()
class ACoalaCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACoalaCharacterBase();
	virtual void SetupInputComponent();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void DisableInput( class APlayerController* playercontroller) override;
	void EnableInput( class APlayerController* playercontroller) override;

	//CoalaCharacter
	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CursorToWorld subobject **/
	FORCEINLINE class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns StaticMeshes subobject **/
	//FORCEINLINE class UStaticMeshComponent* GetCharacterMesh1() const { return CharcterMesh1; }
	//FORCEINLINE class UStaticMeshComponent* GetCharacterMesh2() const { return CharcterMesh2; }


	/** reference to playercontroller **/
	class APlayerController* playercontroller;

	//Variables
	//Rotation of the Player Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraVariables)
	FRotator cameraRotation;

	//Distance from the Player to the Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraVariables)
	float distanceToPLayer;

	//Size of the Mouse Cursor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MouseCursorVariables)
	FVector cursorSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="interactionRadius") float interactionRadius;

	//Arrays
	//Array Of buildings on the map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Array)
	TArray<class ACoalaInteractableActor*> Buildings;
	
	//Array of POI's on the map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Array)
	TArray<class ACoalaInteractableActor*> POIs;

	//Functions
	UFUNCTION()
	void OnPoiInteract();

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnPoiInteract( class ACoalaInteractableActor* actor, bool clickedOnInteractable );

	void OnRotateCamera(float distance);

	void OnZoomCamera(float value);

	void OnSetDestinationPressed();
	void OnSetDestinationTouched(ETouchIndex::Type type, FVector location);
	void OnSetDestinationReleased();
	UPROPERTY( BlueprintAssignable, Category = "Coala|Character" )
	FOnWorldPositionChanged OnWorldPositionChanged;

private:

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** A decal that projects to the cursor location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Static CharacterMesh */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//	class UStaticMeshComponent* CharcterMesh1;
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//	class UStaticMeshComponent* CharcterMesh2;
	void AdjustInteractionRadiusTexture(float radius);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
