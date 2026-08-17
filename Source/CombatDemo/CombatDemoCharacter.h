#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Logging/LogMacros.h"
#include "CombatDemoCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAbilitySystemComponent;
class UPlayerAttributeSet;
class UGameplayAbility;
class UPlayerStatusWidget;
class UBossHealthBarWidget;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ACombatDemoCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    // ========== GAS 组件 ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
    UAbilitySystemComponent* AbilitySystemComponent;

    UPROPERTY()
    UPlayerAttributeSet* PlayerAttributeSet;
    // ========== GAS 组件结束 ==========

    // 在 ACombatDemoCharacter 的 public 或 protected 区域添加
public:
    /** 右手斧头组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    UStaticMeshComponent* AxeMesh;

    /** 左手盾牌组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    UStaticMeshComponent* ShieldMesh;
protected:

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* LookAction;

    /** Mouse Look Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* MouseLookAction;

    /** Attack Input Action — 新增 */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* AttackAction;

    /** Block Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* BlockAction;

    /** Called for block input */
    void OnBlockPressed();
    void OnBlockReleased();

    /** 初始技能 */
    UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
    TArray<TSubclassOf<UGameplayAbility>> InitialAbilities;

    // ========== UI ==========
    /** 玩家状态HUD（血条+精力条）Widget 蓝图类，在 BP_ThirdPersonCharacter 中指定 WBP_PlayerStatus */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPlayerStatusWidget> PlayerStatusWidgetClass;

    /** Boss 血条 Widget 蓝图类，在 BP_ThirdPersonCharacter 中指定 WBP_BossHealthBar */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UBossHealthBarWidget> BossHealthBarWidgetClass;
    // ========== UI 结束 ==========

    // ========== 精力恢复 ==========
    /** 精力每秒恢复量（默认5） */
    UPROPERTY(EditAnywhere, Category = "GAS|Stamina")
    float StaminaRegenPerSecond = 5.0f;

    /** 精力恢复计时累积器（每秒触发一次恢复） */
    float StaminaRegenAccumulator = 0.0f;
    // ========== 精力恢复结束 ==========

private:
    // 击倒状态跟踪：是否上一帧处于击倒状态
    bool bWasStunned = false;

    // 处理击倒状态变化
    void UpdateStunnedState(float DeltaTime);

    // 精力每秒恢复
    void RegenStamina(float DeltaTime);

    // 保存 Widget 实例（防止被 GC）
    UPROPERTY()
    UPlayerStatusWidget* PlayerStatusWidget;

    UPROPERTY()
    UBossHealthBarWidget* BossHealthBarWidget;

public:

    /** Constructor */
    ACombatDemoCharacter();

    // GAS 接口
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

    /** Initialize input action bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
protected:

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

    /** Called for attack input — 新增 */
    void OnAttackPressed();

public:

    /** Handles move inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(float Right, float Forward);

    /** Handles look inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoLook(float Yaw, float Pitch);

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();
    // 翻滚方向（由输入设置，GA读取）
public:
    FVector DodgeDirection;

protected:
    /** Dodge Input Action */
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* DodgeAction;

    void OnDodgePressed(const FInputActionValue& Value);
    void UpdateDodgeDirection();

    FVector2D LastMovementInput;

public:

    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:
    virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};
