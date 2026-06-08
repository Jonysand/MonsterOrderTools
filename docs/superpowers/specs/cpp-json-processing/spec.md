## ADDED Requirements

### Requirement: 加载配置文件
系统 SHALL 能够从指定的JSON文件加载MainConfig配置数据。

#### Scenario: 成功加载配置文件
- **WHEN** 提供有效的配置文件路径
- **THEN** 系统返回解析后的MainConfig对象

#### Scenario: 配置文件不存在
- **WHEN** 提供的配置文件路径不存在
- **THEN** 系统抛出FileNotFoundException异常

#### Scenario: 配置文件格式错误
- **WHEN** 配置文件包含无效的JSON格式
- **THEN** 系统抛出JsonParseException异常

### Requirement: 保存配置文件
系统 SHALL 能够将MainConfig配置数据保存到指定的JSON文件。

#### Scenario: 成功保存配置文件
- **WHEN** 提供有效的MainConfig对象和文件路径
- **THEN** 系统将配置数据序列化为JSON并写入文件

#### Scenario: 保存路径无效
- **WHEN** 提供的文件路径目录不存在
- **THEN** 系统抛出IOException异常

### Requirement: 加载怪物数据
系统 SHALL 能够从指定的JSON文件加载怪物数据。

#### Scenario: 成功加载怪物数据
- **WHEN** 提供有效的怪物数据文件路径
- **THEN** 系统返回解析后的怪物数据字典

#### Scenario: 怪物数据文件格式错误
- **WHEN** 怪物数据文件包含无效的JSON格式
- **THEN** 系统抛出JsonParseException异常

### Requirement: 加载优先级队列
系统 SHALL 能够从指定的JSON文件加载优先级队列数据。

#### Scenario: 成功加载优先级队列
- **WHEN** 提供有效的优先级队列文件路径
- **THEN** 系统返回解析后的优先级队列数据

#### Scenario: 优先级队列文件为空
- **WHEN** 优先级队列文件存在但为空
- **THEN** 系统返回空的优先级队列

### Requirement: 保存优先级队列
系统 SHALL 能够将优先级队列数据保存到指定的JSON文件。

#### Scenario: 成功保存优先级队列
- **WHEN** 提供有效的优先级队列数据和文件路径
- **THEN** 系统将数据序列化为JSON并写入文件

### Requirement: 解析弹幕数据
系统 SHALL 能够从JSON字符串解析弹幕数据。

#### Scenario: 成功解析弹幕数据
- **WHEN** 提供有效的弹幕JSON字符串
- **THEN** 系统返回解析后的Danmu对象

#### Scenario: 弹幕数据格式错误
- **WHEN** 弹幕JSON字符串格式无效
- **THEN** 系统抛出JsonParseException异常

### Requirement: 统一错误处理
系统 SHALL 提供统一的JSON处理错误处理机制。

#### Scenario: JSON解析错误
- **WHEN** JSON解析过程中发生错误
- **THEN** 系统抛出包含详细错误信息的JsonParseException

#### Scenario: 文件I/O错误
- **WHEN** 文件读写过程中发生错误
- **THEN** 系统抛出包含详细错误信息的IOException