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
#include "Blueprint/UserWidget.h"
#include "UI/PlayerStatusWidget.h"
#include "UI/BossHealthBarWidget.h"

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

    // 创建斧头组件，挂接到右手 Socket
    AxeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AxeMesh"));
    AxeMesh->SetupAttachment(GetMesh(), FName("AxeSocket"));
    AxeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 武器一般不需要碰撞，用伤害判定

    // 创建盾牌组件，挂接到左手 Socket
    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    ShieldMesh->SetupAttachment(GetMesh(), FName("ShieldSocket"));
    ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 盾牌如果需要碰撞挡子弹可以开启，此处先关闭

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

    SetActorTickEnabled(true);

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

    // ===== 创建 UI =====
    // 玩家状态 HUD（血条+精力条，屏幕左下方）
    if (PlayerStatusWidgetClass)
    {
        PlayerStatusWidget = CreateWidget<UPlayerStatusWidget>(GetWorld(), PlayerStatusWidgetClass);
        if (PlayerStatusWidget)
        {
            PlayerStatusWidget->AddToViewport();
        }
    }
    // Boss 血条（屏幕上方中央，靠近Boss渐显/远离渐隐）
    if (BossHealthBarWidgetClass)
    {
        BossHealthBarWidget = CreateWidget<UBossHealthBarWidget>(GetWorld(), BossHealthBarWidgetClass);
        if (BossHealthBarWidget)
        {
            BossHealthBarWidget->AddToViewport();
        }
    }
    // ===== 创建 UI 结束 =====
}

void ACombatDemoCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateStunnedState(DeltaSeconds);
    RegenStamina(DeltaSeconds);
}

void ACombatDemoCharacter::RegenStamina(float DeltaTime)
{
    if (!AbilitySystemComponent) return;

    // 每秒恢复 StaminaRegenPerSecond 点精力（上限 MaxStamina）
    StaminaRegenAccumulator += DeltaTime;
    while (StaminaRegenAccumulator >= 1.0f)
    {
        StaminaRegenAccumulator -= 1.0f;

        const float Stamina = AbilitySystemComponent->GetNumericAttribute(UPlayerAttributeSet::GetStaminaAttribute());
        const float MaxStamina = AbilitySystemComponent->GetNumericAttribute(UPlayerAttributeSet::GetMaxStaminaAttribute());
        if (Stamina < MaxStamina)
        {
            AbilitySystemComponent->SetNumericAttributeBase(
                UPlayerAttributeSet::GetStaminaAttribute(),
                FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond));
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

        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &ACombatDemoCharacter::OnBlockPressed);
        EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &ACombatDemoCharacter::OnBlockReleased);

        // Dodging
        EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ACombatDemoCharacter::OnDodgePressed);
    }
    else
    {
        UE_LOG(LogCombatDemo, Error, TEXT("Failed to find Enhanced Input Component!"));
    }
}

void ACombatDemoCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    LastMovementInput = MovementVector;
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

    // 击倒期间禁止攻击
    FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag(FName("Status.Stunned"));
    if (AbilitySystemComponent->HasMatchingGameplayTag(StunnedTag))
        return;

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
    // 【倒地禁移动】倒地/起身蒙太奇播放期间（Status.Downed 标签存在）忽略移动输入。
    // 这里只拦截输入层，不修改 CharacterMovement 的 MovementMode，
    // 保证倒地蒙太奇的根运动（被Boss打飞一小段距离）正常播放。
    if (AbilitySystemComponent)
    {
        if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Downed"))))
            return;
        if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Stunned"))))
            return;
    }

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

void ACombatDemoCharacter::UpdateStunnedState(float DeltaTime)
{
    if (!AbilitySystemComponent)
        return;

    // 检查当前是否拥有击倒标签
    FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag(FName("Status.Stunned"));
    bool bIsStunnedNow = AbilitySystemComponent->HasMatchingGameplayTag(StunnedTag);

    // 仅在状态变化时处理
    if (bIsStunnedNow != bWasStunned)
    {
        bWasStunned = bIsStunnedNow;

        // 获取 CharacterMovement 和 Controller
        UCharacterMovementComponent* MovementComp = GetCharacterMovement();
        APlayerController* PC = Cast<APlayerController>(GetController());

        if (bIsStunnedNow)
        {
            // 进入击倒：禁用移动和输入
            if (MovementComp)
            {
                MovementComp->SetMovementMode(MOVE_None);
            }
            if (PC)
            {
                // 禁用输入（包括移动、跳跃、攻击等）
                PC->SetIgnoreLookInput(true);
                PC->SetIgnoreMoveInput(true);
                // 也可以使用 DisableInput(PC) 但 SetIgnore 更精细
            }
        }
        else
        {
            // 恢复：启用移动和输入
            if (MovementComp)
            {
                MovementComp->SetMovementMode(MOVE_Walking);
            }
            if (PC)
            {
                PC->ResetIgnoreLookInput();
                PC->ResetIgnoreMoveInput();
                // 如果使用了 DisableInput，则用 EnableInput(PC)
            }
        }
    }
}

void ACombatDemoCharacter::OnBlockPressed()
{
    if (!AbilitySystemComponent) return;

    // 检查是否已经处于防御状态，防止重复激活
    FGameplayTag BlockingTag = FGameplayTag::RequestGameplayTag(FName("Status.Blocking"));
    if (AbilitySystemComponent->HasMatchingGameplayTag(BlockingTag))
        return;

    // 激活防御 GA
    FGameplayTagContainer BlockTag;
    BlockTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));
    AbilitySystemComponent->TryActivateAbilitiesByTag(BlockTag);
}

void ACombatDemoCharacter::OnBlockReleased()
{
    if (!AbilitySystemComponent) return;

    // 取消所有带有 Ability.Player.Block 标签的技能
    FGameplayTagContainer BlockTag;
    BlockTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));
    AbilitySystemComponent->CancelAbilities(&BlockTag);
}

void ACombatDemoCharacter::OnDodgePressed(const FInputActionValue& Value)
{
    if (!AbilitySystemComponent) return;

    // 根据当前移动输入设置翻滚方向，如果没有移动输入则默认向前
    UpdateDodgeDirection();

    // 激活翻滚GA
    FGameplayTagContainer DodgeTag;
    DodgeTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Dodge")));
    const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(DodgeTag);
    if (!bActivated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Dodge] Failed to activate dodge GA! (GA可能仍处于激活/卡死状态)"));
    }
}

void ACombatDemoCharacter::UpdateDodgeDirection()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        DodgeDirection = GetActorForwardVector().GetSafeNormal2D();
        return;
    }

    FRotator ControlRotation = PC->GetControlRotation();
    FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // 根据最后的移动输入组合方向
    if (LastMovementInput.IsNearlyZero())
    {
        // 没有输入时默认视角前方
        DodgeDirection = Forward;
    }
    else
    {
        DodgeDirection = (Forward * LastMovementInput.Y + Right * LastMovementInput.X).GetSafeNormal2D();
    }
}