// ActivityComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCDefinition.h"
#include "GoalItem.h"
#include "ActivityBase.h"
#include "NPCTypes.h"
#include "ActivityComponent.generated.h"

class AAIC_NPCController;

UCLASS(ClassGroup = "NPC", meta = (BlueprintSpawnableComponent))
class NPCCORE_API UActivityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActivityComponent();

    // ── Configuration ────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
    TObjectPtr<UNPCDefinition> NPCDefinition;

    // Controls whether tick-based rescore is active
    // Toggle via significance tier through Blueprint
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Configuration")
    bool bRescoringEnabled = true;

    // ── State ────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<UActivityBase> ActiveActivity;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    TArray<TObjectPtr<UGoalItem>> GoalList;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    EAlertState CurrentAlertState = EAlertState::Unaware;

    // ── Delegates ────────────────────────────────────────────────────────

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActivityChanged,
        UActivityBase*, OldActivity,
        UActivityBase*, NewActivity);

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnActivityChanged OnActivityChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlertStateChanged,
        EAlertState, NewState);

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAlertStateChanged OnAlertStateChanged;

    // ── Public API ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void Initialize();

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void TriggerGoalGenerator(
        TSubclassOf<UGoalGeneratorBase> GeneratorClass,
        AActor* Source);

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void AddGoal(TSubclassOf<UGoalItem> GoalClass, AActor* Source);

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void RemoveGoal(UGoalItem* Goal, EGoalRemoveReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void ClearAllGoals(EGoalRemoveReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void SetAlertState(EAlertState NewState);

    // Called by Blueprint hub when significance tier changes
    UFUNCTION(BlueprintCallable, Category = "Activity")
    void SetRescoringInterval(float NewInterval);

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void RequestImmediateRescore();

    UFUNCTION(BlueprintCallable, Category = "Activity")
    void OnOwnerDied();

    // Accessors
    UFUNCTION(BlueprintPure, Category = "Activity")
    UActivityBase* GetActiveActivity() const { return ActiveActivity; }

    UFUNCTION(BlueprintPure, Category = "Activity")
    EAlertState GetAlertState() const { return CurrentAlertState; }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ── Cached refs ──────────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<AAIC_NPCController> CachedController;

    // ── Rescore state ────────────────────────────────────────────────────

    float RescoringInterval = 0.3f;
    float RescoringTimer = 0.f;
    bool  bNeedsImmediateRescore = false;
    bool  bIsDead = false;

    // ── Internal ─────────────────────────────────────────────────────────

    void                Rescore();
    UActivityBase* SelectBestActivity();
    void                SwitchToActivity(UActivityBase* NewActivity);
    void                CleanupExpiredGoals(float DeltaTime);
    UGoalItem* FindExistingGoal(
        TSubclassOf<UGoalItem> GoalClass) const;
    AAIC_NPCController* GetCachedController();
};