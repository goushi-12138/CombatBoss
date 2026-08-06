#include "Boss/AnimNotify_SendGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"

void UAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !EventTag.IsValid())
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	// 检查 Owner 是否实现了 IAbilitySystemInterface
	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner))
	{
		UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
		if (ASC)
		{
			FGameplayEventData EventData;
			// 可以填充 payload，这里留空
			ASC->HandleGameplayEvent(EventTag, &EventData);
			// 或者使用 SendGameplayEventToActor
			// ASC->SendGameplayEventToActor(Owner, EventTag, EventData);
		}
	}
}