#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerCameraComponent.h"
#include "GameplayCameraInterface.generated.h"
/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UGameplayCameraInterface : public UInterface
{
	GENERATED_BODY()
};

class CAMERASYSTEM_API IGameplayCameraInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Gameplay Camera")
	FStruct_GameplayCamera GetCharacterPropertiesForCamera();
};
