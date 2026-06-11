#include "PlayerCameraComponent.h"

// Sets default values for this component's properties
UPlayerCameraComponent::UPlayerCameraComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReference();
	// ...
	
}


// Called every frame
void UPlayerCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef) || bIsOverrideActive) return;
	if (IsValid(Character) && Character->GetClass()->ImplementsInterface(UGameplayCameraInterface::StaticClass()))
	{
		FStruct_SpringArmCamera CurrentDesiredCameraProperties = IGameplayCameraInterface::Execute_GetCharacterPropertiesForSpringArmCamera(Character);
		// apply stance offset

		// apply base rig

		// apply shoulder offset
	}
	// ...
}

void UPlayerCameraComponent::SetCameraReference_Implementation()
{
}
void UPlayerCameraComponent::CreateGameplayCamera()
{
	if (!IsValid(GameplayCameraRef))
	{
		GameplayCameraRef = NewObject<UGameplayCameraComponent>(
			GetOwner(), TEXT("GameplayCamera")
		);
		GameplayCameraRef->bAutoActivate = false;
	}
	GameplayCameraRef->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GameplayCameraRef->RegisterComponent();
	SetCameraReference();
}
void UPlayerCameraComponent::CreateCamera()
{
	SpringArmRef = NewObject<USpringArmComponent>(GetOwner(), TEXT("Camera Boom"));
	SpringArmRef->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SpringArmRef->RegisterComponent();
	CameraRef = NewObject<UCameraComponent>(GetOwner(), TEXT("Camera"));
	CameraRef->AttachToComponent(SpringArmRef, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CameraRef->RegisterComponent();
}
void UPlayerCameraComponent::InitCamera(bool bUseGameplayCamera)
{
	if (!IsValid(Character))
	{
		SetReference();
	}
	if (bUseGameplayCamera)
	{
		CreateGameplayCamera();
		InitializeGameplayCamera();
	}
	else {
		CreateCamera();
		InitializeCamera();
	}
}
void UPlayerCameraComponent::SetReference()
{
	Character = Cast<ACharacter>(GetOwner());
}
void UPlayerCameraComponent::InitializeCamera()
{
	if (!IsValid(CameraRef) && !IsValid(SpringArmRef)) return;
	
	SpringArmRef->SetRelativeLocation(SpringArmLocation);
	SpringArmRef->bUsePawnControlRotation = true;
	SpringArmRef->TargetArmLength = TargetArmLength;
	SpringArmRef->SetRelativeRotation(SpringArmRotation);
	SpringArmRef->bEnableCameraLag = true;
	SpringArmRef->bEnableCameraRotationLag = true;
	SpringArmRef->CameraLagSpeed = LagSpeed;
	SpringArmRef->CameraRotationLagSpeed = RotationLagSpeed;
	
	CameraRef->FieldOfView = FieldOfView;
	CameraRef->bAutoActivate = true;
}
void UPlayerCameraComponent::InitializeGameplayCamera()
{
	if (!IsValid(GameplayCameraRef)) return;
	GameplayCameraRef->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	GameplayCameraRef->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	GameplayCameraRef->ActivateCameraForPlayerController(
		Cast<APlayerController>(Character->GetController())
	);
}

