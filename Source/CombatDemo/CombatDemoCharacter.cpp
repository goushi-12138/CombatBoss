#include "CombatDemoCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"
#include "Player/PlayerAttributeSet.h"
#include "CombatDemo.h"

ACombatDemoCharacter::ACombatDemoCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
    GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // ========== 创建 GAS 组件 ==========
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* ACombatDemoCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ACombatDemoCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 初始化 GAS
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    // 创建并注册属性集
    PlayerAttributeSet = NewObject<UPlayerAttributeSet>(this);
    AbilitySystemComponent->AddAttributeSetSubobject(PlayerAttributeSet);

    // 赋予初始技能
    for (TSubclassOf<UGameplayAbility> AbilityClass : InitialAbilities)
    {
        if (AbilityClass)
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
        }
    }
}

void ACombatDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACombatDemoCharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACombatDemoCharacter::Look);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACombatDemoCharacter::Look);

        // Attack — 新增
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ACombatDemoCharacter::OnAttackPressed);
    }
    else
    {
        UE_LOG(LogCombatDemo, Error, TEXT("Failed to find Enhanced Input Component!"));
    }
}

void ACombatDemoCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void ACombatDemoCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACombatDemoCharacter::OnAttackPressed()
{
    if (!AbilitySystemComponent) return;

    // 1. 先尝试激活连招 GA（首次攻击或上一轮已结束）
    FGameplayTagContainer AttackTag;
    AttackTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));
    bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(AttackTag);

    if (bActivated)
    {
        // 首次激活成功
        UE_LOG(LogTemp, Warning, TEXT("[Combo] First hit or new activation"));
    }
    else
    {
        // 2. 激活失败说明 GA 正在运行（连招窗口等待中），发送事件通知它
        UE_LOG(LogTemp, Warning, TEXT("[Combo] GA already active, sending ComboInput event"));

        FGameplayEventData EventData;
        AbilitySystemComponent->HandleGameplayEvent(
            FGameplayTag::RequestGameplayTag(FName("Event.Player.ComboInput")),
            &EventData
        );
    }
}

void ACombatDemoCharacter::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDirection, Forward);
        AddMovementInput(RightDirection, Right);
    }
}

void ACombatDemoCharacter::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void ACombatDemoCharacter::DoJumpStart()
{
    Jump();
}

void ACombatDemoCharacter::DoJumpEnd()
{
    StopJumping();
}

float ACombatDemoCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    UE_LOG(LogTemp, Warning, TEXT("Player took %f damage!"), Damage);

    if (AbilitySystemComponent && PlayerAttributeSet)
    {
        // 通过 GAS 处理伤害（需要配合伤害GE）
        // 这里先保留日志，后续接入完整伤害系统
    }

    return Damage;
}
