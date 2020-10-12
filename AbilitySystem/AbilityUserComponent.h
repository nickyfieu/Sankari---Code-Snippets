// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityBase.h"
#include "AbilityUserComponent.generated.h"


UCLASS( ClassGroup=(Abilities), meta=(BlueprintSpawnableComponent) )
class SANKARI_API UAbilityUserComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAbilityUserComponent();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
		void UseAbility(int index);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
		void AddAbility(class UChildActorComponent* abilityToAdd, class UTexture2D* abilityUIIcon = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
		FAbilityInfo GetAbilityInfo(int index) const;
	
	void SetAbilityAtIndex(int index, FAbilityInfo newAbility);

protected:
	virtual void BeginPlay() override;

private:
	bool AbilityPreCheck(uint32 index) const;
	TArray<FAbilityInfo> m_KnownAbilities;
};
