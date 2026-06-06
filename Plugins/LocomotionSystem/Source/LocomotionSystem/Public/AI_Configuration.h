#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Activity_Base.h"
#include "AI_Configuration.generated.h"

class UGoal_Generator;

UCLASS(BlueprintType)
class LOCOMOTIONSYSTEM_API UAI_Configuration : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing Settings")
	float RescoreInterval = 0.5f;

	// Scriptable behavioral capabilities assigned to this NPC profile blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capabilities", meta = (AssetBundles = "Client"))
	TArray<TSubclassOf<UActivity_Base>> ActivityCapabilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generators")
	TArray<TSubclassOf<UGoal_Generator>> GoalGenerators;
};
