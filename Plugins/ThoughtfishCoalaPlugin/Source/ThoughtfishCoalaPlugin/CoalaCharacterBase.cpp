// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaCharacterBase.h"
#include "CoalaInteractableActor.h"
#include "Components/InputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "CoalaBlueprintUtility.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TopDownPlayerController.h"
#include "CoalaBluePrintUtility.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Engine.h"

// Sets default values
ACoalaCharacterBase::ACoalaCharacterBase()
{
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = distanceToPLayer;
	CameraBoom->RelativeRotation = cameraRotation;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	//CharcterMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainBody"));
	//CharcterMesh1->SetupAttachment(RootComponent);
	
	//CharcterMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshFloor"));
	//CharcterMesh2->SetupAttachment(CharcterMesh1);
	//CharcterMesh2->RelativeLocation = FVector(0.0f, 0.0f, 60.0f);

	// Create a decal in the world to show the cursor's location
	/*
	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/Map1/Materials/MouseCursor/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(1600.0f, 3200.0f, 3200.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());
	*/

}
void ACoalaCharacterBase::SetupInputComponent()
{
	//InputComponent->BindAction("SetDestination", IE_Pressed, this, &ACoalaCharacter::OnSetDestinationPressed);
	//InputComponent->BindAction("SetDestination", IE_Released, this, &ACoalaCharacter::OnSetDestinationReleased);
	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ACoalaCharacterBase::OnSetDestinationPressed);
	InputComponent->BindTouch(IE_Pressed, this, &ACoalaCharacterBase::OnSetDestinationTouched);
}

void ACoalaCharacterBase::OnSetDestinationPressed()
{
	ATopDownPlayerController* c = Cast<ATopDownPlayerController>(GetController());
	if( !c )
		return;

	FVector world_hit_position = c->GetHitpointWithWorldAtCourse();
	if( world_hit_position == FVector::ZeroVector )
	{
		UE_LOG(LogTemp, Warning, TEXT("ACoalaCharacter: Nothing hit with GetHitpointWithWorldAtCourse() ?!"));
		return;
	}
		OnWorldPositionChanged.Broadcast( world_hit_position.X, world_hit_position.Y );
}
void ACoalaCharacterBase::OnSetDestinationTouched(ETouchIndex::Type type, FVector location)
{
	ATopDownPlayerController* c = Cast<ATopDownPlayerController>(GetController());
	if (!c)
		return;
	FVector world_hit_position = c->MoveToTouchLocation(type, location);
	if (world_hit_position.X != 0 && world_hit_position.Y != 0)
		OnWorldPositionChanged.Broadcast(world_hit_position.X, world_hit_position.Y);
}

void ACoalaCharacterBase::OnSetDestinationReleased()
{
	ATopDownPlayerController* c = Cast<ATopDownPlayerController>(GetController());
	if( !c )
		return;

}

// Called when the game starts or when spawned
void ACoalaCharacterBase::BeginPlay()
{
	Super::BeginPlay();	
	playercontroller = Cast<APlayerController>(GetController());

	SetupInputComponent();

	cameraRotation = FRotator(-60.f, 0.f, 0.f);
	distanceToPLayer = 80000.f;
	cursorSize = FVector(1600.0f, 3200.0f, 3200.0f);
	//finteractionRadius = 20000;
	AdjustInteractionRadiusTexture(interactionRadius);
}

void ACoalaCharacterBase::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("InteractPOI", IE_Pressed, this, &ACoalaCharacterBase::OnPoiInteract);
}

// Called every frame
void ACoalaCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (APlayerController* PC = playercontroller)
	{
		FHitResult TraceHitResult;
		PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
		FVector CursorFV = TraceHitResult.ImpactNormal;
		FRotator CursorR = CursorFV.Rotation();
		//CursorToWorld->SetWorldLocation(TraceHitResult.Location);
		//CursorToWorld->SetWorldRotation(CursorR);

		if (PC->IsInputKeyDown(EKeys::E) == true)
		{
			OnRotateCamera(-1);
		}
		else if (PC->IsInputKeyDown(EKeys::Q) == true)
		{
			OnRotateCamera(1);
		}
		if (PC->IsInputKeyDown(EKeys::T) == true)
		{
			OnZoomCamera(1);
		}
		else if (PC->IsInputKeyDown(EKeys::G) == true)
		{
			OnZoomCamera(-1);
		}
		//PC->InputTouch()
	}
}

void ACoalaCharacterBase::OnPoiInteract()
{
	if (APlayerController* PC = playercontroller)
	{
		FHitResult Hit;
		PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			ACoalaInteractableActor* a = Cast<ACoalaInteractableActor>(Hit.GetActor());

			if( a )
			{

				BPOnPoiInteract( a, true );
			}
			else
			{
				//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("no POI found"));
				BPOnPoiInteract( nullptr, false );
			}
				

		}
	}
}

void ACoalaCharacterBase::EnableInput(APlayerController* givenPlayerController = nullptr)
{
	if (givenPlayerController)
	{
		Super::EnableInput(givenPlayerController);
	}
	else if (playercontroller)
	{
		Super::EnableInput(playercontroller);
		ATopDownPlayerController* cast = Cast<ATopDownPlayerController>(playercontroller);
		if (cast)
		{
			cast->SetInput(true);
		}
	}
	
}

void ACoalaCharacterBase::DisableInput(APlayerController* givenPlayerController = nullptr)
{
	if (givenPlayerController)
	{
		Super::DisableInput(givenPlayerController);
	}
	else if (playercontroller)
	{
		Super::DisableInput(playercontroller);
		ATopDownPlayerController* cast = Cast<ATopDownPlayerController>(playercontroller);
		if (cast)
		{
			cast->SetInput(false);
		}
	}
}

void ACoalaCharacterBase::OnRotateCamera(float distance)
{

	if (CameraBoom)
	{
		FTransform _t = CameraBoom->GetRelativeTransform();
		FRotator rotation = _t.GetRotation().Rotator();
		rotation.Yaw += 1 * distance;
		CameraBoom->SetRelativeRotation(FRotator(rotation.Pitch, rotation.Yaw, 0));
	}
}

void ACoalaCharacterBase::OnZoomCamera(float value)
{
	if (CameraBoom)
	{
		FTransform _t = CameraBoom->GetRelativeTransform();
		FRotator rotation = _t.GetRotation().Rotator();
		if (value > 0 && rotation.Pitch >= -30)
		{
			return;
		}
		else if (value < 0 && rotation.Pitch <= -85)
		{
			return;
		}

		rotation.Pitch += 2 * value;
		CameraBoom->SetRelativeRotation(FRotator(rotation.Pitch, rotation.Yaw, 0));
		CameraBoom->TargetArmLength += (-1)*value * 1000 * UCoalaBlueprintUtility::GetCoalaScale(); 
	}
}
void ACoalaCharacterBase::AdjustInteractionRadiusTexture(float radius)
{
	USkeletalMeshComponent* mesh = GetMesh();
	TArray<USceneComponent* >children = mesh->GetAttachChildren();

	for (int i = 0; i < children.Num(); i++)
	{
		if (children[i]->GetFName() == "SM_InteractionArea")
		{
			
			FBoxSphereBounds bounds = children[i]->Bounds;
			if (radius != 0)
			{
				float scale =  (radius/ (bounds.SphereRadius/100))*UCoalaBlueprintUtility::GetCoalaScale();
				children[i]->SetRelativeScale3D(FVector(scale, scale, 0));
			}
		}

	}


}




