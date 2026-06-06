#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Goal_Base.generated.h"

class UBlackboardComponent;

UCLASS(Blueprintable, EditInlineNew, Abstract)
class LOCOMOTIONSYSTEM_API UGoal_Base : public UObject
{
	GENERATED_BODY()

public:
	// Fallback tracking baseline if no custom scoring overrides are written
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Goal Config")
	float BaseScore = 1.0f;

	// Rule to clear this token out of the Action Component pool when finished
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal Config")
	bool bRemoveOnSuccess = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal Config")
	bool bRemoveOnFailure = true;

	// Hook to pass dynamic contextual values directly into the current Blackboard layout
	UFUNCTION(BlueprintImplementableEvent, Category = "Goal Logic")
	void OnGoalActivated(AActor* OwnerActor, UBlackboardComponent* BlackboardComp);

	// Mathematical Utility Function to evaluate current relative priority weights
	UFUNCTION(BlueprintNativeEvent, Category = "Goal Logic")
	float ScoreGoal(AActor* OwnerActor) const;
	virtual float ScoreGoal_Implementation(AActor* OwnerActor) const { return BaseScore; }

	UFUNCTION(BlueprintNativeEvent, Category = "Goal Logic")
	bool IsGoalValid(AActor* OwnerActor) const;
	virtual bool IsGoalValid_Implementation(AActor* OwnerActor) const { return true; }
};