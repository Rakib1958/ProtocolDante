#include "PlayerCameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

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
	if (Character->GetClass()->ImplementsInterface(UGameplayCameraInterface::StaticClass()))
	{
		bIsOverrideActive = IGameplayCameraInterface::Execute_OverrideBaseCamera(Character);
		if (bIsOverrideActive)
		{
			return;
		}
		DesiredCameraState = IGameplayCameraInterface::Execute_GetCharacterPropertiesForSpringArmCamera(Character);
		EvaluateAndInterpBase(DeltaTime);
		EvaluateAndInterpStanceOffset(DeltaTime);
		EvaluateAndInterpShoulderOffset(DeltaTime);
		UpdateCapsuleZSmoothing(DeltaTime);
		UpdateNotifyOverride(DeltaTime);
		if (CharacterMovement->IsFalling())
		{
			UpdateFallFeel(DeltaTime);
		}
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


void UPlayerCameraComponent::ApplyToSpringArm()
{
	// ── Layer 1: base + stance + shoulder ────────────────────────────────────
	float FinalArmLength = FMath::Max(
		CurrentBase.TargetArmLength
		+ CurrentStanceOffset.ArmLengthDelta
		- FallArmReduction,
		0.f
	);

	FVector FinalSocketOffset = CurrentBase.SocketOffset
		+ CurrentStanceOffset.SocketOffsetDelta
		+ CurrentShoulderOffset.SocketOffsetDelta;

	FVector FinalPivot = CurrentBase.PivotOffset + CurrentStanceOffset.PivotOffsetDelta;

	// Z smoothing only adds a correction during stance transitions
	// so capsule shrink/grow doesn't snap the camera
	if (bIsStanceTransitioning)
	{
		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		if (IsValid(Capsule))
		{
			float ActualCapsuleZ = Capsule->GetScaledCapsuleHalfHeight();
			float ZCorrection = SmoothedPivotZ - ActualCapsuleZ;
			FinalPivot.Z += ZCorrection;
		}
	}

	float FinalFOV = CurrentBase.FieldOfView
		+ CurrentStanceOffset.FOVDelta
		+ FallFOVReduction;  // additive, negative = zoom in

	// ── Layer 2: anim-notify override (additive on top) ───────────────────────
	if (bIsOverrideActive)
	{
		FinalSocketOffset += CurrentNotifyOverride.SocketOffsetDelta;
		FinalFOV += CurrentNotifyOverride.FOVDelta;
		// Rotation delta is applied as an additive offset to the spring arm's
		// current relative rotation so it stacks cleanly with pawn control rotation
		FRotator CurrentRel = SpringArmRef->GetRelativeRotation();
		SpringArmRef->SetRelativeRotation(CurrentRel + CurrentNotifyOverride.RotationDelta);
	}

	// ── Write to components ───────────────────────────────────────────────────
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
	if (IsValid(Character))
	{
		CharacterMovement = Character->GetCharacterMovement();
	}
}

void UPlayerCameraComponent::UpdateCapsuleZSmoothing(float DeltaTime)
{
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!IsValid(Capsule)) return;
	if (DesiredCameraState.StanceOffset != LastStanceOffset)
	{
		LastStanceOffset = DesiredCameraState.StanceOffset;
		bIsStanceTransitioning = true;
	}
	if (bIsStanceTransitioning)
	{
		float TargetZ = Capsule->GetScaledCapsuleHalfHeight();

		SmoothedPivotZ = FMath::FInterpTo(SmoothedPivotZ, TargetZ, DeltaTime, CrouchZInterpSpeed);

		// Stop transitioning when close enough
		if (FMath::IsNearlyEqual(SmoothedPivotZ, TargetZ, 0.5f))
		{
			SmoothedPivotZ = TargetZ;
			bIsStanceTransitioning = false;
		}
	}
}

// ── Anim-Notify Camera Override ───────────────────────────────────────────────

void UPlayerCameraComponent::BeginCameraOverride(FStruct_CameraNotifyParams DefaultParams,
	FStruct_CameraNotifyParams TargetParams)
{
	NotifyDefaultParams = DefaultParams;
	NotifyTargetParams = TargetParams;
	bIsOverrideActive = true;
	bNotifyBlendingIn = true;
	bNotifyBlendingOut = false;
}

