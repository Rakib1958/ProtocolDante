#include "PlayerCameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

UPlayerCameraComponent::UPlayerCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // we need tick to set our camera params smoothly per frame
}
void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReference(); // we store the character reference at first
}

// ── Tick ──────────────────────────────────────────────────────────────────────
void UPlayerCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(Character) || !IsValid(SpringArmRef) || !IsValid(CameraRef)) return; // early return if the character is not valid yet
	// also we use this component only for a spring arm camera based setup, the gameplay camera is configured in its own graph

	// Pull desired state from character via interface
	if (Character->GetClass()->ImplementsInterface(UGameplayCameraInterface::StaticClass()))
	{
		// If interface signals external override (e.g. gameplay camera rig active) — hand off entirely
		if (IGameplayCameraInterface::Execute_OverrideBaseCamera(Character))
			return; // this state is useful for scenerios where we want to use some other view point, like cctv footages or someone else's perspective

		DesiredCameraState = IGameplayCameraInterface::Execute_GetCharacterPropertiesForSpringArmCamera(Character);
	}

	// FPS path
	bool bWantsFPS = DesiredCameraState.ViewMode ==
		FGameplayTag::RequestGameplayTag("CameraSystem.ViewMode.FirstPerson");

	if (bWantsFPS)
	{
		if (!bIsFPSActive) SwitchToFPS(true);
		EvaluateAndInterpFPS(DeltaTime);
		return; // skip TPS evaluation entirely
	}
	else if (bIsFPSActive)
	{
		// Leaving FPS — restore TPS state cleanly
		SwitchToFPS(false);
	}

	// TPS path — stacked layer evaluation
	EvaluateAndInterpBase(DeltaTime);
	EvaluateAndInterpStanceOffset(DeltaTime);
	EvaluateAndInterpShoulderOffset(DeltaTime);


	if (bIsOverrideActive || bNotifyBlendingOut)
	{
		UpdateNotifyOverride(DeltaTime);
	}
	UpdateCapsuleZSmoothing(DeltaTime);
	UpdateFallFeel(DeltaTime);
	ApplyToSpringArm();
}

// ── Layer Evaluation ──────────────────────────────────────────────────────────

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

	CurrentShoulderOffset.SocketOffsetDelta = FMath::VInterpTo(
		CurrentShoulderOffset.SocketOffsetDelta,
		Target->SocketOffsetDelta,
		DeltaTime,
		Target->InterpSpeed
	);
}

void UPlayerCameraComponent::UpdateCapsuleZSmoothing(float DeltaTime)
{
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!IsValid(Capsule)) return;

	// Detect stance change — only smooth during transitions
	if (DesiredCameraState.StanceOffset != LastStanceOffset)
	{
		LastStanceOffset = DesiredCameraState.StanceOffset;
		bIsStanceTransitioning = true;
		SmoothedPivotZ = Capsule->GetScaledCapsuleHalfHeight(); // seed from current
	}

	if (!bIsStanceTransitioning) return;

	float TargetZ = Capsule->GetScaledCapsuleHalfHeight();
	SmoothedPivotZ = FMath::FInterpTo(SmoothedPivotZ, TargetZ, DeltaTime, CrouchZInterpSpeed);

	if (FMath::IsNearlyEqual(SmoothedPivotZ, TargetZ, 0.5f))
	{
		SmoothedPivotZ = TargetZ;
		bIsStanceTransitioning = false;
	}
}
void UPlayerCameraComponent::UpdateFallFeel(float DeltaTime)
{
	if (!IsValid(CharacterMovement)) return;

	float FallVelocity = CharacterMovement->Velocity.Z;

	float FallT = FMath::Clamp(FMath::GetMappedRangeValueClamped(
		FVector2D(-600.f, -2400.f), FVector2D(0.f, 1.f), FallVelocity), 0.f, 1.f);

	FallFOVReduction = FMath::FInterpTo(FallFOVReduction, FMath::Lerp(0.f, -15.f, FallT), DeltaTime, 1.5f);
	FallArmReduction = FMath::FInterpTo(FallArmReduction, FMath::Lerp(0.f, -60.f, FallT), DeltaTime, 1.5f);
}
void UPlayerCameraComponent::ApplyToSpringArm()
{
	// ── Combine base + stance + shoulder ──────────────────────────────────────
	float FinalArmLength = FMath::Max(
		CurrentBase.TargetArmLength
		+ CurrentStanceOffset.ArmLengthDelta
		+ FallArmReduction,
		0.f
	);

	FVector FinalSocketOffset = CurrentBase.SocketOffset
		+ CurrentStanceOffset.SocketOffsetDelta
		+ CurrentShoulderOffset.SocketOffsetDelta;

	FVector FinalPivot = CurrentBase.PivotOffset + CurrentStanceOffset.PivotOffsetDelta;
	if (bIsStanceTransitioning)
	{
		float ActualCapsuleZ = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FinalPivot.Z += SmoothedPivotZ - ActualCapsuleZ;
	}

	float FinalFOV = (CurrentBase.FieldOfView + CurrentStanceOffset.FOVDelta + FallFOVReduction) * FOV_Multiplier;

	// ── Apply notify override additively on top ───────────────────────────────
	if (bIsOverrideActive || bNotifyBlendingOut)
	{
		FinalSocketOffset += CurrentNotifySocketOffset;
		FinalFOV += CurrentNotifyFOVDelta;

		// FIX 2: Apply custom notify rotations directly to the Camera component node instead 
		// of adding them to the Spring Arm, eliminating infinite spinning bugs.
		CameraRef->SetRelativeRotation(CurrentNotifyRotation);
	}
	else
	{
		CameraRef->SetRelativeRotation(FRotator::ZeroRotator);
	}

	// ── Write ─────────────────────────────────────────────────────────────────
	SpringArmRef->TargetArmLength = FinalArmLength;
	SpringArmRef->SocketOffset = FinalSocketOffset;
	SpringArmRef->SetRelativeLocation(FinalPivot);
	SpringArmRef->CameraLagSpeed = CurrentBase.LagSpeed;
	SpringArmRef->CameraRotationLagSpeed = CurrentBase.RotationLagSpeed;
	CameraRef->FieldOfView = FinalFOV;
}

