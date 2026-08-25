# BossCombatDemo

UE5.7 + GAS + 行为树制作的 Boss 战 Demo，用于战斗策划实习作品展示。

## 🎮 战斗演示视频

[点击观看 B 站演示视频](https://www.bilibili.com/video/BV1mNh364E1n/?share_source=copy_web&vd_source=a8f100e49cebd96a99f68d0a73840ef0)

## 📄 战斗设计文档

查看完整设计文档：

- [战斗设计文档](Docs/CombatDesign.md)

## ✨ 核心特性

### Boss 两阶段战斗
- 一阶段：近战压制
- 二阶段：狂暴追击，伤害提升 30%
- 转阶段怒吼动画

### Boss AI 行为树
- 远距离追击
- 中距离范围攻击
- 近距离压制
- 动画驱动转身

### 玩家操作
- 四段连招（支持预输入）
- 右键防御（正面减伤 70%，背面无效）
- 翻滚闪避（基于镜头方向，期间无敌）

### 受击反馈
- 普通攻击：四方向摇晃
- 跳劈：倒地后自动起身

## 🛠 技术栈

- Unreal Engine 5.7
- Gameplay Ability System (GAS)
- Behavior Tree
- Enhanced Input

## 📁 项目结构

| 路径 | 说明 |
|------|------|
| `Source/CombatDemo/Public/Boss/` | Boss 相关 C++ 类 |
| `Source/CombatDemo/Public/Player/` | 玩家相关 C++ 类 |
| `Docs/` | 战斗设计文档 |
| `Media/` | 行为树截图等展示素材 |

## ⚙️ 构建说明

本项目基于 UE5.7，需在 UE 编辑器中打开 `.uproject` 文件。
