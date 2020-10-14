#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilityBase.generated.h"

// type of ability
UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	ACTIVE = 0 UMETA(DisplayName = "Active"),
	PASSIVE = 1 UMETA(DisplayName = "Passive")
};

UCLASS()
class SANKARI_API AAbilityBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbilityBase();

	virtual void Tick(float DeltaTime) override;

	// All blueprints based on this base class can implement the UseAbility Event
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
		void UseAbility(class AActor* CharacterReference);

	// All blueprints based on this base class can implement the OnAbilityCooldownEnded Event
	// !!!Should only be implemented on abilities that arent pasives!!!
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
		void OnAbilityCooldownEnded();

	UFUNCTION(BlueprintCallable, Category = "Ability")
		void CastedAbility();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
		bool CanCastAbility() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
		float CooldownPercentage() const;

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* inProperty) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", Meta = (DisplayName = "Type"))
		EAbilityType m_Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", Meta = (DisplayName = "Name"))
		FString m_Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", Meta = (DisplayName = "Cooldown"))
		float m_Cooldown = 0.0f;

private:
	float m_TimeSinceAbilityUsed = FLT_MAX;
	bool m_CanUseAbility = true;
};

USTRUCT(BlueprintType)
struct FAbilityInfo
{
	GENERATED_BODY()

	FAbilityInfo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Info", Meta = (DisplayName = "Icon"))
		class UTexture2D* m_pUIIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Info", Meta = (DisplayName = "AbilityReference"))
		class AAbilityBase* m_pAbilityRef = nullptr;
};
