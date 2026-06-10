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
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef)) return;
	if (bIsOverrideActive) return;
	OnCameraPresetChanged.Broadcast();
	EvaluateTargetRig();        // picks OverrideRig or RigMap[ActiveStyle]
	InterpCurrentRig(DeltaTime); // FInterpTo per field toward target
	UpdateCollision(DeltaTime);  // sphere sweep, modifies correction values
	ApplyCurrentRigToSpringArm(); // writes everything to the spring arm
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

// custom logic
void UPlayerCameraComponent::EvaluateTargetRig()
{
	const FStruct_CameraRigParams* Found = RigMap.Find(ActivePreset);
	if (Found) TargetRig = *Found;
}
void UPlayerCameraComponent::InterpCurrentRig(float DeltaTime)
{
	float Speed = TargetRig.InterpSpeed;
	CurrentRig.TargetArmLength = FMath::FInterpTo(CurrentRig.TargetArmLength, TargetRig.TargetArmLength, DeltaTime, Speed);
	CurrentRig.SocketOffset = FMath::VInterpTo(CurrentRig.SocketOffset, TargetRig.SocketOffset, DeltaTime, Speed);
	CurrentRig.PivotOffset = FMath::VInterpTo(CurrentRig.PivotOffset, TargetRig.PivotOffset, DeltaTime, Speed);
	CurrentRig.FieldOfView = FMath::FInterpTo(CurrentRig.FieldOfView, TargetRig.FieldOfView, DeltaTime, Speed);
}
void UPlayerCameraComponent::ApplyCurrentRigToSpringArm()
{
	float FinalArmLength = FMath::Max(
		CurrentRig.TargetArmLength - CollisionArmLengthCorrection, 0.f
	);
	FVector FinalOffset = CurrentRig.SocketOffset;
	FinalOffset.Y = FMath::FInterpTo(
		FinalOffset.Y,
		FinalOffset.Y - CollisionOffsetCorrection,
		GetWorld()->GetDeltaSeconds(), 8.f
	);

	SpringArmRef->TargetArmLength = FinalArmLength;
	SpringArmRef->SocketOffset = FinalOffset;
	SpringArmRef->SetRelativeLocation(CurrentRig.PivotOffset);
	CameraRef->FieldOfView = CurrentRig.FieldOfView;
}
void UPlayerCameraComponent::UpdateCollision(float DeltaTime)
{
	if (!IsValid(SpringArmRef)) return;

	FVector Start = SpringArmRef->GetComponentLocation();
	FVector End = CameraRef->GetComponentLocation();
	float   Radius = 12.f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

	if (bHit)
	{
		// shorten arm to hit distance
		float HitDist = FVector::Dist(Start, Hit.ImpactPoint);
		CollisionArmLengthCorrection = FMath::FInterpTo(
			CollisionArmLengthCorrection,
			CurrentRig.TargetArmLength - HitDist,
			DeltaTime, 15.f     // fast in
		);

		// push socket offset toward center based on wall side
		float RightDot = FVector::DotProduct(
			Hit.ImpactNormal,
			Character->GetActorRightVector()
		);
		CollisionOffsetCorrection = FMath::FInterpTo(
			CollisionOffsetCorrection,
			FMath::Abs(CurrentRig.SocketOffset.Y) * FMath::Abs(RightDot),
			DeltaTime, 8.f
		);
	}
	else
	{
		// restore slowly
		CollisionArmLengthCorrection = FMath::FInterpTo(
			CollisionArmLengthCorrection, 0.f, DeltaTime, 3.f   // slow out
		);
		CollisionOffsetCorrection = FMath::FInterpTo(
			CollisionOffsetCorrection, 0.f, DeltaTime, 4.f
		);
	}
}