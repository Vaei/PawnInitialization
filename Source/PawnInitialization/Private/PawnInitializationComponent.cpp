// Copyright (c) Jared Taylor. All Rights Reserved.


#include "PawnInitializationComponent.h"

#include "PawnInitializationTags.h"
#include "Components/GameFrameworkComponentManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PawnInitializationComponent)

const FName UPawnInitializationComponent::NAME_ActorFeatureName("PawnInitialization");

UPawnInitializationComponent::UPawnInitializationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPawnInitializationComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
	if (!CurrentState.IsValid() && DesiredState == PawnInitializationTags::InitState_Spawned)
	{
		// As long as we are on a valid pawn, we count as spawned
		if (Pawn)
		{
			return true;
		}
	}
	if (CurrentState == PawnInitializationTags::InitState_Spawned && DesiredState == PawnInitializationTags::InitState_DataAvailable)
	{
		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			// Check for being possessed by a controller.
			if (!GetController<AController>())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == PawnInitializationTags::InitState_DataAvailable && DesiredState == PawnInitializationTags::InitState_DataInitialized)
	{
		// Transition to initialize if all features have their data available
		return Manager->HaveAllFeaturesReachedInitState(Pawn, PawnInitializationTags::InitState_DataAvailable);
	}
	else if (CurrentState == PawnInitializationTags::InitState_DataInitialized && DesiredState == PawnInitializationTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UPawnInitializationComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	// This is currently all handled by other components listening to this state change
	// if (DesiredState == PawnInitializationTags::InitState_DataInitialized)
}

void UPawnInitializationComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// If another feature is now in DataAvailable, see if we should transition to DataInitialized
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == PawnInitializationTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

void UPawnInitializationComponent::CheckDefaultInitialization()
{
	// Before checking our progress, try progressing any other features we might depend on
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = { PawnInitializationTags::InitState_Spawned, PawnInitializationTags::InitState_DataAvailable, PawnInitializationTags::InitState_DataInitialized, PawnInitializationTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UPawnInitializationComponent::OnRegister()
{
	Super::OnRegister();
	
	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf((Pawn != nullptr), TEXT("GPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one GPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

	// Register with the init state system early, this will only work if this is a game world
	RegisterInitStateFeature();
}

void UPawnInitializationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Listen for changes to all features
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	
	// Notifies state manager that we have spawned, then try rest of default initialization
	ensure(TryToChangeInitState(PawnInitializationTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UPawnInitializationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	
	Super::EndPlay(EndPlayReason);
}

void UPawnInitializationComponent::HandleControllerChanged()
{
	CheckDefaultInitialization();
}

void UPawnInitializationComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void UPawnInitializationComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}