void UPlayerCameraComponent::EndCameraOverride()
{
	// Begin blending back toward the default params supplied at notify begin.
	// bIsOverrideActive stays true until UpdateNotifyOverride confirms it settled.
	bNotifyBlendingIn = false;
	bNotifyBlendingOut = true;
}

void UPlayerCameraComponent::UpdateNotifyOverride(float DeltaTime)
{
	if (!bIsOverrideActive) return;

	// Choose the target for this frame
	const FStruct_CameraNotifyParams& Target =
		bNotifyBlendingIn ? NotifyTargetParams : NotifyDefaultParams;

	float Speed = Target.InterpSpeed;

	CurrentNotifyOverride.SocketOffsetDelta = FMath::VInterpTo(
		CurrentNotifyOverride.SocketOffsetDelta, Target.SocketOffsetDelta, DeltaTime, Speed);

	CurrentNotifyOverride.RotationDelta = FMath::RInterpTo(
		CurrentNotifyOverride.RotationDelta, Target.RotationDelta, DeltaTime, Speed);

	CurrentNotifyOverride.FOVDelta = FMath::FInterpTo(
		CurrentNotifyOverride.FOVDelta, Target.FOVDelta, DeltaTime, Speed);

	// Once blending out, check if we've settled back to zero/default
	if (bNotifyBlendingOut)
	{
		bool bSocketSettled = CurrentNotifyOverride.SocketOffsetDelta.Equals(Target.SocketOffsetDelta, 0.5f);
		bool bRotationSettled = CurrentNotifyOverride.RotationDelta.Equals(Target.RotationDelta, 0.1f);
		bool bFOVSettled = FMath::IsNearlyEqual(CurrentNotifyOverride.FOVDelta, Target.FOVDelta, 0.1f);

		if (bSocketSettled && bRotationSettled && bFOVSettled)
		{
			// Fully restored — clear everything so normal evaluation resumes
			CurrentNotifyOverride = FStruct_CameraNotifyParams();
			bIsOverrideActive = false;
			bNotifyBlendingOut = false;
		}
	}
}

void UPlayerCameraComponent::UpdateFallFeel(float DeltaTime)
{
	if (!IsValid(Character)) return;

	float FallVelocity = Character->GetCharacterMovement()->Velocity.Z;

	// Only zoom on significant downward velocity (below -600 cm/s)
	float FallT = FMath::Clamp(
		FMath::GetMappedRangeValueClamped(
			FVector2D(-600.f, -2400.f),  // velocity range
			FVector2D(0.f, 1.f),          // normalized
			FallVelocity
		), 0.f, 1.f
	);
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("FallT, %f"), FallT));
	}*/
	// Target FOV reduction — max 15 degrees zoom at terminal velocity
	float TargetFOVReduction = FMath::Lerp(0.f, -15.f, FallT);
	FallFOVReduction = FMath::FInterpTo(
		FallFOVReduction, TargetFOVReduction, DeltaTime, 10.f // slow in
	);

	// Also tighten arm length slightly — "death creeping in" feel
	float TargetArmReduction = FMath::Lerp(0.f, -60.f, FallT);
	FallArmReduction = FMath::FInterpTo(
		FallArmReduction, TargetArmReduction, DeltaTime, 10.f
	);
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
	CameraRef->AttachToComponent(SpringArmRef, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndPoint"));
	CameraRef->RegisterComponent();
}

void UPlayerCameraComponent::InitializeCamera()
{
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef)) return;

	SpringArmRef->SetRelativeLocation(SpringArmLocation);
	SpringArmRef->SetRelativeRotation(SpringArmRotation);
	SpringArmRef->bUsePawnControlRotation = true;
	SpringArmRef->TargetArmLength = TargetArmLength;
	SpringArmRef->bEnableCameraLag = true;
	SpringArmRef->bEnableCameraRotationLag = true;
	SpringArmRef->CameraLagSpeed = LagSpeed;
	SpringArmRef->CameraRotationLagSpeed = RotationLagSpeed;
	SpringArmRef->bDoCollisionTest = true;  // use built-in spring arm collision probe

	CameraRef->FieldOfView = FieldOfView;
	CameraRef->bAutoActivate = true;

	// Seed interp state with defaults so first frame has no pop
	CurrentBase.TargetArmLength = TargetArmLength;
	CurrentBase.PivotOffset = SpringArmLocation;
	CurrentBase.FieldOfView = FieldOfView;
	CurrentBase.LagSpeed = LagSpeed;
	CurrentBase.RotationLagSpeed = RotationLagSpeed;
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