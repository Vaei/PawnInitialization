Add UPawnInitializationComponent to your Pawn/Character

Call `PawnInitialization->HandleControllerChanged();` for overrides `PossessedBy` `UnPossessed` `OnRep_Controller`
Call `PawnInitialization->HandlePlayerStateReplicated();` for override `OnRep_PlayerState`
Call `PawnInitialization->SetupPlayerInputComponent();` for override `SetupPlayerInputComponent`

Now bind to it:

```cpp
	UFUNCTION()
	virtual void OnCharacterReady(const FActorInitStateChangedParams& Params);
```

```cpp
	Super::BeginPlay();

	FActorInitStateChangedBPDelegate Delegate;
	Delegate.BindDynamic(this, &ThisClass::OnCharacterReady);
	PawnInitialization->RegisterAndCallForInitStateChange(PawnInitializationTags::InitState_GameplayReady, Delegate);
```

Or do the equivalent in BP.
