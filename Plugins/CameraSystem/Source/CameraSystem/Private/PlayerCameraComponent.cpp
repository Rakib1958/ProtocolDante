#include "PlayerCameraComponent.h"

UPlayerCameraComponent::UPlayerCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReference();
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void UPlayerCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(SpringArmRef) || !IsValid(CameraRef) || !IsValid(Character)) return;
	if (bIsOverrideActive)
	{
		return;
	}
	// Pull desired state from character via interface each tick
	if (Character->GetClass()->ImplementsInterface(UGameplayCameraInterface::StaticClass()))
	{
		DesiredCameraState = IGameplayCameraInterface::Execute_GetCharacterPropertiesForSpringArmCamera(Character);
		EvaluateAndInterpBase(DeltaTime);
		EvaluateAndInterpStanceOffset(DeltaTime);
		EvaluateAndInterpShoulderOffset(DeltaTime);
		UpdateCapsuleZSmoothing(DeltaTime);
		UpdateCollision(DeltaTime);
		ApplyToSpringArm();
	}
}

// ── Tick Sub-steps ────────────────────────────────────────────────────────────

void UPlayerCameraComponent::EvaluateAndInterpBase(float DeltaTime)
{
	const FStruct_CameraRigParams* Target = BasePresetMap.Find(DesiredCameraState.BasePreset);
	if (!Target) return;

	float Speed = Target->InterpSpeed;
	CurrentBase.TargetArmLength = FMath::FInterpTo(CurrentBase.TargetArmLength, Target->TargetArmLength, DeltaTime, Speed);
	CurrentBase.PivotOffset = FMath::VInterpTo(CurrentBase.PivotOffset, Target->PivotOffset, DeltaTime, Speed);
	CurrentBase.SocketOffset = FMath::VInterpTo(CurrentBase.SocketOffset, Target->SocketOffset, DeltaTime, Speed);
	CurrentBase.FieldOfView = FMath::FInterpTo(CurrentBase.FieldOfView, Target->FieldOfView, DeltaTime, Speed);
	CurrentBase.LagSpeed = FMath::FInterpTo(CurrentBase.LagSpeed, Target->LagSpeed, DeltaTime, Speed);
	CurrentBase.RotationLagSpeed = FMath::FInterpTo(CurrentBase.RotationLagSpeed, Target->RotationLagSpeed, DeltaTime, Speed);
}

void UPlayerCameraComponent::EvaluateAndInterpStanceOffset(float DeltaTime)
{
	const FStruct_CameraRigOffset* Target = StanceOffsetMap.Find(DesiredCameraState.StanceOffset);
	if (!Target) return;

	float Speed = Target->InterpSpeed;
	CurrentStanceOffset.PivotOffsetDelta = FMath::VInterpTo(CurrentStanceOffset.PivotOffsetDelta, Target->PivotOffsetDelta, DeltaTime, Speed);
	CurrentStanceOffset.SocketOffsetDelta = FMath::VInterpTo(CurrentStanceOffset.SocketOffsetDelta, Target->SocketOffsetDelta, DeltaTime, Speed);
	CurrentStanceOffset.ArmLengthDelta = FMath::FInterpTo(CurrentStanceOffset.ArmLengthDelta, Target->ArmLengthDelta, DeltaTime, Speed);
	CurrentStanceOffset.FOVDelta = FMath::FInterpTo(CurrentStanceOffset.FOVDelta, Target->FOVDelta, DeltaTime, Speed);
}

void UPlayerCameraComponent::EvaluateAndInterpShoulderOffset(float DeltaTime)
{
	const FStruct_CameraRigOffset* Target = ShoulderOffsetMap.Find(DesiredCameraState.ShoulderOffset);
	if (!Target) return;

	float Speed = Target->InterpSpeed;
	CurrentShoulderOffset.SocketOffsetDelta = FMath::VInterpTo(CurrentShoulderOffset.SocketOffsetDelta, Target->SocketOffsetDelta, DeltaTime, Speed);
	// Shoulder only ever shifts socket offset — pivot and arm length untouched
}

void UPlayerCameraComponent::UpdateCapsuleZSmoothing(float DeltaTime)
{
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!IsValid(Capsule)) return;

	float TargetZ = Capsule->GetScaledCapsuleHalfHeight()
		+ CurrentBase.PivotOffset.Z
		+ CurrentStanceOffset.PivotOffsetDelta.Z;

	if (!bPivotZInitialized)
	{
		SmoothedPivotZ = TargetZ;
		bPivotZInitialized = true;
	}

	SmoothedPivotZ = FMath::FInterpTo(SmoothedPivotZ, TargetZ, DeltaTime, CrouchZInterpSpeed);
}

