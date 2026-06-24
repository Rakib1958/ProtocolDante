#include "AC_NPC_Clothing.h"
#include "Engine/AssetManager.h"
#include "Animation/AnimBlueprint.h" 
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

UAC_NPC_Clothing::UAC_NPC_Clothing()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_NPC_Clothing::InitializeClothing(bool bWasLooted)
{
    if (!ClothingDefinition) return;

    //ClothingDefinition = Definition;
    bUniformLooted = bWasLooted;

    ResolveMeshComponents();
    SetAllMeshesVisible(false);
    StartAsyncLoad();
}

void UAC_NPC_Clothing::ResolveMeshComponents()
{
    BodyMesh = FindMeshByTag(FName("Body"));
    UniformMesh = FindMeshByTag(FName("Uniform"));

    FollowerMeshes.Reset();

    if (USkeletalMeshComponent* Head = FindMeshByTag(FName("Head")))
        FollowerMeshes.Add(Head);
    if (USkeletalMeshComponent* Hair = FindMeshByTag(FName("Hair")))
        FollowerMeshes.Add(Hair);

    if (!bUniformLooted && UniformMesh)
        FollowerMeshes.Add(UniformMesh);
}

USkeletalMeshComponent* UAC_NPC_Clothing::FindMeshByTag(FName Tag) const
{
    if (!GetOwner()) return nullptr;

    TArray<UActorComponent*> Found = GetOwner()->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), Tag);
    return Found.IsEmpty() ? nullptr : Cast<USkeletalMeshComponent>(Found[0]);
}

void UAC_NPC_Clothing::StartAsyncLoad()
{
    if (!ClothingDefinition) return;

    TArray<FSoftObjectPath> AssetsToLoad;

    for (auto& Pair : ClothingDefinition->MeshParts)
    {
        if (Pair.Key == EBodyPart::Uniform && bUniformLooted) continue;

        if (!Pair.Value.MeshAsset.IsNull())
            AssetsToLoad.Add(Pair.Value.MeshAsset.ToSoftObjectPath());
    }

    TSoftObjectPtr<UAnimBlueprint> RetargetABP =
        ClothingDefinition->SkeletonType == ESkeletonType::UE4Mannequin
        ? ClothingDefinition->RetargetABP_UE4
        : ClothingDefinition->RetargetABP_UE5;

    if (!RetargetABP.IsNull())
        AssetsToLoad.Add(RetargetABP.ToSoftObjectPath());

    if (AssetsToLoad.IsEmpty())
    {
        OnAllMeshesLoaded();
        return;
    }

    AsyncLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &UAC_NPC_Clothing::OnAllMeshesLoaded));
}

void UAC_NPC_Clothing::OnAllMeshesLoaded()
{
    if (!ClothingDefinition) return;
    if (!BodyMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("AC_NPC_Clothing: BodyMesh is null on %s — check Body tag is set"),
            *GetOwner()->GetName());
        return;
    }

    UAnimBlueprint* RawRetargetABP = ClothingDefinition->SkeletonType == ESkeletonType::UE4Mannequin
        ? ClothingDefinition->RetargetABP_UE4.Get()
        : ClothingDefinition->RetargetABP_UE5.Get();

    for (auto& Pair : ClothingDefinition->MeshParts)
    {
        if (Pair.Key == EBodyPart::Uniform && bUniformLooted) continue;

        USkeletalMesh* LoadedMesh = Pair.Value.MeshAsset.Get();
        if (!LoadedMesh) continue;

        static const TMap<EBodyPart, FName> PartToTag =
        {
            { EBodyPart::Body,    FName("Body")    },
            { EBodyPart::Head,    FName("Head")    },
            { EBodyPart::Hair,    FName("Hair")    },
            { EBodyPart::Uniform, FName("Uniform") }
        };

        const FName* Tag = PartToTag.Find(Pair.Key);
        if (!Tag) continue;

        USkeletalMeshComponent* Comp = FindMeshByTag(*Tag);
        if (!Comp) continue;

        bool bIsBody = (Pair.Key == EBodyPart::Body);
        //bool bIsUniform = (Pair.Key == EBodyPart::Uniform);

        ApplyMeshToComponent(
            Comp,
            LoadedMesh,
            bIsBody ? nullptr : RawRetargetABP,
            Pair.Value.MorphTargets);

        if (!bIsBody && BodyMesh)
            Comp->SetLeaderPoseComponent(BodyMesh);
        //if (!bIsBody)
        //{
        //    Comp->SetLeaderPoseComponent(nullptr);
        //}
    }

    SetAllMeshesVisible(true);
    bClothingLoaded = true;
    OnClothingLoaded.Broadcast();
}

void UAC_NPC_Clothing::ApplyMeshToComponent(USkeletalMeshComponent* Comp, USkeletalMesh* Mesh, UAnimBlueprint* RetargetABP, const TMap<FName, float>& Morphs)
{
    if (!Comp || !Mesh) return;

    Comp->SetSkeletalMesh(Mesh);

    if (RetargetABP)
        Comp->SetAnimInstanceClass(RetargetABP->GeneratedClass);

    for (const auto& Morph : Morphs)
        Comp->SetMorphTarget(Morph.Key, Morph.Value);
}

void UAC_NPC_Clothing::LootUniform()
{
    if (bUniformLooted) return;
    bUniformLooted = true;

    if (UniformMesh)
        UniformMesh->SetSkeletalMesh(nullptr);

    FollowerMeshes.Remove(UniformMesh);
    OnUniformLooted.Broadcast();
}

void UAC_NPC_Clothing::SetMeshLODBias(int32 LODBias)
{
    if (BodyMesh)
        BodyMesh->SetForcedLOD(LODBias);

    for (USkeletalMeshComponent* Follower : FollowerMeshes)
        if (Follower)
            Follower->SetForcedLOD(LODBias);
}

void UAC_NPC_Clothing::SetMeshVisibility(bool bVisible)
{
    if (BodyMesh)
        BodyMesh->SetVisibility(bVisible);

    for (USkeletalMeshComponent* Follower : FollowerMeshes)
        if (Follower)
            Follower->SetVisibility(bVisible);
}

void UAC_NPC_Clothing::SetFollowerTickInterval(float Interval)
{
    for (USkeletalMeshComponent* Follower : FollowerMeshes)
        if (Follower)
            Follower->SetComponentTickInterval(Interval);
}

void UAC_NPC_Clothing::SetAllMeshesVisible(bool bVisible)
{
    SetMeshVisibility(bVisible);
}

void UAC_NPC_Clothing::ApplyClothingLODBias(int32 LODBias)
{
    if (BodyMesh)
    {
        BodyMesh->SetForcedLOD(LODBias);
    }

    for (USkeletalMeshComponent* Follower : FollowerMeshes)
    {
        if (Follower)
        {
            Follower->SetForcedLOD(LODBias);
        }
    }
}

void UAC_NPC_Clothing::ApplyClothingVisibility(bool bVisible)
{
    if (BodyMesh)
    {
        BodyMesh->SetVisibility(bVisible);
    }

    for (USkeletalMeshComponent* Follower : FollowerMeshes)
    {
        if (Follower)
        {
            Follower->SetVisibility(bVisible);
        }
    }
}

void UAC_NPC_Clothing::ApplyClothingFollowerTickInterval(float Interval)
{
    for (USkeletalMeshComponent* Follower : FollowerMeshes)
    {
        if (Follower && Follower != UniformMesh) // Protect core uniform structural syncing
        {
            Follower->SetComponentTickInterval(Interval);
        }
    }
}