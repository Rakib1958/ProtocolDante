#include "MMAnimInstance.h"

void UMMAnimInstance::UpdateTrajectory(float DeltaTime)
{
    // Routes cleanly inside the thread worker pool based on your BP native control check
    if (IsPlayerControlledCharacter())
    {
        GeneratePlayerTrajectory(DeltaTime);
    }
    else
    {
        Trajectory = GenerateNPCTrajectory(30, 0.1f, 15, DeltaTime);
        ApplyCollisionToTrajectoryWhenAir();
    }
}

void UMMAnimInstance::GeneratePlayerTrajectory(float DeltaTime)
{
    const float CachedSpeed2D = GetThreadSafeSpeed2D();
    const FPoseSearchTrajectoryData& ConfigData = (CachedSpeed2D > 0.0f) ? TrajectoryGenerationDataMoving : TrajectoryGenerationDataIdle;

    FTransformTrajectory GeneratedTrajectory;
    UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
        this,
        ConfigData,
        DeltaTime,
        Trajectory,
        PreviousDesiredControllerYaw,
        GeneratedTrajectory,
        -1.0f, // InHistorySamplingInterval (Auto-calculate)
        30,    // InTrajectoryHistoryCount
        0.1f,  // InPredictionSamplingInterval
        15     // InTrajectoryPredictionCount
    );

    // Pull combat rules safely from property access interfaces
    if (GetThreadSafeRotationMode() == Enum_RotationMode::LookingDirection &&
        GetThreadSafeGait() != Enum_Gait::Sprint &&
        GetThreadSafeCharacterState() == Enum_CharacterState::Combat)
    {
        OverrideTrajectoryFacing(GeneratedTrajectory, GetThreadSafeCombatRotation(), 1.0f, true);
    }

    FTransformTrajectory CollidedTrajectory;
    FPoseSearchTrajectory_WorldCollisionResults CollisionResults;
    TArray<AActor*> ActorsToIgnore;

    if (AActor* OwningPawn = TryGetPawnOwner())
    {
        ActorsToIgnore.Add(OwningPawn);
    }

    UPoseSearchTrajectoryLibrary::HandleTransformTrajectoryWorldCollisions(
        this, this, GeneratedTrajectory, true, 0.01f,
        CollidedTrajectory, CollisionResults, ETraceTypeQuery::TraceTypeQuery1,
        false, ActorsToIgnore, EDrawDebugTrace::None, true, 150.0f
    );

    Trajectory = CollidedTrajectory;
    TrajectoryCollision = CollisionResults;

    // UE 5.8 Static Reference Signature Parsing
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, -0.3f, -0.2f, TrjPastVelocity, false);
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.0f, 0.2f, TrjCurrentVelocity, false);
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.4f, 0.5f, TrjFutureVelocity, false);
}

FTransformTrajectory UMMAnimInstance::GenerateNPCTrajectory(int32 HistoryCount, float InPredictionInterval, int32 PredictionCount, float DeltaTime)
{
    const float CachedSpeed2D = GetThreadSafeSpeed2D();
    FPoseSearchTrajectoryData ConfigData = (CachedSpeed2D > 1.0f) ? TrajectoryGenerationDataMoving : TrajectoryGenerationDataIdle;

    FTransformTrajectory GeneratedTrajectory;
    UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
        this, ConfigData, DeltaTime, Trajectory, PreviousDesiredControllerYaw,
        GeneratedTrajectory, -1.0f, HistoryCount, InPredictionInterval, PredictionCount
    );

    Trajectory = GeneratedTrajectory;

    // UE 5.8 Static Reference Out-Param Sampling
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.4f, 0.5f, TrjFutureVelocity, false);

    return Trajectory;
}

void UMMAnimInstance::ApplyCollisionToTrajectoryWhenAir()
{
    const FVector CachedVelocity = GetThreadSafeVelocity();

    if (GetThreadSafeMovementMode() == Enum_MovementMode::InAir && CachedVelocity.Z < 0.0f)
    {
        FTransformTrajectory CollidedTrajectory;
        FPoseSearchTrajectory_WorldCollisionResults CollisionResults;
        TArray<AActor*> ActorsToIgnore;

        if (AActor* OwningPawn = TryGetPawnOwner())
        {
            ActorsToIgnore.Add(OwningPawn);
        }

        UPoseSearchTrajectoryLibrary::HandleTransformTrajectoryWorldCollisions(
            this, this, Trajectory, true, 0.02f,
            CollidedTrajectory, CollisionResults, ETraceTypeQuery::TraceTypeQuery1,
            false, ActorsToIgnore, EDrawDebugTrace::None, true, 2000.0f
        );

        Trajectory = CollidedTrajectory;
    }
}

void UMMAnimInstance::OverrideTrajectoryFacing(FTransformTrajectory& InTrajectoryData, const FRotator& InFacing, float NewFacingAlpha, bool bIncludeHistory)
{
    const FQuat FacingQuat = InFacing.Quaternion();

    for (int32 i = 0; i < InTrajectoryData.Samples.Num(); ++i)
    {
        FTransformTrajectorySample& Sample = InTrajectoryData.Samples[i];

        if (!bIncludeHistory && Sample.TimeInSeconds < 0.0f)
        {
            continue;
        }

        Sample.Facing = FQuat::Slerp(Sample.Facing, FacingQuat, NewFacingAlpha);
    }
}