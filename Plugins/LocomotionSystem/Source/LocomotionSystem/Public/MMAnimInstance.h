#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Animation/TrajectoryTypes.h"
#include "LocomotionComponent.h"
#include "MMAnimInstance.generated.h"

UCLASS()
class LOCOMOTIONSYSTEM_API UMMAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

protected:
    // --- Trajectory Configurations & Buffers ---
    UPROPERTY(EditDefaultsOnly, Category = "Motion Matching|Trajectory")
    FPoseSearchTrajectoryData TrajectoryGenerationDataIdle;

    UPROPERTY(EditDefaultsOnly, Category = "Motion Matching|Trajectory")
    FPoseSearchTrajectoryData TrajectoryGenerationDataMoving;

    UPROPERTY(BlueprintReadOnly, Category = "Motion Matching|Trajectory")
    FTransformTrajectory Trajectory;

    UPROPERTY(BlueprintReadOnly, Category = "Motion Matching|Trajectory")
    FPoseSearchTrajectory_WorldCollisionResults TrajectoryCollision;

    UPROPERTY(BlueprintReadWrite, Category = "Motion Matching|Trajectory")
    float PreviousDesiredControllerYaw = 0.f;

    // --- Extracted Velocities ---
    UPROPERTY(BlueprintReadOnly, Category = "Motion Matching|Velocity") FVector TrjPastVelocity;
    UPROPERTY(BlueprintReadOnly, Category = "Motion Matching|Velocity") FVector TrjCurrentVelocity;
    UPROPERTY(BlueprintReadOnly, Category = "Motion Matching|Velocity") FVector TrjFutureVelocity;

    // Set by SignificanceComponent — ABP can skip expensive operations
    UPROPERTY(BlueprintReadWrite, Category = "LOD")
    int32 CurrentSignificanceTier = 2;

    // --- Thread-Safe Core Trajectory Math APIs ---
    UFUNCTION(BlueprintCallable, Category = "Motion Matching|Trajectory", meta = (BlueprintThreadSafe))
    void UpdateTrajectory(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Motion Matching|Trajectory", meta = (BlueprintThreadSafe))
    void GeneratePlayerTrajectory(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Motion Matching|Trajectory", meta = (BlueprintThreadSafe))
    FTransformTrajectory GenerateNPCTrajectory(int32 HistoryCount, float InPredictionInterval, int32 PredictionCount, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Motion Matching|Trajectory", meta = (BlueprintThreadSafe))
    void ApplyCollisionToTrajectoryWhenAir();

    UFUNCTION(BlueprintCallable, Category = "Motion Matching|Trajectory", meta = (BlueprintThreadSafe))
    void OverrideTrajectoryFacing(UPARAM(ref) FTransformTrajectory& InTrajectoryData, const FRotator& InFacing, float NewFacingAlpha, bool bIncludeHistory);

    // ── Thread-Safe Property Access Hooks (BlueprintNativeEvents) ──────────────────────

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    bool IsPlayerControlledCharacter() const;
    virtual bool IsPlayerControlledCharacter_Implementation() const { return false; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    float GetThreadSafeSpeed2D() const;
    virtual float GetThreadSafeSpeed2D_Implementation() const { return 0.f; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    Enum_RotationMode GetThreadSafeRotationMode() const;
    virtual Enum_RotationMode GetThreadSafeRotationMode_Implementation() const { return Enum_RotationMode::LookingDirection; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    Enum_Gait GetThreadSafeGait() const;
    virtual Enum_Gait GetThreadSafeGait_Implementation() const { return Enum_Gait::Run; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    Enum_CharacterState GetThreadSafeCharacterState() const;
    virtual Enum_CharacterState GetThreadSafeCharacterState_Implementation() const { return Enum_CharacterState::Relaxed; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    FRotator GetThreadSafeCombatRotation() const;
    virtual FRotator GetThreadSafeCombatRotation_Implementation() const { return FRotator::ZeroRotator; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    Enum_MovementMode GetThreadSafeMovementMode() const;
    virtual Enum_MovementMode GetThreadSafeMovementMode_Implementation() const { return Enum_MovementMode::OnGround; }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Motion Matching|Inputs", meta = (BlueprintThreadSafe))
    FVector GetThreadSafeVelocity() const;
    virtual FVector GetThreadSafeVelocity_Implementation() const { return FVector::ZeroVector; }
};