// ── Notify Override ───────────────────────────────────────────────────────────
void UPlayerCameraComponent::BeginCameraOverride(
	FVector TargetSocketOffset, FRotator TargetRotation, float TargetFOVDelta,
	float InterpInSpeed,
	bool bUseCustomDefault,
	FVector DefaultSocketOffset, FRotator DefaultRotation, float DefaultFOVDelta,
	float InterpOutSpeed,
	bool bForceTPS,
	bool bIgnoreIfFPS)
{
	// If told to ignore while in FPS — flag and return
	if (bIgnoreIfFPS && bIsFPSActive)
	{
		bIgnoreThisNotify = true;
		return;
	}
	bIgnoreThisNotify = false;

	// Store target
	NotifyTargetSocketOffset = TargetSocketOffset;
	NotifyTargetRotation = TargetRotation;
	NotifyTargetFOVDelta = TargetFOVDelta;
	NotifyInterpInSpeed = InterpInSpeed;

	// Store restore target
	if (bUseCustomDefault)
	{
		NotifyDefaultSocketOffset = DefaultSocketOffset;
		NotifyDefaultRotation = DefaultRotation;
		NotifyDefaultFOVDelta = DefaultFOVDelta;
	}
	else
	{
		// Zero restore — ApplyToSpringArm naturally returns to rig values
		NotifyDefaultSocketOffset = FVector::ZeroVector;
		NotifyDefaultRotation = FRotator::ZeroRotator;
		NotifyDefaultFOVDelta = 0.f;
	}
	NotifyInterpOutSpeed = InterpOutSpeed;

	bNotifyBlendingIn = true;
	bNotifyBlendingOut = false;
	bIsOverrideActive = true;

	// FPS handling — temporarily drop to TPS if requested
	if (bIsFPSActive && bForceTPS)
	{
		bForcedTPSFromFPS = true;
		SwitchToFPS(false);
	}
}
void UPlayerCameraComponent::EndCameraOverride()
{
	if (bIgnoreThisNotify) return;

	bNotifyBlendingIn = false;
	bNotifyBlendingOut = true;
	// bIsOverrideActive stays true until UpdateNotifyOverride confirms settle
}
void UPlayerCameraComponent::UpdateNotifyOverride(float DeltaTime)
{
	if (bIgnoreThisNotify) return;
	if (!bNotifyBlendingIn && !bNotifyBlendingOut) return;

	// Choose destination
	FVector  DestSocket = bNotifyBlendingIn ? NotifyTargetSocketOffset : NotifyDefaultSocketOffset;
	FRotator DestRot = bNotifyBlendingIn ? NotifyTargetRotation : NotifyDefaultRotation;
	float    DestFOV = bNotifyBlendingIn ? NotifyTargetFOVDelta : NotifyDefaultFOVDelta;
	float    Speed = bNotifyBlendingIn ? NotifyInterpInSpeed : NotifyInterpOutSpeed;

	CurrentNotifySocketOffset = FMath::VInterpTo(CurrentNotifySocketOffset, DestSocket, DeltaTime, Speed);
	CurrentNotifyRotation = FMath::RInterpTo(CurrentNotifyRotation, DestRot, DeltaTime, Speed);
	CurrentNotifyFOVDelta = FMath::FInterpTo(CurrentNotifyFOVDelta, DestFOV, DeltaTime, Speed);

	// Check settle on blend-out
	if (bNotifyBlendingOut)
	{
		bool bSocketSettled = CurrentNotifySocketOffset.Equals(DestSocket, 0.5f);
		bool bRotSettled = CurrentNotifyRotation.Equals(DestRot, 0.1f);
		bool bFOVSettled = FMath::IsNearlyEqual(CurrentNotifyFOVDelta, DestFOV, 0.1f);

		if (bSocketSettled && bRotSettled && bFOVSettled)
		{
			// Reset all override state
			CurrentNotifySocketOffset = FVector::ZeroVector;
			CurrentNotifyRotation = FRotator::ZeroRotator;
			CurrentNotifyFOVDelta = 0.f;
			bNotifyBlendingOut = false;
			bIsOverrideActive = false;

			// Restore FPS if we were forced out
			if (bForcedTPSFromFPS)
			{
				bForcedTPSFromFPS = false;
				SwitchToFPS(true);
			}
		}
	}
}

