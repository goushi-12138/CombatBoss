#include "Player/PlayerAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    AActor* Owner = GetOwningActor();
    if (Owner)
    {
        IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
        if (ASCInterface)
        {
            AbilitySystemComponent = ASCInterface->GetAbilitySystemComponent();
        }
    }

    StunnedTag = FGameplayTag::RequestGameplayTag(FName("Status.Stunned"));
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    ACharacter* Character = Cast<ACharacter>(GetOwningActor());
    if (!Character) return;

    Speed = Character->GetVelocity().Size2D();
    bIsInAir = Character->GetCharacterMovement()->IsFalling();

    if (AbilitySystemComponent)
    {
        bIsStunned = AbilitySystemComponent->HasMatchingGameplayTag(StunnedTag);
    }

    if (AbilitySystemComponent)
    {
        bIsStunned = AbilitySystemComponent->HasMatchingGameplayTag(StunnedTag);
        bIsBlocking = AbilitySystemComponent->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag(FName("Status.Blocking")));
    }
}