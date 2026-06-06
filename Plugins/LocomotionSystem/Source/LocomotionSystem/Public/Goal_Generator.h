#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Goal_Generator.generated.h"

class UActionComponent;

UCLASS(Blueprintable, Abstract, EditInlineNew)
class LOCOMOTIONSYSTEM_API UGoal_Generator : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generator Rules")
	float CooldownDuration = 5.0f;

	// Checks conditions periodically and builds/injects goal instances into the pool
	UFUNCTION(BlueprintNativeEvent, Category = "Generation Pass")
	void EvaluateAndGenerateGoals(AActor* OwnerActor, UActionComponent* ActionComp);
	virtual void EvaluateAndGenerateGoals_Implementation(AActor* OwnerActor, UActionComponent* ActionComp) {}

	UFUNCTION(BlueprintCallable, Category = "Generator State")
	bool IsOnCooldown(UWorld* WorldContext) const;

	UFUNCTION(BlueprintCallable, Category = "Generator State")
	void TriggerCooldown(UWorld* WorldContext);

private:
	float LastExecutionTime = -999.0f;
};