void UPlayerCameraComponent::UpdateCollision(float DeltaTime)
{
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef)) return;

	FVector Start = SpringArmRef->GetComponentLocation();
	FVector End = CameraRef->GetComponentLocation();

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(12.f),
		Params
	);

	if (bHit)
	{
		float HitDist = FVector::Dist(Start, Hit.ImpactPoint);
		float ArmCorrection = CurrentBase.TargetArmLength - HitDist;
		CollisionArmCorrection = FMath::FInterpTo(CollisionArmCorrection, ArmCorrection, DeltaTime, 15.f);

		// Push socket offset toward center based on which side the wall is on
		float RightDot = FVector::DotProduct(Hit.ImpactNormal, Character->GetActorRightVector());
		float OffsetCorrection = FMath::Abs(CurrentBase.SocketOffset.Y + CurrentShoulderOffset.SocketOffsetDelta.Y) * FMath::Abs(RightDot);
		CollisionOffsetCorrection = FMath::FInterpTo(CollisionOffsetCorrection, OffsetCorrection, DeltaTime, 8.f);
	}
	else
	{
		// Restore — slow out feels more natural than snapping back
		CollisionArmCorrection = FMath::FInterpTo(CollisionArmCorrection, 0.f, DeltaTime, 3.f);
		CollisionOffsetCorrection = FMath::FInterpTo(CollisionOffsetCorrection, 0.f, DeltaTime, 4.f);
	}
}

void UPlayerCameraComponent::ApplyToSpringArm()
{
	// Combine all layers
	float FinalArmLength = FMath::Max(
		CurrentBase.TargetArmLength
		+ CurrentStanceOffset.ArmLengthDelta
		- CollisionArmCorrection,
		0.f
	);

	FVector FinalSocketOffset = CurrentBase.SocketOffset
		+ CurrentStanceOffset.SocketOffsetDelta
		+ CurrentShoulderOffset.SocketOffsetDelta;

	// Collision pushes socket toward center (reduces Y magnitude)
	float SocketYSign = FMath::Sign(FinalSocketOffset.Y);
	FinalSocketOffset.Y = FMath::Max(
		FMath::Abs(FinalSocketOffset.Y) - CollisionOffsetCorrection, 0.f
	) * SocketYSign;

	FVector FinalPivot = CurrentBase.PivotOffset + CurrentStanceOffset.PivotOffsetDelta;
	// Z is driven by smoothed capsule tracking, X and Y from rig
	FinalPivot.Z = SmoothedPivotZ;

	float FinalFOV = CurrentBase.FieldOfView + CurrentStanceOffset.FOVDelta;

	// Write to spring arm
	SpringArmRef->TargetArmLength = FinalArmLength;
	SpringArmRef->SocketOffset = FinalSocketOffset;
	SpringArmRef->SetRelativeLocation(FinalPivot);
	SpringArmRef->CameraLagSpeed = CurrentBase.LagSpeed;
	SpringArmRef->CameraRotationLagSpeed = CurrentBase.RotationLagSpeed;
	CameraRef->FieldOfView = FinalFOV;
}

// ── Initialization ────────────────────────────────────────────────────────────

void UPlayerCameraComponent::SetReference()
{
	Character = Cast<ACharacter>(GetOwner());
}

void UPlayerCameraComponent::InitCamera(bool bUseGameplayCamera)
{
	if (!IsValid(Character)) SetReference();
	if (!IsValid(Character)) return;

	if (bUseGameplayCamera)
	{
		CreateGameplayCamera();
		InitializeGameplayCamera();
	}
	else
	{
		CreateCamera();
		InitializeCamera();
	}
}

void UPlayerCameraComponent::SetCameraReference_Implementation()
{
	// Override in Blueprint to assign GameplayCamera asset
}

void UPlayerCameraComponent::CreateCamera()
{
	SpringArmRef = NewObject<USpringArmComponent>(GetOwner(), TEXT("CameraBoom"));
	SpringArmRef->AttachToComponent(Character->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SpringArmRef->RegisterComponent();

	CameraRef = NewObject<UCameraComponent>(GetOwner(), TEXT("Camera"));
	CameraRef->AttachToComponent(SpringArmRef, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CameraRef->RegisterComponent();
}

void UPlayerCameraComponent::InitializeCamera()
{
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef)) return;

	//SpringArmRef->SetRelativeLocation(SpringArmLocation);
	//SpringArmRef->SetRelativeRotation(SpringArmRotation);
	//SpringArmRef->bUsePawnControlRotation = true;
	//SpringArmRef->TargetArmLength = TargetArmLength;
	//SpringArmRef->bEnableCameraLag = true;
	//SpringArmRef->bEnableCameraRotationLag = true;
	//SpringArmRef->CameraLagSpeed = LagSpeed;
	//SpringArmRef->CameraRotationLagSpeed = RotationLagSpeed;
	//SpringArmRef->bDoCollisionTest = false; // we handle collision ourselves

	//CameraRef->FieldOfView = FieldOfView;
	//CameraRef->bAutoActivate = true;

	//// Seed interp state with defaults so first frame has no pop
	//CurrentBase.TargetArmLength = TargetArmLength;
	//CurrentBase.PivotOffset = SpringArmLocation;
	//CurrentBase.FieldOfView = FieldOfView;
	//CurrentBase.LagSpeed = LagSpeed;
	//CurrentBase.RotationLagSpeed = RotationLagSpeed;
}

void UPlayerCameraComponent::CreateGameplayCamera()
{
	if (!IsValid(GameplayCameraRef))
	{
		GameplayCameraRef = NewObject<UGameplayCameraComponent>(GetOwner(), TEXT("GameplayCamera"));
		GameplayCameraRef->bAutoActivate = false;
	}
	GameplayCameraRef->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GameplayCameraRef->RegisterComponent();
	SetCameraReference();
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