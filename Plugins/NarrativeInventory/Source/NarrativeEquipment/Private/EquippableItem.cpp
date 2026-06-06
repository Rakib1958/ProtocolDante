// Copyright Narrative Tools 2025. 

#include "EquippableItem.h"
#include "EquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkinnedAssetCommon.h"

#define LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem()
{
	UseActionText = LOCTEXT("UseActionText_Equippable", "Equip");
	bStackable = false;
	bCanActivate = true;
	bToggleActiveOnUse = true;
	Weight = 1.f;
}


void UEquippableItem::HandleEquip_Implementation()
{

}

void UEquippableItem::HandleUnequip_Implementation()
{

}

void UEquippableItem::Deactivated_Implementation()
{
	if (GetOwningPawn())
	{
		UseActionText = LOCTEXT("EquipText", "Equip");

		if (UEquipmentComponent* EquipmentComponent = Cast<UEquipmentComponent>(GetOwningPawn()->GetComponentByClass(UEquipmentComponent::StaticClass())))
		{
			EquipmentComponent->UnequipItem(this);
		}
	}
}

void UEquippableItem::Activated_Implementation()
{
	if (GetOwningPawn())
	{
		UseActionText = LOCTEXT("UnequipText", "Unequip");

		if (UEquipmentComponent* EquipmentComponent = Cast<UEquipmentComponent>(GetOwningPawn()->GetComponentByClass(UEquipmentComponent::StaticClass())))
		{
			EquipmentComponent->EquipItem(this);
		}
	}
}

UEquippableItem_Clothing::UEquippableItem_Clothing()
{
	bShouldSetLeaderPose = true;
}

#if WITH_EDITOR
void UEquippableItem_Clothing::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UEquippableItem_Clothing, ClothingMesh))
	{
		if (ClothingMesh)
		{
			ClothingMaterials.Empty();

			for (auto& ClothingMeshMat : ClothingMesh->GetMaterials())
			{
				ClothingMaterials.Add(ClothingMeshMat.MaterialInterface);
			}
		}
	}
}
#endif

void UEquippableItem_Clothing::HandleUnequip_Implementation()
{
	if (UEquipmentComponent* EquipmentComponent = Cast<UEquipmentComponent>(GetOwningPawn()->GetComponentByClass(UEquipmentComponent::StaticClass())))
	{
		if (EquipmentComponent->EquippableComponents.Contains(EquippableSlot) && EquipmentComponent->DefaultClothing.Contains(EquippableSlot) && EquipmentComponent->DefaultClothingMaterials.Contains(EquippableSlot))
		{
			//Set the clothing back to its default mesh and materials
			if (USkeletalMeshComponent* SlotComponent = *EquipmentComponent->EquippableComponents.Find(EquippableSlot))
			{
				if (USkeletalMesh* DefaultClothingMesh = *EquipmentComponent->DefaultClothing.Find(EquippableSlot))
				{
					SlotComponent->SetSkeletalMesh(DefaultClothingMesh);

					if (FDefaultClothingMeshMaterials* DefaultMaterials = EquipmentComponent->DefaultClothingMaterials.Find(EquippableSlot))
					{
						int32 Idx = 0;

						for (auto& DefaultMat : DefaultMaterials->Materials)
						{
							SlotComponent->SetMaterial(Idx, DefaultMat);
							++Idx;
						}
					}
				}
				else 
				{
					SlotComponent->SetSkeletalMesh(nullptr);
				}

				if(EquipmentComponent->LeaderPoseComponent)
				{
					SlotComponent->SetLeaderPoseComponent(EquipmentComponent->LeaderPoseComponent);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Narrative Equipment tried unequipping item but the Equipment Component doesn't have a skeletal mesh component added. Have you called initialize?"), *GetNameSafe(this));
		}

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Narrative Equipment tried unequipping your item, but your pawn doesn't have an Equipment Component added. Please add one."));
	}
}

void UEquippableItem_Clothing::HandleEquip_Implementation()
{
	if (UEquipmentComponent* EquipmentComponent = Cast<UEquipmentComponent>(GetOwningPawn()->GetComponentByClass(UEquipmentComponent::StaticClass())))
	{
		if (ClothingMesh)
		{
			if (EquipmentComponent->EquippableComponents.Contains(EquippableSlot))
			{
				//Set the clothing mesh to the new mesh
				if (USkeletalMeshComponent* SlotComponent = *EquipmentComponent->EquippableComponents.Find(EquippableSlot))
				{
					SlotComponent->SetSkeletalMesh(ClothingMesh);

					int32 Idx = 0;
					for (auto& Mat : ClothingMaterials)
					{
						SlotComponent->SetMaterial(Idx, Mat);
						++Idx;
					}

					if (EquipmentComponent->LeaderPoseComponent)
					{
						if (bShouldSetLeaderPose)
						{
							SlotComponent->SetLeaderPoseComponent(EquipmentComponent->LeaderPoseComponent);
						}
						else
						{
							SlotComponent->SetLeaderPoseComponent(nullptr);
						}
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Narrative Equipment tried equipping %s but the Equipment Component doesn't have a skeletal mesh component added. Have you called Initialize?"), *GetNameSafe(this));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Narrative Equipment tried putting an item on, but your pawn doesn't have an Equipment Component added. Please add one."));
	}
}

#undef LOCTEXT_NAMESPACE