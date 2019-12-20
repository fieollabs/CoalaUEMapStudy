// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "TopDownPlayerController.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
//#include "Engine.h"
#include "../Plugins/Runtime/LocationServicesBPLibrary/Source/LocationServicesBPLibrary/Classes/LocationServicesImpl.h"
#include "CoalaBlueprintUtility.h"

ATopDownPlayerController::ATopDownPlayerController()
{
	bShowMouseCursor = true;
	InputActive = true;
	//DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ATopDownPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	/*if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}*/
}

void ATopDownPlayerController::SetInput(bool input)
{
	InputActive = input;
}

void ATopDownPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	//InputComponent->BindAxis("Move Forward", this, &ATopDownPlayerController::OnMoveForward);
	//InputComponent->BindAxis("Move Right", this, &ATopDownPlayerController::OnMoveRight);

}

FVector ATopDownPlayerController::GetHitpointWithWorldAtCourse()
{
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
//	UE_LOG( LogTemp, Warning, TEXT("Hit.Locaiton: %s"), *Hit.Location.ToString() );
/*
	FHitResult hit2;
	{
		FVector worldLocation = FVector::ZeroVector;
		FVector worldDirection = FVector::ZeroVector;
		this->DeprojectMousePositionToWorld( worldLocation, worldDirection );
	
		FVector end = worldDirection*1000+worldLocation;
		this->GetWorld()->LineTraceSingleByChannel( hit2, worldLocation, end, ECC_Visibility );
		UE_LOG( LogTemp, Warning, TEXT("hit2.Locaiton: %s"), *hit2.Location.ToString() );
	}
*/

	if( !Hit.bBlockingHit )
		return FVector::ZeroVector;

	if( !InputActive )
		return FVector::ZeroVector;

	// We hit something, move there
	//SetNewMoveDestination(Hit.ImpactPoint);

	float lon = 0;
	float lat = 0;
	UCoalaBlueprintUtility::WorldPositionToGpsPosition( Hit.ImpactPoint.X, Hit.ImpactPoint.Y, lon, lat );

//	UE_LOG( LogTemp, Warning, TEXT( "WorldPositionToGpsPosition - lon: %f lat: %f" ), lon, lat );

	return FVector(
		lon,
		lat,
		0
	);
}

FVector ATopDownPlayerController::MoveToTouchLocation(ETouchIndex::Type type, FVector location)
{

	//FVector2D ScreenSpaceLocation(Location);

	// Trace to see what is under the touch location
	FHitResult HitResult;
	//GetHitResultAtScreenPosition(ScreenSpaceLocation, CurrentClickTraceChannel, true, HitResult);
	GetHitResultUnderFinger(ETouchIndex::Touch1, ECC_Visibility, false, HitResult);
			// We hit something, move there
	if (!HitResult.bBlockingHit)
		return FVector::ZeroVector;

	if (!InputActive)
		return FVector::ZeroVector;

	float lon = 0;
	float lat = 0;
	UCoalaBlueprintUtility::WorldPositionToGpsPosition(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, lon, lat);

	UE_LOG(LogTemp, Warning, TEXT("WorldPositionToGpsPosition - lon: %f lat: %f"), lon, lat);

	return FVector(
		lon,
		lat,
		0
	);
	
}

void ATopDownPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	APawn* const MyPawn = GetPawn();
	if (MyPawn)
	{
		float const Distance = FVector::Dist(DestLocation, MyPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f))
		{
			MyPawn->TeleportTo(DestLocation, FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

void ATopDownPlayerController::OnMoveForward(float Value)
{
	if (InputActive)
	{
		APawn* const MyPawn = GetPawn();
		if (MyPawn != NULL)
			MyPawn->AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
	}

}

void ATopDownPlayerController::OnMoveRight(float Value)
{
	if (InputActive)
	{
		APawn* const MyPawn = GetPawn();
		if (MyPawn != NULL)
			MyPawn->AddMovementInput(FVector(0.0f, 1.0f, 0.0f), Value);
	}
}
