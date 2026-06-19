#include "AC_NPC_Clothing.h"
#include "Engine/AssetManager.h"
#include "Animation/AnimBlueprint.h" // Required to resolve ->GeneratedClass syntax hooks
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

UAC_NPC_Clothing::UAC_NPC_Clothing()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAC_NPC_Clothing::InitializeClothing(UNPCClothingDefinition* Definition, bool bWasLooted)
{
    if (!Definition) return;

    ClothingDefinition = Definition;
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

    // FIX: Extract the raw pointer structure safely from the loaded asset data arrays
    UAnimBlueprint* RawRetargetABP = ClothingDefinition->SkeletonType == ESkeletonType::UE4Mannequin
        ? ClothingDefinition->RetargetABP_UE4.Get()
        : ClothingDefinition->RetargetABP_UE5.Get();

    for (auto& Pair : ClothingDefinition->MeshParts)
    {
        if (Pair.Key == EBodyPart::Uniform && bUniformLooted) continue;

        // FIX: Replaced USkeletalMeshComponent* with USkeletalMesh* matching your asset configuration properties
        USkeletalMesh* LoadedMesh = Pair.Value.MeshAsset.Get();
        if (!LoadedMesh) continue;

        FString EnumString = UEnum::GetValueAsString(Pair.Key);
        int32 ColonIndex = EnumString.Find("::");
        FName Tag = (ColonIndex != INDEX_NONE) ? FName(*EnumString.Right(EnumString.Len() - ColonIndex - 2)) : FName(*EnumString);

        USkeletalMeshComponent* Comp = FindMeshByTag(Tag);
        if (!Comp) continue;

        bool bIsBody = (Pair.Key == EBodyPart::Body);

        ApplyMeshToComponent(
            Comp,
            LoadedMesh,
            bIsBody ? nullptr : RawRetargetABP,
            Pair.Value.MorphTargets);

        if (!bIsBody && BodyMesh)
            Comp->SetLeaderPoseComponent(BodyMesh);
    }

    SetAllMeshesVisible(true);
    bClothingLoaded = true;
    OnClothingLoaded.Broadcast();
}

// FIX: Aligned signature to match the exact pointer parameters set up in your header file definitions
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