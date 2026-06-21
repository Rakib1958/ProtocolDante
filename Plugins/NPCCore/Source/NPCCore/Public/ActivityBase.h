// ActivityBase.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GoalItem.h"
#include "ActivityBase.generated.h"

class UActivityComponent;
class UBehaviorTree;
class UBlackboardData;
class UBlackboardComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew)
class NPCCORE_API UActivityBase : public UObject
{
    GENERATED_BODY()

public:
    // ── Configuration ────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity")
    TSubclassOf<UGoalItem> CompatibleGoalType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity")
    TObjectPtr<UBehaviorTree> BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity")
    TObjectPtr<UBlackboardData> BlackboardData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity")
    bool bIsInterruptible = true;

    // ── Scoring ──────────────────────────────────────────────────────────

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Activity")
    bool CanActivate(UActivityComponent* Context) const;
    virtual bool CanActivate_Implementation(
        UActivityComponent* Context) const
    {
        return true;
    }

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Activity")
    float ScoreActivity(UActivityComponent* Context) const;
    virtual float ScoreActivity_Implementation(
        UActivityComponent* Context) const
    {
        return 0.5f;
    }

    // ── Lifecycle ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintNativeEvent, Category = "Activity")
    void SetupBlackboard(
        UBlackboardComponent* BB,
        UActivityComponent* Context);
    virtual void SetupBlackboard_Implementation(
        UBlackboardComponent* BB,
        UActivityComponent* Context)
    {
    }

    UFUNCTION(BlueprintNativeEvent, Category = "Activity")
    void OnActivityStarted(UActivityComponent* Context);
    virtual void OnActivityStarted_Implementation(
        UActivityComponent* Context)
    {
    }

    UFUNCTION(BlueprintNativeEvent, Category = "Activity")
    void OnActivityEnded(UActivityComponent* Context);
    virtual void OnActivityEnded_Implementation(
        UActivityComponent* Context)
    {
    }

    // ── Helper ───────────────────────────────────────────────────────────

    UGoalItem* FindBestCompatibleGoal(
        const TArray<UGoalItem*>& GoalList) const
    {
        if (!CompatibleGoalType) return nullptr;

        UGoalItem* Best = nullptr;
        float       BestScore = -1.f;

        for (UGoalItem* Goal : GoalList)
        {
            if (!Goal || !Goal->IsA(CompatibleGoalType)) continue;

            float Score = Goal->GetGoalScore();
            if (Score > BestScore)
            {
                BestScore = Score;
                Best = Goal;
            }
        }
        return Best;
    }
};