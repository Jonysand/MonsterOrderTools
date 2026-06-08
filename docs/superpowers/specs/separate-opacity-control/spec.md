## ADDED Requirements

### Requirement: Separate Opacity Control

用户可以分别设置非穿透模式和穿透模式的窗口透明度

#### Scenario: 非穿透模式下使用非穿透透明度

- **WHEN** 用户未锁定窗口（mIsLocked = false）
- **THEN** 窗口使用 `opacity` 配置值作为透明度（范围 0-100）

#### Scenario: 穿透模式下使用穿透模式透明度

- **WHEN** 用户锁定窗口（mIsLocked = true）
- **THEN** 窗口使用 `penetratingModeOpacity` 配置值作为透明度（范围 0-100）

#### Scenario: 默认透明度值

- **WHEN** 用户首次使用应用
- **THEN** 非穿透模式透明度默认值为 80
- **AND** 穿透模式透明度默认值为 50

#### Scenario: 透明度配置持久化

- **WHEN** 用户调整透明度滑块并保存配置
- **THEN** 配置值被持久化到配置文件
- **AND** 下次启动时应用读取并应用这些值

#### Scenario: 透明度实时生效

- **WHEN** 用户在配置窗口调整透明度滑块
- **THEN** 点怪窗口的透明度立即反映变化
