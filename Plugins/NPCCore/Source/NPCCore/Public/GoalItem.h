// GoalItem.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "NPCTypes.h"
#include "GoalItem.generated.h"

class UActivityComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew)
class NPCCORE_API UGoalItem : public UObject
{
    GENERATED_BODY()

public:
    // ── Configuration ────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
    EGoalPriority Priority = EGoalPriority::Normal;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
    float DefaultScore = 1.0f;

    // -1 = never expires by time, rely on ExpiryCondition only
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
    float GoalLifetime = -1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
    bool bRemoveOnSucceeded = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal")
    FGameplayTagContainer GoalTags;

    // ── Runtime ──────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadWrite, Category = "Goal")
    TWeakObjectPtr<AActor> SourceActor;

    float ElapsedLifetime = 0.f;

    // ── API ──────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Goal")
    float GetGoalScore() const;
    virtual float GetGoalScore_Implementation() const
    {
        return DefaultScore;
    }

    // Return true to remove this goal immediately
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Goal")
    bool ExpiryCondition() const;
    virtual bool ExpiryCondition_Implementation() const
    {
        return false;
    }

    UFUNCTION(BlueprintNativeEvent, Category = "Goal")
    void Initialize(UActivityComponent* OwnerComp);
    virtual void Initialize_Implementation(UActivityComponent* OwnerComp)
    {
    }

    UFUNCTION(BlueprintNativeEvent, Category = "Goal")
    void OnRemoved(EGoalRemoveReason Reason);
    virtual void OnRemoved_Implementation(EGoalRemoveReason Reason)
    {
    }

    // ── Internal helpers ─────────────────────────────────────────────────

    bool HasExpired() const
    {
        if (GoalLifetime > 0.f && ElapsedLifetime >= GoalLifetime)
            return true;
        return ExpiryCondition();
    }

    void TickLifetime(float DeltaTime)
    {
        if (GoalLifetime > 0.f)
            ElapsedLifetime += DeltaTime;
    }

    void ResetLifetime()
    {
        ElapsedLifetime = 0.f;
    }
};