// ── First Person ──────────────────────────────────────────────────────────────
void UPlayerCameraComponent::EvaluateAndInterpFPS(float DeltaTime)
{
	if (!IsValid(Character->GetMesh()) || !IsValid(CharacterMovement)) return;

	CharacterMovement->bOrientRotationToMovement = false;
	CharacterMovement->bUseControllerDesiredRotation = true; // default, but a GASP-like setup requires you to define again inside the rotation tick

	bool bIsProne = DesiredCameraState.StanceOffset ==
		FGameplayTag::RequestGameplayTag("CameraSystem.Stance.Prone");

	ConfigureFPSViewLimits(bIsProne); // normal fps and prone fps is different

	FVector EyeLocation = Character->GetMesh()->GetSocketLocation(TEXT("head"));
	FVector Forward = Character->GetActorForwardVector();
	FVector Up = Character->GetActorUpVector();

	if (bIsProne)
	{
		SpringArmRef->bEnableCameraRotationLag = true;
		SpringArmRef->CameraRotationLagSpeed = FPSProneRotationLag;
		SpringArmRef->TargetArmLength = 0.f;
		SpringArmRef->TargetOffset = FVector::ZeroVector;

		FVector FinalLoc = EyeLocation
			+ Forward * FPSMeshOffset.X
			+ Up * FPSMeshOffset.Z;
		CameraRef->SetWorldLocation(FinalLoc);
	}
	else
	{
		SpringArmRef->bEnableCameraRotationLag = false;
		SpringArmRef->TargetArmLength = 0.f;
		SpringArmRef->TargetOffset = FVector::ZeroVector;

		CameraRef->SetWorldLocation(EyeLocation + Forward * FPSMeshOffset.X);
	}

	CameraRef->FieldOfView = FMath::FInterpTo(CameraRef->FieldOfView, 90.f, DeltaTime, 10.f);
}
void UPlayerCameraComponent::SwitchToFPS(bool bEnable)
{
	bIsFPSActive = bEnable;
	// tps to fps
	if (bEnable)
	{
		SpringArmRef->bEnableCameraLag = false; // we dont want camera sway while looking in fps
		SpringArmRef->bEnableCameraRotationLag = false;
	}
	else // fps to tps
	{
		SpringArmRef->bEnableCameraLag = true;
		SpringArmRef->bEnableCameraRotationLag = true;
		SpringArmRef->TargetOffset = FVector::ZeroVector;
		SpringArmRef->TargetArmLength = CurrentBase.TargetArmLength;
		CameraRef->SetRelativeLocation(FVector::ZeroVector);
		CameraRef->SetRelativeRotation(FRotator::ZeroRotator);
		if (IsValid(Character->GetCapsuleComponent())) // very important, while proning, the character should always first person, at least in my game, so stance transitioning will be true then and if we dont guard this, there will be an unnecessary offset while getting up from prone
		{
			SmoothedPivotZ = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			bIsStanceTransitioning = false; 
		}
		LastStanceOffset = DesiredCameraState.StanceOffset;
		if (const FStruct_CameraRigParams* Target = BasePresetMap.Find(DesiredCameraState.BasePreset))
			CurrentBase = *Target; // after that we reselect our current stance, the tick will be running any way, so we dont need to think much here
		
	}
}
void UPlayerCameraComponent::ConfigureFPSViewLimits(bool bIsProne)
{
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager)) return;

	float Limit = bIsProne ? FPSPronePitchLimit : FPSNormalPitchLimit; // if the character is proned, we need to limit their max pitch, no one on their right mind would do yoga 
	PC->PlayerCameraManager->ViewPitchMin = -Limit;
	PC->PlayerCameraManager->ViewPitchMax = Limit;
}
void UPlayerCameraComponent::ResetCameraManagerLimits()
{
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager)) return;

	PC->PlayerCameraManager->ViewPitchMin = -89.9f;
	PC->PlayerCameraManager->ViewPitchMax = 89.9f;
}

