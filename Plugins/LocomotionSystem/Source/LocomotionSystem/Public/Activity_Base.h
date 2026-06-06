#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Activity_Base.generated.h"

class UGoal_Base;
class UBlackboardComponent;

UCLASS(Blueprintable, Abstract, EditInlineNew)
class LOCOMOTIONSYSTEM_API UActivity_Base : public UObject
{
	GENERATED_BODY()
	
public:
	// The static category type of Goal this activity is designed to satisfy
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Core")
	TSubclassOf<UGoal_Base> SupportedGoalClass = nullptr;

	// The primary asset defining the underlying state task structure
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Core")
	UBehaviorTree* ActivityBehaviorTree = nullptr;

	// Guard flag dictating whether higher utility goals can abort this task mid-execution
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Core")
	bool bIsInterruptible = true;

	// Baseline weight modifier used during scoring assessment routines
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activity Balancing")
	float ActivityWeightModifier = 1.0f;

	// --- THE OVERRIDABLE NARRATIVE SYSTEM CLONES ---

	UFUNCTION(BlueprintNativeEvent, Category = "Activity Lifecycle")
	bool CanRunActivity(AActor* OwnerActor, const UGoal_Base* AssociatedGoal) const;
	virtual bool CanRunActivity_Implementation(AActor* OwnerActor, const UGoal_Base* AssociatedGoal) const { return true; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Activity Lifecycle")
	void RunActivity(AActor* OwnerActor, UGoal_Base* AssociatedGoal);

	UFUNCTION(BlueprintNativeEvent, Category = "Activity Lifecycle")
	FString DescribeActivity() const;
	virtual FString DescribeActivity_Implementation() const { return GetClass()->GetName(); }

	UFUNCTION(BlueprintImplementableEvent, Category = "Activity Lifecycle")
	void EndActivity(AActor* OwnerActor, UGoal_Base* AssociatedGoal, bool bWasSuccessful);

	UFUNCTION(BlueprintNativeEvent, Category = "Activity Lifecycle")
	float ScoreActivity(AActor* OwnerActor, const UGoal_Base* AssociatedGoal) const;
	virtual float ScoreActivity_Implementation(AActor* OwnerActor, const UGoal_Base* AssociatedGoal) const { return ActivityWeightModifier; }

	UFUNCTION(BlueprintNativeEvent, Category = "Activity Lifecycle")
	float ScoreGoalItem(AActor* OwnerActor, const UGoal_Base* TargetGoal) const;
	virtual float ScoreGoalItem_Implementation(AActor* OwnerActor, const UGoal_Base* TargetGoal) const { return 0.0f; }

	// Use this hook to perform explicit blackboard value injections (Targets, Vectors, State flags)
	UFUNCTION(BlueprintNativeEvent, Category = "Activity Lifecycle")
	void SetupBlackBoard(UBlackboardComponent* BlackboardComp, UGoal_Base* AssociatedGoal);
	virtual void SetupBlackBoard_Implementation(UBlackboardComponent* BlackboardComp, UGoal_Base* AssociatedGoal) {}
};
