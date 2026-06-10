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

	// ...
}

void UPlayerCameraComponent::CreateGameplayCamera_Implementation()
{
	GameplayCameraRef = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("Gameplay Camera"));
}
void UPlayerCameraComponent::InitCamera_Implementation(bool bUseGameplayCamera)
{
	if (!IsValid(Character))
	{
		SetReference();
	}
	if (bUseGameplayCamera)
	{
		InitializeGameplayCamera();
	}
	else {
		InitializeCamera();
	}
}
void UPlayerCameraComponent::CreateCamera_Implementation()
{
	SpringArmRef = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraRef = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
}
void UPlayerCameraComponent::SetReference()
{
	Character = Cast<ACharacter>(GetOwner());
}
void UPlayerCameraComponent::InitializeCamera()
{
	if (!IsValid(CameraRef))
	{
		SpringArmRef->SetupAttachment(Character->GetRootComponent());
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
}
void UPlayerCameraComponent::InitializeGameplayCamera()
{
	if (!IsValid(GameplayCameraRef))
	{
		CreateGameplayCamera();
	}
	GameplayCameraRef->SetupAttachment(Character->GetMesh());
	GameplayCameraRef->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	GameplayCameraRef->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	GameplayCameraRef->ActivateCameraForPlayerController(Cast<APlayerController>(Character->GetController()));
}