// ── Initialization ────────────────────────────────────────────────────────────

void UPlayerCameraComponent::SetReference()
{
	Character = Cast<ACharacter>(GetOwner());
	if (IsValid(Character))
		CharacterMovement = Character->GetCharacterMovement();
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
	// Override in Blueprint to assign GameplayCamera asset after creation
}

// springarm camera initialization
void UPlayerCameraComponent::CreateCamera()
{
	SpringArmRef = NewObject<USpringArmComponent>(GetOwner(), TEXT("CameraBoom"));
	SpringArmRef->AttachToComponent(
		Character->GetRootComponent(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SpringArmRef->RegisterComponent(); // first create the springarm

	CameraRef = NewObject<UCameraComponent>(GetOwner(), TEXT("Camera"));
	CameraRef->AttachToComponent(
		SpringArmRef,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("SpringEndPoint"));
	CameraRef->RegisterComponent(); // then create the camera, both of these are essential for third persion POV

	// FPS camera — attaches to head socket, inactive by default, so we dont necessarily need a springarm
	FPSCameraRef = NewObject<UCameraComponent>(GetOwner(), TEXT("FPSCamera"));
	FPSCameraRef->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("head"));
	FPSCameraRef->RegisterComponent();
	FPSCameraRef->bAutoActivate = false;
	FPSCameraRef->bUsePawnControlRotation = true;
}
void UPlayerCameraComponent::InitializeCamera()
{
	if (!IsValid(SpringArmRef) || !IsValid(CameraRef)) return;
	// give some default believable values for the springarm camera to spawn in the correct place
	SpringArmRef->SetRelativeLocation(SpringArmLocation);
	SpringArmRef->SetRelativeRotation(SpringArmRotation);
	SpringArmRef->bUsePawnControlRotation = true;
	SpringArmRef->TargetArmLength = TargetArmLength;
	SpringArmRef->bEnableCameraLag = true;
	SpringArmRef->bEnableCameraRotationLag = true;
	SpringArmRef->CameraLagSpeed = LagSpeed;
	SpringArmRef->CameraRotationLagSpeed = RotationLagSpeed;
	SpringArmRef->bDoCollisionTest = true;

	CameraRef->FieldOfView = FieldOfView;
	CameraRef->bAutoActivate = true;

	// prevents pop on first frame, we dont explicitly set positions to the camera. rather we set a target point and in the tick we slowly interpolate the current position to the target positon
	CurrentBase.TargetArmLength = TargetArmLength;
	CurrentBase.PivotOffset = SpringArmLocation;
	CurrentBase.FieldOfView = FieldOfView;
	CurrentBase.LagSpeed = LagSpeed;
	CurrentBase.RotationLagSpeed = RotationLagSpeed;

	// forces PSO compilation during init rather than on first FPS switch, if you dont use it then on the very first time you turn on fps camera, the game freezes for a few seconds
	if (IsValid(FPSCameraRef))
	{
		FPSCameraRef->Activate(true);
		FPSCameraRef->Deactivate();
	}

	// Pre-evaluate spring arm to eliminate deferred init cost on first FPS switch
	SpringArmRef->TargetArmLength = TargetArmLength;
	SpringArmRef->SocketOffset = FVector::ZeroVector;
}

// gameplay camera initialization
void UPlayerCameraComponent::CreateGameplayCamera()
{
	if (!IsValid(GameplayCameraRef))
	{
		GameplayCameraRef = NewObject<UGameplayCameraComponent>(GetOwner(), TEXT("GameplayCamera"));
		GameplayCameraRef->bAutoActivate = false;
	}
	GameplayCameraRef->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GameplayCameraRef->RegisterComponent();
	PrimaryComponentTick.bCanEverTick = false; // we dont need to uselessly run tick for gameplay camera system , it has its own built in ticking
	SetCameraReference();
}
void UPlayerCameraComponent::InitializeGameplayCamera()
{
	if (!IsValid(GameplayCameraRef)) return;
	GameplayCameraRef->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	GameplayCameraRef->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	GameplayCameraRef->ActivateCameraForPlayerController(
		Cast<APlayerController>(Character->GetController()));
}