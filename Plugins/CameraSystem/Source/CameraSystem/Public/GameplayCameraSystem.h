#pragma once
#include "UObject/Interface.h"
#include "CoreMinimal.h"
#include "GameplayCameraTags.h"
#include "GameplayCameraSystem.generated.h"

USTRUCT(BlueprintType)
struct FStruct_GameplayCamera
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CameraStyle = GameplayCameraTags::Style_Balanced;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CameraMode = GameplayCameraTags::Mode_Freecam;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CameraView = GameplayCameraTags::View_Third;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CameraShoulder = GameplayCameraTags::Shoulder_Right;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Stance;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UGameplayCameraSystem : public UInterface
{
	GENERATED_BODY()
};


class CAMERASYSTEM_API IGameplayCameraSystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gameplay Camera")
	FStruct_GameplayCamera GetCharacterPropertiesFromCamera();

};
