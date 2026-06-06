#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Activity_Base.h"
#include "Goal_Base.h"
#include "AI_Configuration.h"
#include "ActionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGoalExecutionSwapped, UGoal_Base*, NewActiveGoal, UActivity_Base*, ActiveActivity, bool, bInterruptibleState);

UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class LOCOMOTIONSYSTEM_API UActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActionComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Action Pool")
	TArray<UGoal_Base*> ActiveGoals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action Pool")
	UAI_Configuration* ActivityConfiguration = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "Action Pool")
	FOnGoalExecutionSwapped OnGoalExecutionSwapped;

	UFUNCTION(BlueprintCallable, Category = "Action Pool")
	void AddGoal(UGoal_Base* NewGoal);

	UFUNCTION(BlueprintCallable, Category = "Action Pool")
	void RemoveGoal(UGoal_Base* GoalToRemove);

	UFUNCTION(BlueprintCallable, Category = "Action Pool")
	void EvaluateHighestBidder();

	UFUNCTION(BlueprintPure, Category = "Action Pool")
	UActivity_Base* FindBestActivityForGoal(UGoal_Base* Goal, float& OutCombinedScore);

	// --- EXPLICIT SAVE/LOAD COMPONENT HELPERS ---

	/** Flushes all running states instantly; critical call for the start of Easy Game UI's loading delegate */
	UFUNCTION(BlueprintCallable, Category = "Action Pool | Serialization")
	void ForceResetComponentState();

	/** Instantly prompts the evaluation pool to calculate optimal behaviors after variables reload */
	UFUNCTION(BlueprintCallable, Category = "Action Pool | Serialization")
	void ReevaluateStatePostLoad();

private:
	UPROPERTY()
	UGoal_Base* CurrentActiveGoal = nullptr;

	// CRITICAL FIX: Track active activity to support lifecycle EndActivity calls and interruption locks
	UPROPERTY()
	UActivity_Base* CurrentActiveActivity = nullptr;

	UPROPERTY()
	TArray<UActivity_Base*> InstantiatedActivities;

	UPROPERTY()
	TArray<UGoal_Generator*> InstantiatedGenerators;

	FTimerHandle RescoreTimerHandle;

	void RunGoalGenerationPass();
};